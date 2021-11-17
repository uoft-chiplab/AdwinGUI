#include <ansi_c.h>
#include <gpib.h>
#include <userint.h>
#include <stdlib.h>
#include <analysis.h>
#include <visa.h>
#include "GPIB_SRS_SETUP.h"
#include "GPIB_SRS_SETUP2.h"
#include "vars.h"




// Stefan Trotzky - 2013-01-30 -- massive changes to the GPIB settings.
// Allowing for (currently) up to 10 devices to be programmed in a
// versatile fashion, by entering a command string that can be filled
// with values stored in a table. A scan can change these values, allowing
// for scans of GPIB device parameters.

// Scott Smale, Kenneth Jackson - 2019-01-28 -- more big changes to GPIB
// Modified all GPIB communication to use the VISA library/format.
//	No more calls to ibwrt, ibrd, et cetera. Now uses viOpen, viWrite, viRead.
//  My appologies but to keep backwards compatibility this file is
//	VISA_SRS_SETUP.c but the other files retain their names ie.
//	GPIB_SRS_SETUP.h (GUI premade header file), GPIB_SRS_SETUP2.h (our fns),
//	and GPIB_SRS_SETUP.uir (the GUI).
// GPIB addresses are allowed values from 0 to MAXVISAADDR. Any values larger
// than 30 (the max GPIB address) are interpreted as VISA addresses.
// VISA addresses are defined in vars.h and the variables should have the
// form X_NAME and X_ADDR. X_NAME contains the string that is the VISA
// resource name or the VISA alias. X_ADDR is the corresponding VISA
// pseudo-address. Ex:
// #define RIGOL_DG1022Z_NAME ("Rigol_DG1022Z")
// #define RIGOL_DG1022Z_ADDR (100)
// You set X_NAME using MAX the Measurement and Automation program.
// The X_ADDR is something you choose as the fake GPIB address to put in the
// "device address" field in the GUI.



int CVICALLBACK GPIB_OK_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			ApplyGPIBPanel();
			//GetCtrlVal(panelHandle13, SETUP_GPIB_ADDRESS,&GPIB_address);
			//GPIB_device = ibdev (0, GPIB_address, NO_SAD, T10s, 1, 0);
			//GetCtrlVal(panelHandle13,SETUP_SRS_ampl,&SRS_amplitude);
			//GetCtrlVal(panelHandle13, SETUP_SRS_offst,&SRS_offset);
			//GetCtrlVal(panelHandle13, SETUP_CHECKBOX,&GPIB_ON);
			HidePanel(panelHandle13);
			break;
		}
	return 0;
}


// Fill controls from GPIBDev structure
void SetGPIBPanel(void)

{
	int devnum, j;

	GetCtrlVal (panelHandle13,SETUP_GPIB_DEVICENO, &devnum);

	SetCtrlVal (panelHandle13,SETUP_GPIB_LASTNO, devnum);
	SetCtrlVal (panelHandle13,SETUP_GPIB_ADDRESS, GPIBDev[devnum-1].address);
	SetCtrlVal (panelHandle13,SETUP_CHECKBOX, GPIBDev[devnum-1].active);
	SetCtrlVal (panelHandle13,SETUP_GPIB_TXT_COMMAND, GPIBDev[devnum-1].cmdmask);
	for (j=0;j<NUMGPIBPROGVALS;j++)
	{
		SetTableCellVal(panelHandle13, SETUP_GPIB_TAB_VALS,
				MakePoint(j+1,1),GPIBDev[devnum-1].value[j]);
	}

}

// Store changes to GPIBDev structure
void ApplyGPIBPanel(void)

{
	int devnum, j;
	char command[1024];

	GetCtrlVal (panelHandle13,SETUP_GPIB_LASTNO, &devnum);

	GetCtrlVal (panelHandle13,SETUP_GPIB_ADDRESS, &GPIBDev[devnum-1].address);
	GetCtrlVal (panelHandle13,SETUP_CHECKBOX, &GPIBDev[devnum-1].active);
	GetCtrlVal (panelHandle13,SETUP_GPIB_TXT_COMMAND, &GPIBDev[devnum-1].cmdmask);
	for (j=0;j<NUMGPIBPROGVALS;j++)
	{
		GetTableCellVal(panelHandle13, SETUP_GPIB_TAB_VALS,
				MakePoint(j+1,1),&GPIBDev[devnum-1].value[j]);
	}
	BuildGPIBString(devnum, GPIBDev[devnum-1].command);

	GetCtrlVal (panelHandle13,SETUP_GPIB_DEVICENO, &devnum);
	SetCtrlVal (panelHandle13,SETUP_GPIB_LASTNO, devnum);
}


// Program GPIB device now!
int CVICALLBACK GPIB_BUILD_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int devnum, j, device;

	switch (event)
	{
		case EVENT_COMMIT:

			GetCtrlVal (panelHandle13,SETUP_GPIB_DEVICENO, &devnum);
			SetCtrlVal (panelHandle13,SETUP_GPIB_LASTNO, devnum);
			//j = devnum - 1;

			ApplyGPIBPanel();

			SendGPIBString(devnum);

    		break;
    	}

	return 0;
}

// Building the GPIB string from the mask and the values.
void BuildGPIBString(int j, char *outstr)
{

	int k, g;
	double value;
	int count, totalReplacementsCount;
	char instr[1024];// holds the entire cmdmask
	char buffer[256];// holds a single command in the cmdmask
	char buffer2[256];
	char buffers[NUMGPIBCMDREPS][256];
	char *nextch, *lastch, *pch, *pch2;

	for( k=0; k<NUMGPIBCMDREPS; ++k ){
		for( g=0; g<256; ++g ){
			buffers[k][g] = '\0';
		}
	}

	// retrieve command string from structure
	strcpy(instr,GPIBDev[j-1].cmdmask);

	strcpy(outstr,"");			// initialize outstring
	nextch = strchr(instr,';'); // find termination of first command
	lastch = instr - 1;			// beginning of instring

	totalReplacementsCount = 0;

	//printf("Begin BuildGPIBString\n");

	while (nextch!=NULL)
	{
		strncpy(buffer,lastch+1,nextch-lastch); // copy segment without ";" to buffer
		buffer[nextch-lastch] = '\0';			// terminate segment with null character

		//printf("Start of first while loop-"); printf("%s",buffer); printf("-\n");

		k = 0;
		count = 0;
		pch = buffer;
		//printf("pch: %i\n", (int)pch);
		pch2 = strstr(pch, "%f");
		//printf("pch2: %i\n", (int)pch2);
		while( pch2 = strstr(pch, "%f") ){//pch2!=NULL
			++count;
			if( count > NUMGPIBCMDREPS ){
				printf("Too many replacements in this command:\n");
				printf("%s\n", buffer);
				--count;// decrement count so that the if count>0 can still happen safely (I guess the GPIB will choke on the %f's that are not repalced)
				break;
			}

			pch2 = pch2 + 2;// points to character after "%f"
			//printf("pch -"); printf("%s", pch); printf("-\n");
			//printf("pch2-"); printf("%s", pch2); printf("-\n");
			strncpy(buffers[k], pch, pch2 - pch);// copy upto and including "%f" into the kth buffer
			buffers[k][pch2 - pch] = '\0';
			//printf("bfs%i-", k); printf("%s", buffers[k]); printf("-\n");

			pch = pch2;// this pch is used later to append the part of the command after the last "%f" to the output string.
			++k;
			//pch2 = strstr(pch, "%f");
			//printf("pch2: %i\n", (int)pch2);
		}

		//printf("count: %i\n", count);

		// if count is greater than zero then do replacements
		if( count > 0 ){
			if( totalReplacementsCount + count < NUMGPIBPROGVALS ){// make sure we don't ask for more replacements than there are cells in the replacements array. (could be <= ?)
				for( k=0; k<count; ++k ){
					sprintf(buffer2, buffers[k], GPIBDev[j-1].value[totalReplacementsCount+k]);// for each buffer in buffers do replacement and concatenate to output string
					strcat(outstr, buffer2);
				}
				// Add the part after the last "%f" to the output string. No replacements required.
				strcat(outstr, pch);
				totalReplacementsCount += count;
			}
			else {
				printf("Too many GPIB values ...\n");
			}
		}
		else {// else no replacements to do and so copy buffer directly
			strcat(outstr,buffer);// add buffer to outstring
		}

		lastch = nextch;
		nextch = strchr(lastch+1,';');
	}

}

// Chop GPIB command string and send to device.
// Feb2016 -- added syntax detection to generate predefined waveforms -- ST
void SendGPIBString(int j)
{
	int device;
	int i;
	double value;
	char cmdbuf[1024], buffer[256];
	char buffer2[1024]; // a second buffer array for waveform generation
	double wavpars[64]; // array of parameters to be handed to waveform generation
	char *nextch, *lastch;
	char *nextch2, *lastch2; // a second set of pointers to chop waveform commands


	ViStatus status;
	ViSession defaultRM, instrVISA;
	ViUInt32 retCount;
	ViChar bufferVISA[1024];

	// Begin by initializing the system
	printf("Opening initial defaultRM connection...");
	status = viOpenDefaultRM(&defaultRM);
	if( status < VI_SUCCESS ){
		// Some kind of error
		// Apparently don't need to try to close channel
		printf("FAILED\n");
		return;
	}
	printf("Done\n");




	/*
	// Minimal working example. Keep for reference.
	// !! No error checking
	// VISA resource name for GIPB ex "GPIB0::16::INSTR"
	// VISA resource name for Ethernet ex "TCPIP::192.168.1.170::INSTR"
	//
	// Open communication with VISA device using X_NAME
	sprintf(buffer,RIGOL_DG1022Z_NAME);
	printf(">>%s<<\n", buffer);
	// Use cast to ViBuf from char array to stop warnings.
	status = viOpen(defaultRM, buffer, VI_NULL, VI_NULL, &instrVISA);

	// Set the timeout for message-based communication
	status = viSetAttribute(instrVISA, VI_ATTR_TMO_VALUE, 5000);

	// Ask the device for identification
	status = viWrite(instrVISA, "*IDN?\n", 6, &retCount);
	status = viRead(instrVISA, bufferVISA, 1024, &retCount);
	printf(">>%s<<\n", bufferVISA);

	status = viWrite(instrVISA, "SOUR1:FREQ?\n", 13, &retCount);
	status = viRead(instrVISA, bufferVISA, 1024, &retCount);
	printf(">>%s<<\n", bufferVISA);

	status = viWrite(instrVISA, "SOUR1:FREQ 555e2;", 17, &retCount);
	//status = viRead(instrVISA, bufferVISA, 1024, &retCount);
	//printf(">>%s<<\n", bufferVISA);

	status = viWrite(instrVISA, "SOUR1:FREQ?\n", 13, &retCount);
	status = viRead(instrVISA, bufferVISA, 1024, &retCount);
	printf(">>%s<<\n", bufferVISA);

	// Close VISA
	status = viClose(instrVISA);
	status = viClose(defaultRM);
	// Done VISA
	*/



	// Open connection to specific device.
	// Check for GPIB "(pseudo)-address" to decide how to connect.
	printf("Opening connection to device...");
	if( GPIBDev[j-1].address > 30 ){// fake GPIB address so use VISA alias
		switch( GPIBDev[j-1].address )// choose device based on pseudo-address
		{
		case RIGOL_DG1022Z_ADDR:
			sprintf( buffer, RIGOL_DG1022Z_NAME );
			printf("visa pseudo-addr connect:>>%s<<\n", buffer);
			status = viOpen(defaultRM, buffer, VI_NULL, VI_NULL, &instrVISA);
			if( status < VI_SUCCESS ){// error opening connection
				printf("FAILED\n");
				return;
			}
    		break;
    	case RIGOL_DG4162_ADDR:
    		sprintf( buffer, RIGOL_DG4162_NAME );
    		printf("visa pseudo-addr connect:>>%s<<\n", buffer);
    		status = viOpen(defaultRM, buffer, VI_NULL, VI_NULL, &instrVISA);
    		if( status < VI_SUCCESS ){//error
    			printf("FAILED\n");
    			return;
    		}
    		break;
		case RIGOL_2021_ADDR:
    		sprintf( buffer, RIGOL_2021_NAME );
    		printf("visa pseudo-addr connect:>>%s<<\n", buffer);
    		status = viOpen(defaultRM, buffer, VI_NULL, VI_NULL, &instrVISA);
    		if( status < VI_SUCCESS ){//error
    			printf("FAILED\n");
    			return;
    		}
    		break;
    	}
	}
	else if( GPIBDev[j-1].address >= 0 && GPIBDev[j-1].address <= 30 ){// real GPIB address
		sprintf( buffer, "GPIB::%d::INSTR", GPIBDev[j-1].address );
		printf("visa gpib connect:>>%s<<\n", buffer);
		status = viOpen(defaultRM, buffer, VI_NULL, VI_NULL, &instrVISA);
		if( status < VI_SUCCESS ){// error opening connection
			printf("FAILED\n");
			return;
		}
	}
	printf("Done\n");

	// Set the timeout for message-based communication
	status = viSetAttribute(instrVISA, VI_ATTR_TMO_VALUE, 5000);


	// retrieve command string from structure
	strcpy(cmdbuf, GPIBDev[j-1].command);

	nextch = strchr(cmdbuf,';'); // find termination of first command
	lastch = cmdbuf - 1;			// beginning of instring


	while (nextch!=NULL)
	{

		strncpy(buffer,lastch+1,nextch-lastch); // copy segment to buffer
		buffer[nextch-lastch] = '\0';			// terminate segment with null character

		// now check whether the current command starts with "WAVF", indicating a waveform definition
		if (strncmp(buffer,"WAVF",4) == 0)
		{ // current command defines a waveform
			// cycle through blocks that are separated by whitespace
			// those should be parameters that define the waveform,
			// starting with the waveform identifier

			strcpy(buffer2,buffer);

			lastch2 = strchr(buffer2,';'); // firstly, replace terminating semicolon with whitespace
			buffer2[lastch2-buffer2] = ' ';

			// initialize next&last around first block
			lastch2 = strchr(buffer2,' ');
			nextch2 = strchr(lastch2+1,' ');
			while( lastch2+1 == nextch2 ){// eat any whitespace between the WAVF command and the first parameter (which is the device ie. DS345)
				lastch2 = nextch2;
				nextch2 = strchr(lastch2+1,' ');
			}

			i = 0;

			while( nextch2!=NULL )
			{
				strncpy(buffer2,lastch2+1,nextch2-lastch2-1); // copy block between whitespaces to buffer
				buffer2[nextch2-lastch2-1] = '\0';			// terminate

				if (strncmp(buffer2,"DS345",5)==0){ wavpars[i] = 0; } // convert type identifiers to number ids
				else if (strncmp(buffer2,"KEITH",5)==0){ wavpars[i] = 1; }
				else if (strncmp(buffer2,"DG1022Z",13)==0){ wavpars[i] = 2; }
				else if (strncmp(buffer2,"NOECHO",6)==0){ wavpars[i] = 0; }// convert waveform identifier to number ids
				else if (strncmp(buffer2,"INOECHO",7)==0){ wavpars[i] = 1; }
				else if (strncmp(buffer2,"ECHO",4)==0){ wavpars[i] = 2; }
				else if (strncmp(buffer2,"IECHO",5)==0){ wavpars[i] = 3; }
				else if (strncmp(buffer2,"RABIOSC",7)==0){ wavpars[i] = 4; }
				else if (strncmp(buffer2,"IRABIOSC",8)==0){ wavpars[i] = 5; }
				else if (strncmp(buffer2,"TRAIN",5)==0){ wavpars[i] = 6; }
				else if (strncmp(buffer2,"ITRAIN",6)==0){ wavpars[i] = 7; }
				else if (strncmp(buffer2,"NPIECHO",7)==0){ wavpars[i] = 8; }
				else if (strncmp(buffer2,"INPIECHO",8)==0){ wavpars[i] = 9; }
				else if (strncmp(buffer2,"WIGENV",6)==0){ wavpars[i] = 10; }
				else if (strncmp(buffer2,"PHASER",6)==0){ wavpars[i] = 11; }
				else { wavpars[i] = atof(buffer2); }   // store parameter in array

				//printf("-");printf("%s", buffer2);printf("-\n");

				i+=1;
				lastch2 = nextch2;
				nextch2 = strchr(lastch2+1,' ');// move on
				while( lastch2+1 == nextch2 ){
					lastch2 = nextch2;
					nextch2 = strchr(lastch2+1,' ');// move on
				}
			}

			SendVISAWaveform(instrVISA, wavpars);

		}
		else
		{ // normal command. send it!
			viWrite(instrVISA, (ViBuf)buffer, strlen(buffer), &retCount);
			//ibwrt(device, buffer, strlen(buffer));	// send command
			++DEBUG_NUMBER_IBWRT_CALLS;
			printf("DEBUG_NUMBER_IBWRT_CALLS: %i\n", DEBUG_NUMBER_IBWRT_CALLS);
			printf(buffer); printf("\n");
		}

		// proceed to next
		lastch = nextch;
		nextch = strchr(lastch+1,';');

		lastch2 = strchr(lastch+1,' '); // step over whitespaces at beginning of next command
		while (lastch+1 == lastch2 )
		{
			lastch++;
			lastch2 = strchr(lastch+1,' ');
		}

	}

 	strcpy(GPIBDev[j-1].lastsent, GPIBDev[j-1].command);
    printf(GPIBDev[j-1].lastsent); printf("\n");

    // Close connections since we are done writing
    printf("Close communication with device...");
    status = viClose(instrVISA);
    if( status < VI_SUCCESS ){// error closing connection
		printf("FAILED ERROR: %d\n",status);
	}
	printf("Done\n");
	printf("Close communication defaultRM...");
	status = viClose(defaultRM);
	if( status < VI_SUCCESS ){// error closing connection
		printf("FAILED\n");
		return;
	}
	printf("Done\n");

}


// Send an arbitrary waveform to a GPIB device (currently SRS only)
void SendVISAWaveform(ViSession instrVISA, double *pars)
{ // assuming that pars[0] is the device type, pars[1] is the waveform type

	short data[17000];// short should be 16bit, SRS is 16bit machine so in docs the SRS int is the same as a short on a 32bit machine.
	int ndata;
	int wavfchk;
	char cmd[1024];
	int len;
	char *walk;

	ViStatus status;
	ViUInt32 retCount;
	ViChar bufferVISA[1024];
	ViJobId job;

	int rigolCmdBufLen = 17000000;
	char *rigolCmd = malloc(rigolCmdBufLen * sizeof(char));// needs to be at least 8*2e6 for 25 MHz model



	// initialize flag; send only if set >0
	wavfchk = -1;

	if (pars[0] == 0)
	{ // device is an SRS DS345
		ndata = BuildSRSWaveform(data, pars, &wavfchk); // ndata/2 is number of vertices
		if (wavfchk > 0)
		{
			sprintf(cmd, "LDWF?1,%d\n", ndata/2);
			status = viWrite(instrVISA, (ViBuf) cmd, strlen(cmd), &retCount);
			status = viRead(instrVISA, (ViPBuf) bufferVISA, 1024, &retCount);
			printf(">>%s<<\n", bufferVISA);
			status = viWrite(instrVISA, (char *)data, (int)(2*ndata+2), &retCount);

			DEBUG_NUMBER_IBWRT_CALLS = DEBUG_NUMBER_IBWRT_CALLS + 2;
			printf("DEBUG_NUMBER_IBWRT_CALLS: %i\n", DEBUG_NUMBER_IBWRT_CALLS);

		}
		else
		{
			printf("Failure generating SRS arbitrary waveform; doing nothing.\n");
		}

	}
	else if (pars[0] == 1)
	{// Kiethley
		printf("Too lazy to generate arbitrary waveform; doing nothing.\n");
	}
	else if (pars[0] == 2)
	{// Rigol DG1022Z
		ndata = BuildRIGOLWaveform(rigolCmd, pars, &wavfchk, rigolCmdBufLen);// ndata is the number of voltages
		if( wavfchk > 0 ){

			printf("rigol cmd:>>%s<<\n", rigolCmd);
			printf("Sending rigol cmd...");
			//printf("num rigol cmd len: %d\n", strlen(rigolCmd));

			// Chop arbitrary waveform into 1000 byte chunks
			// Don't send END statement after each write.
			status = viSetAttribute(instrVISA, VI_ATTR_SEND_END_EN, VI_FALSE);
			walk = rigolCmd;
			len = strlen(walk);
			while( len > 1000 ){
				strncpy(cmd, walk, 1000);
				status = viWrite(instrVISA, (ViBuf) cmd, strlen(cmd), &retCount);
				if( status < VI_SUCCESS ){// error during write
					printf("FAILED ERROR: %d\n",status);
				}
				//printf("bytes sent: >>%s<<", cmd);
				walk = walk + 1000;// advance pointer 1000 places
				len = strlen(walk);// new length of string
			}
			strcpy(cmd, walk);
			// Re-enable sending END statement after a viWrite
			status = viSetAttribute(instrVISA, VI_ATTR_SEND_END_EN, VI_TRUE);
			status = viWrite(instrVISA, cmd, strlen(cmd), &retCount);
			//printf("final bytes sent: >>%s<<",cmd);

			printf("retCount after: %d\n", retCount);

			if( status < VI_SUCCESS ){// error during last write
				printf("FAILED ERROR: %d\n",status);
			}
			printf("Done\n");
		}
		else {
			printf("Failure generating RIGOL DG1022Z arbitrary waveform; doing nothing.\n");
		}

	}

	// Free memory
	free(rigolCmd);

}


// Convert a double array of values to the string format the Rigol DG1022Z wants.
void convertRigolDataToCmd(double *data, char *rigolCmd, int ndata, int nChars, int chNum){
//	Start with a comma so this array can be concatenated with
//	SOURn:DATA VOLATILE
//	since there needs to be a comma btw VOLATILE and the first data point but no
//	comma at the end of the list of numbers.
//  Ex. SOUR1:DATA VOLATILE,-0.6,-0.4,0.2, ... ,0.1,0.3

	int i;
	char *walk = rigolCmd;// pointer to walk along voltages char array

	printf("Start convert...");

	// Write the initial prefix
	sprintf(walk, "SOUR%d:DATA VOLATILE\0", chNum);
	while( *walk != '\0' ){ walk++; }// increment walk until pointing at end of string

	// Iterate over data and write each value preceeded by a comma.
	for( i = 0; i < ndata; ++i ){
		// write comma to voltages
		*walk = ',';
		walk++;

		// write voltage value to char voltage array if enough character space left
		if( walk - rigolCmd < nChars - 30 ){// Not exact, just a conservative check

			sprintf(walk, "%f\0", data[i]);
			while( *walk != '\0' ){ walk++; }// increment walk until pointing at end of string

		}
	}
	// debug
	//printf("rigolCmd:>>%s<<\n", rigolCmd);
	printf("Done convert\n");
}


// Building hard-coded wave forms based on a few input parameters
// Currently (meaning from now on in all eternity) no check is done,
// whether the number of parameters matches the selected waveform
int BuildRIGOLWaveform(char *voltages, double *pars, int *wavfchk, int nChars)
{// Rigol DG1022Z accepts a comma separated list of doubles from -1 to 1 as it's
//  	voltage levels.
//		Ex. SOUR1:DATA VOLATILE,-0.6,-0.4,0.2, ... ,0.1,0.3
//  General scheme is to calculate the arb waveform as for the SRS but instead of the amplitudes
// 		being ints they are doubles from -1.0 to 1.0. Then convert the array of points to a string.
//	Rigol does not accept vector input so all samples must be specified in contrast to
//		SRS vector mode.

//	Rigol will interpolate between values that you give it. To do sharp pulses, requires a fast sample rate.

	int i, j;
	int ndata;
	double *data = (double *) malloc(2000000 * sizeof(double));
	int startTime, offsetTime, pulseTime, initPulseTime, finalPulseTime, chNum;
	int holdEndsTime, holdPiTime, numPiPulses;

	int startLength, pulseLength, initPulseLength, finalPulseLength;
	int holdEndsLength, holdPiLength;

	int tailLength;

	double ampl, offset;

	double alpha = 0.16;// alpha param for Blackman pulse, a0 = (1-alpha)/2, a1 = 1/2, a2 = alpha/2
	double a0 = (1.0-alpha)/2.0, a1 = 0.5, a2 = alpha/2.0;
	double pi = acos(-1.0);

	double minVal, maxVal;
	int minIndex, maxIndex;

	printf("Generating Rigol waveform.\n");
	ndata = -1;

	if( pars[1] == 10 ) // Blackman envelope of wiggle
	{   // pars[2] is which channel to load waveform to.
		// pars[3] is the length of wiggle envelope
	  	// pars[4] is the amplitude of the wiggle envelope (units of 0 to 1)
	  	// pars[5] is the offset from zero (offset+amp must be less than 1)
	  	// pars[6] is the time that the envelope should finish

		chNum = pars[2];// channel number of Rigol to load waveform to.

		startTime = (int) (pars[6] - pars[3]);// start time of envelope
		pulseTime = (int) (pars[3]);
		ndata = (int) (pars[6]);

	    ampl = pars[4];
	    offset = pars[5];

	    if( startTime < 0) { *wavfchk = -1; printf("Blackman waveform failure: negative start delay.\n"); return -1;}
		if( pulseTime < 1) { *wavfchk = -1; printf("Blackman waveform failure: negative pulse length.\n"); return -1;}
		printf("Start time in samples: %i  Must be >= 0.\n", startTime);
	    printf("pulse time in samples: %i  Must be larger than 0.\n", pulseTime);
	    printf("ampl: %f and offset: %f\n", ampl, offset);

	    // values before envelope starts are offset
	    for( i=0; i<startTime; i++ ){
	    	data[i] = offset;
	    }
	    // from start time compute the Blackman envelope
	    offsetTime = (startTime >= 0) ? startTime : 0;// protect against negative index
	    for( i=0; i<pulseTime; i++ ){
	    	data[i+offsetTime] = offset + ampl*(a0 - a1*cos(2.0*pi*((double)i)/(pulseTime-1.0))\
	    										   + a2*cos(4.0*pi*((double)i)/(pulseTime-1.0)) );
	    }

		*wavfchk = 1; // waveform has been built

		// Sanity checks (return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		MaxMin1D(data, ndata, &maxVal, &maxIndex, &minVal, &minIndex);
		if( maxVal > 1.0 ){*wavfchk = -1; printf("Voltage of wavf greater than 1.0\n"); return -1;}
	    if( minVal < -1.0 ){*wavfchk = -1; printf("Voltage of wavf less than -1.0\n"); return -1;}

	  	convertRigolDataToCmd(data, voltages, ndata, nChars, chNum);
	}
	else if (pars[1] == 4)// Single pulse
	{ 	// pars[2] which channel to load waveform to.
		// pars[3] is the length of the pulse in samples
	  	// pars[4] is the time that the last pulse should finish

		chNum = pars[2];// channel number of Rigol to load waveform to.

	    pulseTime = (int) (pars[3]);
		startTime = (int) (pars[4] - pars[3]);
		ampl = 1.0;
		ndata = (int) (pars[4]);

		if( startTime < 0) { *wavfchk = -1; printf("Single pulse waveform failure: negative start delay.\n"); return -1;}
		if( pulseTime < 1) { *wavfchk = -1; printf("Single pulse waveform failure: negative pulse length.\n"); return -1;}
		printf("Start time in samples: %i  Must be >= 0.\n", startTime);
	    printf("pulse time in samples: %i  Must be larger than 0.\n", pulseTime);
	    printf("ampl: %f\n", ampl);

	    // values before pulse starts are zero
	    for( i=0; i<startTime; i++ ){
	    	data[i] = 0.0;
	    }
	    // from start time compute the Blackman envelope
	    offsetTime = (startTime >= 0) ? startTime : 0;// protect against negative index
	    for( i=0; i<pulseTime; i++ ){
	    	data[i+offsetTime] = ampl;
	    }

		*wavfchk = 1; // waveform has been built

		// Sanity checks (return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		MaxMin1D(data, ndata, &maxVal, &maxIndex, &minVal, &minIndex);
		if( maxVal > 1.0 ){*wavfchk = -1; printf("Voltage of wavf greater than 1.0\n"); return -1;}
	    if( minVal < -1.0 ){*wavfchk = -1; printf("Voltage of wavf less than -1.0\n"); return -1;}

	  	convertRigolDataToCmd(data, voltages, ndata, nChars, chNum);
	}
	else if (pars[1] == 11)// Phase Modulation of Kiethley
	{ 	// pars[2] which channel to load waveform to.
		// pars[3] is the Rabi-Period
	  	// pars[4] is the initial pulse's angle in units of Pi
	  	// pars[5] is the holdtime between the first pulse and the first Pi-pulse in us
	  	// pars[6] is the number of Pi pulses
	  	// pars[7] is the time that the last pulse should finish
	  	// pars[8] is the phase of the last (pi/2) pulse
	  	// pars[9..end] is the number of pi pulse phases


		chNum = pars[2];// channel number of Rigol to load waveform to.

		initPulseLength = (int) (pars[4]*pars[3]/2);// initial pulse time
		pulseLength = (int) (pars[3]/2);// pi pulse time
		finalPulseLength = (int) (pulseLength/2);// final (pi/2) pulse time
		holdEndsLength = (int) (pars[5]);// time between first pulse and first pi pulse
		holdPiLength = (int) (holdEndsLength*2);// time between pi pulses
		numPiPulses = (int) (pars[6]);

		// set tail time to hold last phase after the pulse sequence has ended
		tailLength = 2;

		// Calculate start time
		startLength = (int) (pars[7] - numPiPulses*pulseLength - (numPiPulses-1)*holdPiLength - 2*holdEndsLength - initPulseLength - finalPulseLength );// N pulses and N-1 hold times

		ndata = pars[7];

		printf("initPulseLength %i \n", initPulseLength);
		printf("PulseLength %i \n", pulseLength);
		printf("finalPulseLength %i \n", finalPulseLength);
		printf("holdendsLength %i \n", holdEndsLength);
		printf("holdPiLength %i \n", holdPiLength);
		printf("numPipulses %i \n", numPiPulses);
		printf("startLength %i \n", startLength);

	    if( startLength < 0) { *wavfchk = -1; printf("Phase mod waveform failure: negative start delay.\n"); return -1;}
		if( startLength < 61) { *wavfchk = -1; printf("Phase mod waveform failure: too small start delay.\n"); return -1;}
		if( pulseLength < 1) { *wavfchk = -1; printf("Phase mod waveform failure: negative pulse length.\n"); return -1;}
		if( initPulseLength < 1) { *wavfchk = -1; printf("Phase mod waveform failure: negative pulse length.\n"); return -1;}
		if( finalPulseLength < 1) { *wavfchk = -1; printf("Phase mod waveform failure: negative pulse length.\n"); return -1;}
		if( holdEndsLength < 1) { *wavfchk = -1; printf("Phase mod waveform failure: negative pulse length.\n"); return -1;}
		if( holdPiLength < 1) { *wavfchk = -1; printf("Phase mod waveform failure: negative pulse length.\n"); return -1;}

		printf("Start time in samples: %i  Must be >= 0.\n", startLength);
	    printf("pulse time in samples: %i  Must be larger than 0.\n", pulseLength);


	    // values before pulse sequence starts are zero
	    for( i=0; i<startLength; i++ ){
	    	data[i] = 0.0;
	    }
	    data[0] = -1.0;
	    // keep phase at zero for first pulse
	    offsetTime = i;
	    printf("firstoffsettime %i \n",offsetTime);
	    for( i=0; i < initPulseLength + holdEndsLength/2; i++ ){
	    	data[offsetTime+i] = 0.0;
	    }
	    // then iterate over pi pulse phases
	    for( j=0; j<numPiPulses; j++ ){
	    	offsetTime = offsetTime + i;
	    	printf("moreoffsettimes %i \n",offsetTime);
	    	for( i=0; i < pulseLength + holdPiLength; i++ ){
	    		data[offsetTime+i] = pars[9+j]/180.0;
	    	}
	    }
	    // then change for final pulse
	    offsetTime = offsetTime + i;
	    printf("finaloffsettime %i \n",offsetTime);
	    for( i=0; i<finalPulseLength+tailLength; i++ ){// +2 is to prevent the Rigol from changing phase during the pulse
	    	data[offsetTime+i] = pars[8]/180.0 ;
	    }
	    ndata = ndata + tailLength;

		*wavfchk = 1; // waveform has been built

		// Sanity checks (return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		MaxMin1D(data, ndata, &maxVal, &maxIndex, &minVal, &minIndex);
		if( maxVal > 1.0 ){*wavfchk = -1; printf("Voltage of wavf greater than 1.0\n"); return -1;}
	    if( minVal < -1.0 ){*wavfchk = -1; printf("Voltage of wavf less than -1.0\n"); return -1;}

	  	convertRigolDataToCmd(data, voltages, ndata, nChars, chNum);
	}
	else
	{

	}

	// Free memory
	free(data);

	return ndata;

}


// Building hard-coded wave forms based on a few input parameters
// Currently (meaning from now on in all eternity) no check is done,
// whether the number of parameters matches the selected waveform
int BuildSRSWaveform(short *data, double *pars, int *wavfchk)
{ // assuming a sampling rate of 1MHz, and that times are given in us
  // changing the sampling rate will rescale the timebase

	int i, j;
	int chksum, ndata;
	int pulse1, pulse2, pulse3, pulseLength, pulsePiLength, pulseFirstLength, pulseLastLength;
	int hold1, hold2, startTime, holdLength, holdEndsLength, holdPiLength;
	int ampl;

	printf("Generating SRS waveform assuming a 1MHz sampling rate; use FSMP 1E6 to set.\n");
	ndata = -1;

	if (pars[1] == 0) // Ramsey sequence without echo
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the first pulses angle in units of pi
	  // pars[4] is the holdtime between pulses in us
	  // pars[5] is the time that the last pulse should finish

	    pulse1 = (int) (pars[3]*pars[2]/2);
		pulse2 = (int) (pars[2]/4);
		hold1 = (int) (pars[4]);
		startTime = (int) (pars[5] - pars[4] - pars[3]*pars[2]/2 - pars[2]/4);
		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		// generating a waveform of two pulses separated by holdtime: preparation pulse, pi pulse, read-out pulse
		// vectorized waveform in pairs: (x,y), where x is in units of time-steps (linked to the sampling rate)
		// may need to round x values?
		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// first pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// hold time
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold1 - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// last pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse2 - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulse1 < 1) || (pulse2 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (hold1 < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 1)// Inverted (amplitude) Ramsey sequence without echo
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the first pulses angle in units of pi
	  // pars[4] is the holdtime between pulses in us
	  // pars[5] is the time that the last pulse should finish

	    pulse1 = (int) (pars[3]*pars[2]/2);
		pulse2 = (int) (pars[2]/4);
		hold1 = (int) (pars[4]);
		startTime = (int) (pars[5] - pars[4] - pars[3]*pars[2]/2 - pars[2]/4);
		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		// generating a waveform of two pulses separated by holdtime: preparation pulse, pi pulse, read-out pulse
		// vectorized waveform in pairs: (x,y), where x is in units of time-steps (linked to the sampling rate)
		// may need to round x values?
		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// first pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// hold time
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold1 - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// last pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse2 - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulse1 < 1) || (pulse2 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (hold1 < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 2) // Ramsey sequence with echo
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the first pulses angle in units of pi
	  // pars[4] is the first holdtime between pulses in us
	  // pars[5] is the second holdtime between pulses in us
	  // pars[6] is the time that the last pulse should finish

		pulse1 = (int) (pars[3]*pars[2]/2);
		pulse2 = (int) (pars[2]/2);
		pulse3 = (int) (pars[2]/4);
		hold1 = (int) (pars[4]);
		hold2 = (int) (pars[5]);
		startTime = (int) (pars[6] - pars[5] - pars[4] - pars[2]/4 - pars[2]/2 - pars[3]*pars[2]/2);

		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		// generating a waveform of three pulses separated by holdtime: preparation pulse, pi pulse, read-out pulse
		// vectorized waveform in pairs: (x,y), where x is in units of time-steps (linked to the sampling rate)
		// may need to round x values?
		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// start first pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// first hold time
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold1 - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// start echo pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse2 - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// second hold time
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold2 - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// start last pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse3 - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulse1 < 1) || (pulse2 < 1) || (pulse3 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if ((hold1 < 1) || (hold2 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 3)// Inverted (amplitude) Ramsey sequence with echo
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the first pulses angle in units of pi
	  // pars[4] is the first holdtime between pulses in us
	  // pars[5] is the second holdtime between pulses in us
	  // pars[6] is the time that the last pulse should finish

		pulse1 = (int) (pars[3]*pars[2]/2);
		pulse2 = (int) (pars[2]/2);
		pulse3 = (int) (pars[2]/4);
		hold1 = (int) (pars[4]);
		hold2 = (int) (pars[5]);
		startTime = (int) (pars[6] - pars[5] - pars[4] - pars[2]/4 - pars[2]/2 - pars[3]*pars[2]/2);

		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		// generating a waveform of three pulses separated by holdtime: preparation pulse, pi pulse, read-out pulse
		// vectorized waveform in pairs: (x,y), where x is in units of time-steps (linked to the sampling rate)
		// may need to round x values?
		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// start first pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// first hold time
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold1 - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// start echo pulse
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse2 - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// second hold time
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + hold2 - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// start last pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse3 - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing ; problem here: SRS drops output to 0 for ~1us before sampling HI again.
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + 16; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2; // does this fix it?

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulse1 < 1) || (pulse2 < 1) || (pulse3 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if ((hold1 < 1) || (hold2 < 1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 4)// Single pulse
	{ // pars[2] is the length of the pulse in us
	  // pars[3] is the time that the last pulse should finish

	    pulse1 = (int) (pars[2]);
		startTime = (int) (pars[3] - pars[2]);
		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// first pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if (pulse1 < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 5)// Inverted (amplitude) single pulse
	{ // pars[2] is the length of the pulse in us
	  // pars[3] is the time that the last pulse should finish

	    pulse1 = (int) (pars[2]);
		startTime = (int) (pars[3] - pars[2]);
		ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

		chksum = 0;
		i = 0;
		// initial zeroing and wait
		data[i] = 0; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// first pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulse1 - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;

		ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

		// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if (pulse1 < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 6) // Pulse train of arbitrary area pulses
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the pulse's angle in units of Pi
	  // pars[4] is the holdtime between pulses in us
	  // pars[5] is the number of pulses
	  // pars[6] is the time that the last pulse should finish

	  	pulseLength = (int) (pars[3]*pars[2]/2);
	  	holdLength = (int) (pars[4]);
	  	startTime = (int) (pars[6] - pars[5]*pulseLength - (pars[5]-1)*holdLength);// N pulses and N-1 hold times

	  	ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

	  	chksum = 0;
	  	i = 0;
	  	// intial zeroing and wait
	  	data[i] = 0; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// loop over all but last pulse
		for( j=0; j < pars[5]-1; ++j ){
			// pulse
			data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + pulseLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
			// not pulse
			data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + holdLength - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		}
		// last pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;

	  	ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

	  	// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if (pulseLength < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (holdLength < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 7) // Inverted (amplitude) pulse train of arbitrary area pulses
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the pulse's angle in units of Pi
	  // pars[4] is the holdtime between pulses in us
	  // pars[5] is the number of pulses
	  // pars[6] is the time that the last pulse should finish

	  	pulseLength = (int) (pars[3]*pars[2]/2);
	  	holdLength = (int) (pars[4]);
	  	startTime = (int) (pars[6] - pars[5]*pulseLength - (pars[5]-1)*holdLength);// N pulses and N-1 hold times

	  	ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

	  	chksum = 0;
	  	i = 0;
	  	// intial zeroing and wait
	  	data[i] = 0; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// loop over all but last pulse
		for( j=0; j < pars[5]-1; ++j ){
			// pulse
			data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + pulseLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
			// not pulse
			data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + holdLength - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		}
		// last pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;

	  	ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

	  	// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if (pulseLength < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if (holdLength < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly
	}
	else if (pars[1] == 8) // N Pi-pulse Ramsey sequence (N spin reversals)
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the initial pulse's angle in units of Pi
	  // pars[4] is the holdtime between the first pulse and the first Pi-pulse in us
	  // pars[5] is the number of Pi pulses
	  // pars[6] is the time that the last pulse should finish

	  	pulsePiLength = (int) (pars[2]/2);
	  	pulseFirstLength = (int) (pars[3]*pars[2]/2);
	  	pulseLastLength = (int) (pars[2]/4);
	  	holdPiLength = (int) (2*pars[4]);
	  	holdEndsLength = (int) (pars[4]);
	  	startTime = (int) (pars[6] - pars[5]*pulsePiLength - (pars[5]-1)*holdPiLength - 2*holdEndsLength-pulseFirstLength - pulseLastLength);// N pulses and N-1 hold times

	  	ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

	  	chksum = 0;
	  	i = 0;
	  	// intial zeroing and wait
	  	data[i] = 0; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		//first pulse (usually pi/2)
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseFirstLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		//first hold time
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + holdEndsLength - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// loop over all but last pi pulses
		for( j=0; j < pars[5]-1; ++j ){
			// pulse
			data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + pulsePiLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
			// not pulse
			data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + holdPiLength - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		}
		// final pi pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulsePiLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// end wait time
		data[i] = data[i-2] + 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + holdEndsLength - 1; data[i+1] = 0;chksum += (data[i]+data[i+1]); i+=2;
		// final pi/2 pulse
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseLastLength - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;

	  	ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

	  	// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulsePiLength  < 1) || (pulseFirstLength <1) || (pulseLastLength <1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if ((holdPiLength < 1) || (holdEndsLength <1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else if (pars[1] == 9) // Inverted (amplitude) N Pi-pulse Ramsey sequence (N spin reversals)
	{ // pars[2] is the Rabi-Period
	  // pars[3] is the initial pulse's angle in units of Pi
	  // pars[4] is the holdtime between the first pulse and the first Pi-pulse in us
	  // pars[5] is the number of Pi pulses
	  // pars[6] is the time that the last pulse should finish

	  	pulsePiLength = (int) (pars[2]/2);
	  	pulseFirstLength = (int) (pars[3]*pars[2]/2);
	  	pulseLastLength = (int) (pars[2]/4);
	  	holdPiLength = (int) (2*pars[4]);
	  	holdEndsLength = (int) (pars[4]);
	  	startTime = (int) (pars[6] - pars[5]*pulsePiLength - (pars[5]-1)*holdPiLength - 2*holdEndsLength-pulseFirstLength - pulseLastLength);

	  	ampl = 2047;

		printf("Start time in samples: %i  Must be larger than 0.\n",startTime);

	  	chksum = 0;
	  	i = 0;
	  	// intial zeroing and wait
	  	data[i] = 0; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + startTime - 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;
		//first pulse (usually pi/2)
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseFirstLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		//first hold time
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + holdEndsLength - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// loop over all but last pi pulses
		for( j=0; j < pars[5]-1; ++j ){
			// pulse
			data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + pulsePiLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
			// not pulse
			data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
			data[i] = data[i-2] + holdPiLength - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		}
		// final pi pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulsePiLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// end wait time
		data[i] = data[i-2] + 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + holdEndsLength - 1; data[i+1] = ampl;chksum += (data[i]+data[i+1]); i+=2;
		// final pi/2 pulse
		data[i] = data[i-2] + 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		data[i] = data[i-2] + pulseLastLength - 1; data[i+1] = 0; chksum += (data[i]+data[i+1]); i+=2;
		// final zeroing
		data[i] = data[i-2] + 1; data[i+1] = ampl; chksum += (data[i]+data[i+1]); i+=2;

	  	ndata = i;
		data[ndata] = chksum;
		*wavfchk = 1; // waveform has been built

	  	// Sanity checks (build waveform always, but return wavfchk = -1 if sanity checks fail)
		// Could do these checks by passing "data" to a function and having the function check for consistency.
		if (startTime < 1) { *wavfchk = -1; printf("Ramsey waveform failure: negative start delay."); }
		if ((pulsePiLength  < 1) || (pulseFirstLength <1) || (pulseLastLength <1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative pulse length."); }
		if ((holdPiLength < 1) || (holdEndsLength <1)) { *wavfchk = -1; printf("Ramsey waveform failure: negative hold time."); }
		if (data[ndata-2] > 16225) { *wavfchk = -1; printf("Ramsey waveform failure: last point too long after intial point."); }// > 16299 strictly

	}
	else
	{

	}

	return ndata;

}



// Keep for now but superseeded by SendVISAWaveform();
// Send an arbitrary waveform to a GPIB device (currently SRS only)
void SendGPIBWaveform(int device, double *pars)
{ // assuming that pars[0] is the device type, pars[1] is the waveform type

	short data[10000];// short should be 16bit, SRS is 16bit machine so in docs the SRS int is the same as a short on a 32bit machine.
	int ndata;
	char cmd[1024];
	int wavfchk;

	// initialize flag; send only if set >0
	wavfchk = -1;

	if (pars[0] == 0)
	{ // device is an SRS DS345
		ndata = BuildSRSWaveform(data, pars, &wavfchk); // ndata/2 is number of vertices
		if (wavfchk > 0)
		{
			sprintf(cmd,"LDWF?1,%d\n",ndata/2);
			ibwrt(device,cmd,strlen(cmd));
			ibrd(device,cmd,40);
			ibwrt(device,(char *)data,(int)(2*ndata+2));
			DEBUG_NUMBER_IBWRT_CALLS = DEBUG_NUMBER_IBWRT_CALLS + 2;
			printf("DEBUG_NUMBER_IBWRT_CALLS: %i\n", DEBUG_NUMBER_IBWRT_CALLS);

		}
		else
		{
			printf("Failure generating arbitrary waveform; doing nothing.");
		}

		//printf("%i\n",sizeof(int));

	}
	else if (pars[0] == 1)
	{
		printf("Too lazy to generate arbitrary waveform; doing nothing.");
	}

}


// Apply changes upon switching to a different device
int CVICALLBACK DEVICENO_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	ApplyGPIBPanel();
	SetGPIBPanel();

	return 0;
}
