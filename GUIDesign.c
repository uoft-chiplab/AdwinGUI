
#include "windows.h"

#include <visa.h>
#include <visatype.h>
#include <gpib.h>
#include "toolbox.h"
#include "ScanTableLoader.h"
#include "Adwin.h"

//To DO:  add more DDS 'clips' for copy/paste routines

//2006
//March 9:  Reorder the routines to more closely match the order in which they are executed.
//          Applies to the 'engine' but not the cosmetic/table handling routnes



#include <utility.h>
#include <string.h>
#include "Scan.h"
#include <userint.h>
#include "GUIDesign.h"
#include "GUIDesign2.h"
#include "AnalogSettings.h"
#include "AnalogSettings2.h"
#include "DigitalSettings2.h"
#include "LaserSettings2.h"
#include "LaserControl2.h"
#include "AnritsuControl2.h"
#include "AnritsuSettings2.h"
#include "LaserTuning.h"
#include "Processor.h"
#include "GPIB_SRS_SETUP.h"
#include "GPIB_SRS_SETUP2.h"
#include "vars.h"
#include "formatio.h"

#include "saveload.h"
#include "multiscan.h"


//Clipboard for copy/paste functions
double TimeClip;
int ClipColumn=-1;
struct AnalogTableClip{
	int		fcn;		//fcn is an integer refering to a function to use.
						// 0-step, 1-linear, 2- exp, 3- 'S' curve, 4 sine wave
	double 	fval;		//the final value
	double	tscale;		//the timescale to approach final value
	} AnalogClip[NUMBERANALOGCHANNELS+1];
int DigClip[NUMBERDIGITALCHANNELS+1];
ddsoptions_struct ddsclip,dds2clip,dds3clip;

struct LaserTableClip{
	int fcn;
	double fval;
}LaserClip[NUMBERLASERS+1];


extern int Active_DDS_PANEL;

// Needed some prototypes. Something is fishy about this. Not bothering right now, though (S. Trotzky, 2012-10-07)




//***************************************************************************************************
//Executed when the "RUN" button is pressed.
//Disables scanning capability, saves the GUI  in C:/LastGui.pan    Just in case of a crash
//Activates the TIMER if necessary.
//Starts the routine to generate new data for ADwin
int CVICALLBACK CMD_RUN_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int repeat=0;

	switch (event)
		{
		case EVENT_COMMIT:
			PScan.Scan_Active=FALSE;
			MultiScan.Active = FALSE;
			MultiScan.Done = FALSE;
			SaveLastGuiSettings();
			ChangedVals = TRUE;	  // Forces the BuildUpdateList() routine to generate new data for the ADwin
			scancount=0;
			GetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,&repeat);  // reads the state of the "repeat" switch
			if(repeat==TRUE)
			{
				SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_ENABLED, 1);
				//activate timer:  calls TIMER_CALLBACK to restart the RunOnce commands after a set time.
			}
			else
			{
				SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_ENABLED, 0);
				//deactivate timer
			}
			RunOnce();		// starts the routine to build the ADwin data.
			break;
		}

	return 0;
}

//*********************************************************************************
//pretty much a copy of the CMD_RUN routine, but activates the SCAN flag and resets the scan counter
// could be integrated into the CMD_RUN routine.... but this works
int CVICALLBACK CMD_SCAN_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int repeat=0,status;
	switch (event)
		{
		case EVENT_COMMIT:
			if (parameterscanmode == 0)
			{	// Multi-parameter scan (leaving old code untouched in else statement - 2012-10-07)

				status = SetupScanFiles(16, MultiScan.CommandsFilePath);// 1 on success, -1 otherwise
				if( status == 1 ){
					MultiScan.NextCommandsFileNumber = 0;// For GetNewMultiScanCommands()
					MultiScan.CurrentStep = 0;// For GetNewMultiScanCommands()
					GetNewMultiScanCommands();

					UpdateMultiScanValues(TRUE);
					SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);

					// Write the info file for the first time.
					writeToScanInfoFile();
				}
			}
			else
			{   // Not touching for now ... if everything works fine, then all this can be changed into a
				// switch question ... I guess that will never happen ...
 				if (TwoParam)
				status = ConfirmPopup ("Confirm Scan", "Confirm: Begin Experiment using 2 Parameter Scan?");
				else
				status = ConfirmPopup ("Confirm Scan", "Confirm: Begin Experiment using 1 Parameter Scan?");
				if (status==1)
				{
					UpdateScanValue(TRUE); // sending value of 1 resets the scan counter.
					PScan.Scan_Active=TRUE;
				}
			}

			// In case the start of the scan was confirmed (common to either scanmode)
			if (status == 1)
			{
				printf("status==1\n");
				SaveLastGuiSettings();
				ChangedVals = TRUE;
				repeat=TRUE;
				SetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,repeat); 		//sets "repeat" button to active
				SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_ENABLED, 1);   //Turn on the timer
				RunOnce();		// starts the routine to build the ADwin data.
			}
			break;
		}

	return 0;
}




//*********************************************************************************************************
//TIMER is a CVI object.  Whenever its countdown reaches 0, it executes the following code.
//TIMER is activated in the -CMD_RUN, CMD_SCAN and BUILDUPDATELIST routines.
//it gets de-activated in this routine, and when we hit CMD_STOP
int CVICALLBACK TIMER_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int runafterscan, repeat;
	switch (event)
		{
		case EVENT_TIMER_TICK:
			SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_ENABLED, FALSE);
			//disable timer and re-enable in the update list loop, if the repeat butn is pressed.
			//reset the timer too and set a timer time of 50ms?

			if (parameterscanmode == 0)
			{	// Multi-parameter scan (leaving old code untouched in else statement - 2012-10-07)
				if (MultiScan.Active)
				{   // Update parameter values if scan is active and run cycle afterwards.
					if (!MultiScan.Done)
					{
						// Look for and read next commands_00000.txt file
						GetNewMultiScanCommands();
						// Advance to new set of values in scan.
						// (sets MultiScan.Done to TRUE if done)
						UpdateMultiScanValues(FALSE);
						writeToScanInfoFile();
						SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED2, MultiScan.Done);
						if (MultiScan.Done)
							SetCtrlAttribute(panelHandle,PANEL_MULTISCAN_DECORATION,
									ATTR_FRAME_COLOR,CLR_SCAN_ANALOG);
						else
							SetCtrlAttribute(panelHandle,PANEL_MULTISCAN_DECORATION,
									ATTR_FRAME_COLOR,VAL_TRANSPARENT);
					}
					if (MultiScan.Done)
					{	// Check whether run should stop after finished scan.
						GetCtrlVal(panelHandle,PANEL_SCAN_KEEPRUNNING_CHK,&runafterscan);
						if(!runafterscan)
						{   // Stop cycle after scan is done and save scan parameters ...
							SetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,FALSE);
							MultiScan.Active = FALSE;
							MultiScan.Done = FALSE;
							AutoExportMultiScanBuffer(); // save to file
							EnableScanControls();
							SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
							SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED2, MultiScan.Done);
							SetCtrlAttribute(panelHandle,PANEL_MULTISCAN_DECORATION,
									ATTR_FRAME_COLOR,VAL_TRANSPARENT);
							DrawNewTable(TRUE);
						}
						else
						{   // .. or keep going if requested
							// (repeat button stays ON and scan is saved upon pressing STOP)
							RunOnce();
						}
					}
					else
					{   // Keep going with the scan if not done.
						RunOnce();
					}
				}
				else
				{
					// Run cycle if timer was enabled and the scan is not active (normal operation)
					RunOnce();
				}
			}
			else
			{   // Not touching for now ... if everything works fine, then all this can be changed into a
				// switch question ... I guess that will never happen ...
				if(PScan.Scan_Active==TRUE)
				{
					UpdateScanValue(FALSE);
				}
				if(PScan.ScanDone==FALSE||PScan.Scan_Active==FALSE)
				{
					RunOnce();
				}
			}

			break;
		}
	return 0;
}

//*********************************************************************************************************
//By: Stefan Myrskog
//Turns off the TIMER object, and turns off the "repeat" button
//Lets the ADwin finish its current program.  Interrupting the program partway can be bad for the
//equipment as the variables are not cleared in memory and updates get out of sync.
//Edited: Aug8, 2005
//Change:  Force all panel controls related to scanning to hide.
int CVICALLBACK CMDSTOP_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int j;

	SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,0);	 // hide the SCAN display panel
	switch (event)
		{
		case EVENT_COMMIT:
			// Stop timer and switch repeat off.
			SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_ENABLED, 0);
			SetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,0);

			SetCtrlVal(panelHandle, PANEL_LED_RED, 0); // Build-indicator LEDs
			SetCtrlVal(panelHandle, PANEL_LED_YEL, 0);
			SetCtrlVal(panelHandle, PANEL_LED_GRE, 0);

			//check to see if we need to export a Scan history
			if (parameterscanmode == 0)
			{	// Multi-parameter scan (leaving old code untouched in else statement - 2012-10-07)
				if (MultiScan.Active)
				{
					// remove highlighting from scan list
					for (j=0;j<MultiScan.NumPars;j++)
					{
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(1,ScanVal.Current_Step+1), ATTR_TEXT_BGCOLOR,VAL_WHITE);
					}
					AutoExportMultiScanBuffer();

					// Spoof the Scan actually have finished and call update to put values back.
					MultiScan.Done = TRUE;
					UpdateMultiScanValues(FALSE);

					MultiScan.Active = FALSE;
					MultiScan.Done = FALSE;
					SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
					SetCtrlVal (panelHandle, PANEL_MULTISCAN_LED2, MultiScan.Done);
					SetCtrlAttribute(panelHandle,PANEL_MULTISCAN_DECORATION,
							ATTR_FRAME_COLOR,VAL_TRANSPARENT);

				}
				EnableScanControls();
			}
			else
			{   // Not touching for now ... if everything works fine, than all this can be changed into a
				// switch question ... I guess that will never happen ...
				if(PScan.Scan_Active==TRUE)
				{
					SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step+1), ATTR_TEXT_BGCOLOR,
							   VAL_WHITE);

			    	if(TwoParam)
			    	{
			    		SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step+1), ATTR_TEXT_BGCOLOR,
							   VAL_WHITE);
			    		ExportScan2Buffer();
			    	}
			    	else
			    		ExportScanBuffer();

				}
			}
			DrawNewTable(TRUE);
			break;
		}
	viClose(VIsess);
	VIsess = -1;
	return 0;
}


int CVICALLBACK VALS_CHANGED_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2){

	int test;
	switch( event ){
		case EVENT_COMMIT:
			ChangedVals = TRUE;
			SetCtrlAttribute(panelHandle,
								PANEL_VALS_CHANGED,
								ATTR_CMD_BUTTON_COLOR,
								CLR_CHANGED_BUTTON_CLICKED);
			break;
	}
	return 0;
}

//**************************************************************************************************
void RunOnce (void)
//Converts the 10 'pages' (3D array) into single 2D array 'metatables'
//Ignores dimmed out columns and pages
{

	static double MetaTimeArray[NUMBEROFMETACOLUMNS];
	static int MetaDigitalArray[NUMBERDIGITALCHANNELS+1][NUMBEROFMETACOLUMNS];
	static struct AnVals MetaAnalogArray[NUMBERANALOGCHANNELS+1][NUMBEROFMETACOLUMNS];
	static ddsoptions_struct MetaDDSArray[NUMBEROFMETACOLUMNS];
	static dds2options_struct MetaDDS2Array[NUMBEROFMETACOLUMNS];
	static dds3options_struct MetaDDS3Array[NUMBEROFMETACOLUMNS];
	static struct LaserTableValues	MetaLaserArray[NUMBERLASERS][NUMBEROFMETACOLUMNS];
	static unsigned int MetaTriggerArray[NUMBERLASERS][NUMBEROFMETACOLUMNS];
	static char errorBuff[200],write_buffer[200];

	static int tsize;// Number of columns or alternatively the number of time steps.

	int i,j,k,mindex;
	int insertpage,insertcolumn,x,y,lastpagenum,FinalPageToUse;
	int device;
	BOOL nozerofound,nozerofound_2;

	static int forceBuild = 0;

	double timeStart, timeEnd;
	double timeStartBLU, timeEndBLU;
	double timeStartDraw, timeEndDraw;

	timeStart = Timer();


	GetCtrlVal(panelHandle, PANEL_FORCE_BUILD_CHK, &forceBuild);

	// Reset the colour of the "Changed" button.
	SetCtrlAttribute(panelHandle,
						PANEL_VALS_CHANGED,
						ATTR_CMD_BUTTON_COLOR,
						CLR_CHANGED_BUTTON_DEFAULT);


	// SetCtrlVal(panelHandle, PANEL_LED_RED, 1); // Build-indicator LEDs
	// SetCtrlVal(panelHandle, PANEL_LED_YEL, 0);
	// SetCtrlVal(panelHandle, PANEL_LED_GRE, 0);


	SeqErrorCount=0;  // For each run reset error count to zero


	isdimmed=TRUE;
	lastpagenum=10;


	// V16.1.6: If nothing needs to be changed then bypass building MetaTimeArray and friends.
	if( ChangedVals == TRUE || forceBuild == TRUE ){// ending bracket not indented

	// Find the position of the imaging panel (last panel on the GUI), this panel can be positioned inside other panels
	GetCtrlVal (panelHandle, PANEL_NUM_INSERTIONPAGE, &insertpage);
	GetCtrlVal (panelHandle, PANEL_NUM_INSERTIONCOL, &insertcolumn);

	//Lets build the times list first...so we know how long it will be.
	//check each page...find columns in use and dim out unused....(with 0 or negative time values)
	SetCtrlAttribute(panelHandle,PANEL_ANALOGTABLE,ATTR_TABLE_MODE,VAL_COLUMN);
	mindex=0;
	FinalPageToUse=NUMBEROFPAGES-2;//issues with '0 or 1 indexing'.  If there are 10 pages, we end at 9.
	//Page 10(imaging) handled separately.

	if(insertpage==NUMBEROFPAGES-1) //if the imaging page is set for 10, then set the final page as 10.
	{
		FinalPageToUse++;
	}
	//go through for each page
	for(k=1;k<=FinalPageToUse;k++)// numberofpages-1 because last page is 'mobile'
	{
		nozerofound=TRUE;
		if(ischecked[k]==1) //if the page is selected (checkbox is checked)
		{
			//go through for each time column
			for(i=1;i<=NUMBEROFCOLUMNS;i++)
			{
				if((i==insertcolumn)&&(k==insertpage)&&(k!=NUMBEROFPAGES-1))  //NUMBEROFPAGES-1 is 10 ie the insert page itself
				{
					nozerofound_2=TRUE;
					if(ischecked[lastpagenum]==1)
					{
						for(x=1;x<=NUMBEROFCOLUMNS;x++)
						{
							if((nozerofound==TRUE)&&(nozerofound_2==TRUE)&&(TimeArray[x][lastpagenum]>0))
							{
								mindex++;
								MetaTimeArray[mindex]=TimeArray[x][lastpagenum];
								for(y=1;y<=NUMBERANALOGCHANNELS;y++)
								{
									//Sets MetaArray with appropriate fcn val when "same" selected
									if(AnalogTable[x][y][lastpagenum].fcn==6)
									{
										MetaAnalogArray[y][mindex].fcn=1; ///Use code on right to mirror prev fncn MetaAnalogArray[y][mindex].fcn=MetaAnalogArray[y][mindex-1].fcn;
										MetaAnalogArray[y][mindex].fval=MetaAnalogArray[y][mindex-1].fval;
										MetaAnalogArray[y][mindex].tscale=MetaAnalogArray[y][mindex-1].tscale;

									}
									else if(AnalogTable[x][y][lastpagenum].fcn!=6)
									{
										MetaAnalogArray[y][mindex].fcn=AnalogTable[x][y][lastpagenum].fcn;
										MetaAnalogArray[y][mindex].fval=AnalogTable[x][y][lastpagenum].fval;
										MetaAnalogArray[y][mindex].tscale=AnalogTable[x][y][lastpagenum].tscale;
									}
								}

								for(y=0;y<NUMBERLASERS;y++)
								{
									if(LaserTable[y][x][lastpagenum].fcn!=0)
										MetaLaserArray[y][mindex]=LaserTable[y][x][lastpagenum];
									else
										MetaLaserArray[y][mindex]=MetaLaserArray[y][mindex-1];
								}

								for(y=1;y<=NUMBERDIGITALCHANNELS;y++)
								{
									MetaDigitalArray[y][mindex]=DigTableValues[x][y][lastpagenum];
								}
								MetaDDSArray[mindex] = ddstable[x][lastpagenum];
								MetaDDS2Array[mindex] = dds2table[x][lastpagenum];
								MetaDDS3Array[mindex] = dds3table[x][lastpagenum];
							}
							else if(TimeArray[x][lastpagenum]==0)
							{
								nozerofound_2=0;
							}
						}
					}

/*end A */		}

				if((nozerofound==1)&&(TimeArray[i][k]>0))
				//ignore all columns after the first
				// time 0 (for that page)
				{
					mindex++; //increase the number of columns counter
					MetaTimeArray[mindex]=TimeArray[i][k];

					//go through for each analog channel
					for(j=1;j<=NUMBERANALOGCHANNELS;j++)
					{
						//Sets MetaArray with appropriate fcn val when "same" selected
						if(AnalogTable[i][j][k].fcn==6)
						{
							MetaAnalogArray[j][mindex].fcn=1; //Use code on right to mirror prev fncn MetaAnalogArray[j][mindex].fcn=MetaAnalogArray[j][mindex-1].fcn;
							MetaAnalogArray[j][mindex].fval=MetaAnalogArray[j][mindex-1].fval;
							MetaAnalogArray[j][mindex].tscale=MetaAnalogArray[j][mindex-1].tscale;
						}
						else if (AnalogTable[i][j][k].fcn!=6)
						{
							MetaAnalogArray[j][mindex].fcn=AnalogTable[i][j][k].fcn;
							MetaAnalogArray[j][mindex].fval=AnalogTable[i][j][k].fval;
							MetaAnalogArray[j][mindex].tscale=AnalogTable[i][j][k].tscale;
						}
					}

					for(j=0;j<NUMBERLASERS;j++)
					{
						if(LaserTable[j][i][k].fcn!=0)
							MetaLaserArray[j][mindex]=LaserTable[j][i][k];
						else
							MetaLaserArray[j][mindex]=MetaLaserArray[j][mindex-1];
					}


					for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
					{
						MetaDigitalArray[j][mindex]=DigTableValues[i][j][k];
					}
					/* ddsoptions_struct contains floats and ints, so shallow copy is ok */
					MetaDDSArray[mindex] = ddstable[i][k];
					MetaDDS2Array[mindex] = dds2table[i][k];
					MetaDDS3Array[mindex] = dds3table[i][k];
					MetaDDSArray[mindex].delta_time=TimeArray[i][k]/1000;
					MetaDDS2Array[mindex].delta_time=TimeArray[i][k]/1000;
					MetaDDS3Array[mindex].delta_time=TimeArray[i][k]/1000;
					// Don't know why the preceding line is necessary...
					// But the time information wasn't always copied in... or it was erased elsewhere
				}
				else if (TimeArray[i][k]==0)
				{
					nozerofound = FALSE;
				}
			}
		}
	}
	tsize = mindex; //tsize is the number of columns


	}// end if( ChangedVals == TRUE || forceBuild == TRUE )


	isdimmed=TRUE;


	timeStartDraw = Timer();
	DrawNewTable(TRUE);
	timeEndDraw = Timer();

	// Programs Rabbit
	timeStartBLU = Timer();
	BuildLaserUpdates(MetaLaserArray, MetaTimeArray, MetaTriggerArray, tsize, forceBuild);
	timeEndBLU = Timer();


    // Send the new arrays to BuildUpdateList()
    if (SeqErrorCount>0)
    {
			sprintf(errorBuff,"Bad Setting in Sequence.\nView %d Error[s]?",SeqErrorCount);
			if(ConfirmPopup ("Sequence Setting Error",errorBuff))
			{
				for(j=0;j<SeqErrorCount;j++)
				{
					sprintf(errorBuff,"Error %d",j);
					MessagePopup (errorBuff, SeqErrorBuffer[j]);
				}

			}
			if (ConfirmPopup("Error Directive","Execute Sequence Anyways?\n(not recommened)"))
			{
				BuildUpdateList(MetaTimeArray,MetaAnalogArray,MetaDigitalArray,MetaDDSArray,MetaDDS2Array,MetaDDS3Array,MetaTriggerArray,tsize,forceBuild);
			}
	}
	else
	{
		BuildUpdateList(MetaTimeArray,MetaAnalogArray,MetaDigitalArray,MetaDDSArray,MetaDDS2Array,MetaDDS3Array,MetaTriggerArray,tsize,forceBuild);
	}

	//And communicate with the microwave source
    // if the ON button is flicked
    if (AnritsuSettingValues[0].com_on == 1)
    {
    	AnritsuCOMMUNICATE ();
    }

    // Program GPIB devices if communication set to active and news are to be told.
    for (j=0;j<=NUMBERGPIBDEV;j++)
    {   // check whether communication is set to active
    	if (GPIBDev[j].active)
    	{   // only update if new command differs from last command sent
    		BuildGPIBString(j+1,GPIBDev[j].command);
    		if (strcmp(GPIBDev[j].command,GPIBDev[j].lastsent) != 0)
    		{
    			SendGPIBString(j+1);
    		}
    	}
    }


	timeEnd = Timer();
	//printf("RunOnce: EndDraw-StartDraw: %f seconds.\n", timeEndDraw-timeStartDraw);
	//printf("RunOnce: EndBLU-StartBLU: %f seconds.\n", timeEndBLU-timeStartBLU);
	//printf("RunOnce: End-Start: %f seconds.\n", timeEnd-timeStart);

}
//*****************************************************************************************
void BuildUpdateList(double TMatrix[],
					 struct AnVals AMat[NUMBERANALOGCHANNELS+1][500],
					 int DMat[NUMBERDIGITALCHANNELS+1][500],
					 ddsoptions_struct DDSArray[500],
					 dds2options_struct DDS2Array[500],
					 dds3options_struct DDS3Array[500],
					 unsigned int LaserTriggerArray[NUMBERLASERS][500],
					 int numtimes,
					 int forceBuild){
	/*

	TMatrix[update period#] -- stores the interval time of each column
	AMat[channel#][update period#] -- stores info located in the analog table
	DMat[channel#][update period#] --
	DDS -- note is_stop=1 means DDS OFF

	all the above have 500 update period elements note that valid elements are base1

	numtimes = the actual number of valid update period elements.


	Generate the data that is sent to the ADwin and sends the data.
	From the meta-lists, we generate 3 arrays.
	UpdateNum - each entry is the number of channel updates that we perform during the ADwin EVENT, where an
				ADwin event is an update cycle, i.e. 10 microseconds, 100 microseconds... etc.  We advance  through this
				array once per ADwin Event.  UpdateNum controls how fast we scan through ChNum and ChVal
	ChNum - 	An array that contains the channel number to be updated. Synchronous with ChVal.  	   Channels listed below
	ChVal -		An array that contains the value to be written to a channel. Synchronous with ChVal.

	ChNum -     Value 1-32:  Analog lines, 4 cards with 8 lines each.  ChVal is -10V to 10V
				Value 51:	 DDS1 line.   ChVal is either a 2-bit value (0-3) to write, or (4-7) a reset signal
				Value 52:	 DDS2 line.   ChVal is either a 2-bit value (0-3) to write, or (4-7) a reset signal
				Value 101, 102  First 16 and last 16 lines on the first DIO card.  ChVal is a 16 bit integer
				Value 103, 104	First 16 and last 16 lines on the second DIO card. ChVal is a 16 bit integer

    Mar 09_2006:Added ChNum 201,202   These  are codes to enable/disable looping.
                Corresponding ChVal is the number of loops.
	dds_cmd_seq List of dds commands, parsed into 2-bit sections, or reset lines to be written
				Commands are listed along with the time they should occur at.
	*/






	BOOL UseCompression,ArraysToDebug,StreamSettings;

	FILE *fp;
	int NewTimeMat[500];
	int i=0,j=0,k=0,m=0,n=0,tau=0,p=0,imax;
	int nupcurrent=0,nuptotal=0,checkresettozero=0;
	int usefcn=0,digchannel;  //Bool
	unsigned int digval,digval2,digval3,digval4,LastDVal=0,LastDVal2=0,LastDVal3=0,LastDVal4=0,lasTrigVal=0;
	int UsingFcns[NUMBERANALOGCHANNELS+1]={0},count=0,ucounter=0,counter,channel;
	double LastAval[NUMBERANALOGCHANNELS+1]={0},NewAval,TempChVal,TempChVal2;
	int ResetToZeroAtEnd[110]; //1-24 for analog, ...but for now, if [1]=1 then all zero, else no change
	float ResetToZeroAtEndDig[3]; //1-24 for analog, ...but for now, if [1]=1 then all zero, else no change
	int timemult,t,c,bigger;
	double cycletime=0;
	int GlobalDelay=40000;
	char buff[100];
	int repeat=0;
	int temp_timesum = 0;
	static int didboot=0;
	static int didprocess=0;
	int memused;
	int timeused;
	int tmp_dds;
	dds_cmds_ptr dds_cmd_seq = NULL;
	dds_cmds_ptr dds_cmd_seq_AD9858 = NULL;
	double DDSoffset=0.0,DDS2offset=0.0,DDS3offset=0.0;
	int digchannelsum;
	int newcount=0;
	// variables for timechannel optimization
	int ZeroThreshold=50;
	int lastfound=0;
	int ii=0,jj=0,kk=0,tt=0; // variables for loops
	int start_offset=0;
	time_t tstart,tstop;
	double timestamp, timestamp1,timestamp2,timestamp3;
	int size1, size2;
	int runafterscan;

	// V16.1.6: Make UpdateNum, ChNum, and ChVal static pointers.
	//			UpdateNumInitialized is 0 if UpdateNum etc. do not point to memory locations.
	static int UpdateNumInitialized = 0;
	static int *UpdateNum = NULL;
	static int *ChNum = NULL;
	static float *ChVal = NULL;
	static int timesum = 0;




	double timeStart, timeEnd;
	double timeStartProcess, timeEndProcess;

	timeStart = Timer();



	//Change run button appearance while operating
	SetCtrlAttribute (panelHandle, PANEL_CMD_RUN,ATTR_CMD_BUTTON_COLOR, VAL_GREEN);

	// SetCtrlVal(panelHandle, PANEL_LED_RED, 0); // Build-indicator LEDs
	// SetCtrlVal(panelHandle, PANEL_LED_YEL, 1);
	// SetCtrlVal(panelHandle, PANEL_LED_GRE, 0);

   	tstart=clock();				   // Timing information for debugging purposes
	timemult=(int)(1/EventPeriod); //number of adwin upates per ms

	if(processorT1x==0)
	{
		AdwinTick = 0.000025;
	}
	else if(processorT1x==1)
	{
		AdwinTick = 0.00001/3;
	}

	GlobalDelay=EventPeriod/AdwinTick; // AdwintTick=0.000025ms=AW clock cycle (Gives #of clock cycles/update)




	// V16.1.6: Check each time if UpdateNum, ChNum, and ChVal need to be extended.

	//make a new time list...converting the TimeTable from milliseconds to number of events (numtimes=total #of columns)
	temp_timesum = 0;
	for (i=1;i<=numtimes;i++)
	{
		NewTimeMat[i]=(int)(TMatrix[i]*timemult); //number of Adwin events in column i
		temp_timesum += NewTimeMat[i];            //total number of Adwin events
	}
	if( temp_timesum > timesum ){

		ChangedVals = TRUE;// Redundant since timesum has changed;
		//						but we MUST build UpdateNum again since we free it here.

		free(UpdateNum);
		UpdateNum = calloc((int)((double)temp_timesum*1.2),sizeof(int));
		if( UpdateNum == NULL ){ exit(1); }

		free(ChNum);
		ChNum = calloc((int)((double)temp_timesum*4),sizeof(int));
		if( ChNum == NULL ){ exit(1); }

		free(ChVal);
		ChVal = calloc((int)((double)temp_timesum*4),sizeof(double));
		if( ChVal == NULL ){ exit(1); }

		UpdateNumInitialized = 1;
	}
	if( !UpdateNumInitialized ){// Just in case.

		UpdateNum = calloc((int)((double)temp_timesum*1.2),sizeof(int));
		if( UpdateNum == NULL ){ exit(1); }

		ChNum=calloc((int)((double)temp_timesum*4),sizeof(int));
		if( ChNum == NULL ){ exit(1); }

		ChVal=calloc((int)((double)temp_timesum*4),sizeof(double));
		if( ChVal == NULL ){ exit(1); }

		UpdateNumInitialized = 1;
	}
	timesum = temp_timesum;





	cycletime=(double)timesum/(double)timemult/1000;	// Total duration of the cycle, in seconds

	sprintf(buff,"timesum %d",timesum);					// Print more debug info

	// Update the ADWIN array if the user values have changed or we are requiring a rebuild.
	if( ChangedVals == TRUE || forceBuild == TRUE){

    	timestamp1 = Timer();

    	/* Update the array of DDS commands
		EventPeriod is in ms, create_command_array in s, so convert units */
		GetMenuBarAttribute (menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, &UseSimpleTiming);
		GetCtrlVal (panelHandle, PANEL_NUM_DDS_OFFSET, &DDSoffset);
		GetCtrlVal (panelHandle, PANEL_NUM_DDS2_OFFSET, &DDS2offset);
		GetCtrlVal (panelHandle, PANEL_NUM_DDS3_OFFSET, &DDS3offset);
		GetCtrlVal (panelHandle, PANEL_ANRITSU_OFFSET, &AnritsuSettingValues[0].offset);
		// read offsets to add to DDSArray
		for(m=1;m<=numtimes;m++)
		{
			DDSArray[m].start_frequency=DDSArray[m].start_frequency+DDSoffset;
			DDSArray[m].end_frequency=DDSArray[m].end_frequency+DDSoffset;
		// uncomment these as needed to run DDS2,3
			DDS2Array[m].start_frequency=DDS2Array[m].start_frequency+DDS2offset;	   // Alma uncommented these
			DDS2Array[m].end_frequency=DDS2Array[m].end_frequency+DDS2offset;
		///	DDS3Array[m].start_frequency=DDS3Array[m].start_frequency+DDS3offset;
		///	DDS3Array[m].end_frequency=DDS3Array[m].end_frequency+DDS3offset;

		}


		dds_cmd_seq = create_ad9852_cmd_sequence(DDSArray, numtimes,DDSFreq.PLLmult,
		DDSFreq.extclock,EventPeriod/1000);
		// again, uncomment as needed
   	      dds_cmd_seq_AD9858 = create_ad9858_cmd_sequence(DDS2Array, numtimes,DDS2_CLOCK,
		EventPeriod/1000,0);	   // assume frequency offset of 0 MHz

    	/*dds_cmd_seq = create_ad9852_cmd_sequence(DDS3Array, numtimes,DDSFreq.PLLmult,
		DDSFreq.extclock,EventPeriod/1000);
   	    */



		//Go through for each column that needs to be updated

		//Important Variables:
		//count: Number of Adwin events until the current position
		//nupcurrent: number of updates for the current Adwin event
		//nuptotal: current position in the channel/value column
		for (i=1;i<=numtimes;i++)
		{
			// find out how many channels need updating this round...
			// if it's a non-step fcn, then keep a list of UsingFcns, and change it now
			nupcurrent=0;
			usefcn=0;

			// scan over the analog channel..find updated values by comparing to old values.
			for (j=1;j<=NUMBERANALOGCHANNELS;j++)
			{
				LastAval[j]=-99;
				if(AMat[j][i].fval!=AMat[j][i-1].fval)
				{
					nupcurrent++;
					nuptotal++;
					ChNum[nuptotal]=AChName[j].chnum;

					NewAval=CalcFcnValue(AMat[j][i].fcn,AMat[j][i-1].fval,AMat[j][i].fval,AMat[j][i].tscale,0.0,TMatrix[i]);

 					TempChVal=AChName[j].tbias+NewAval*AChName[j].tfcn;
 					ChVal[nuptotal]=CheckIfWithinLimits(TempChVal,j);

					if(AMat[j][i].fcn!=1)
					{
						usefcn++;
						UsingFcns[usefcn]=j;	// mark these lines for special attention..more complex
					}
				}
			}//done scanning the analog values.
			//*********now the digital value
			digval=0;
			digval2=0;
			digval3=0;
			digval4=0;
			for(k=1;k<=NUMBERDIGITALCHANNELS;k++)
			{
				digchannel=DChName[k].chnum;

				if(digchannel<=32)
				{
					digval=digval+DMat[k][i]*int_power(2,DChName[k].chnum-1);
				}

				if((digchannel>=101)&&(digchannel<=132))
				{
					digval2=digval2+DMat[k][i]*int_power(2,(DChName[k].chnum-100)-1);
				}

			}// finished computing current digital data

			if(digval!=LastDVal)
			{
				nupcurrent++;
				nuptotal++;
				ChNum[nuptotal]=101;
				ChVal[nuptotal]=digval;
			}
			LastDVal=digval;

			/*** Check if any Lasers need triggering */
			lasTrigVal=0;
			for(k=0;k<NUMBERLASERS;k++)
			{
				if(LaserTriggerArray[k][i]>0)
				{
					lasTrigVal+=int_power(2,(LaserProperties[k].DigitalChannel-100)-1);
				}
			}

			if(digval2!=LastDVal2||lasTrigVal>0)
			{
				nupcurrent++;
				nuptotal++;
				ChNum[nuptotal]=102;
				ChVal[nuptotal]=digval2+lasTrigVal;
				LastDVal2=digval2;
			}


/*			if(!(digval3==LastDVal3))
			{
				nupcurrent++;
				nuptotal++;
				ChNum[nuptotal]=103;
				ChVal[nuptotal]=digval3;
			}
			LastDVal3=digval3;
			if(!(digval4==LastDVal4))
			{
				nupcurrent++;
				nuptotal++;
				ChNum[nuptotal]=104;
				ChVal[nuptotal]=digval4;
			}
			LastDVal4=digval4;
*/

			count++;
			UpdateNum[count]=nupcurrent;

			//end of first scan
			//now do the remainder of the loop...but just the complicated fcns, i.e. ramps, sine wave

			t=0;
			while(t<NewTimeMat[i]-1)	   //-1 because the first event is dedicated to steps+digital chans
			{
				t++;
				k=0;
				nupcurrent=0;

				// If there were laser triggers we must set them low
				if(lasTrigVal>0)
				{
					nupcurrent++;
					nuptotal++;
					ChNum[nuptotal]=102;
					ChVal[nuptotal]=LastDVal2;
					lasTrigVal=0;
				}

				//look for a new DDS command, start_offset=0
				tmp_dds = get_dds_cmd(dds_cmd_seq, count-1-start_offset);  //dds translator(zero base) runs 1 behind this counter
				if (tmp_dds>=0)
				{
					nupcurrent++;
					nuptotal++;
					ChNum[nuptotal] = 51; //DDS1 dummy channel
					ChVal[nuptotal] = tmp_dds;


				}

		/*		tmp_dds = get_dds_cmd(dds_cmd_seq_AD9858, count-1-start_offset);  //dds translator(zero base) runs 1 behind this counter
				if (tmp_dds>=0)
				{
					nupcurrent++;
					nuptotal++;
					ChNum[nuptotal] = 52; //dummy channel
					ChVal[nuptotal] = tmp_dds;

				} //done the DDS2
		*/

				while(k<usefcn)
				{
					k++;
					c=UsingFcns[k];
					NewAval=CalcFcnValue(AMat[c][i].fcn,AMat[c][i-1].fval,AMat[c][i].fval,AMat[c][i].tscale,t,TMatrix[i]);
					TempChVal=AChName[c].tbias+NewAval*AChName[c].tfcn;
					TempChVal2=CheckIfWithinLimits(TempChVal,c);
					if(fabs(TempChVal2-LastAval[k])>0.0003) // only update if the ADwin will output a new value.
					// ADwin is 16 bit, +/-10 V, to 1 bit resolution implies dV=20V/2^16 ~=  0.3mV
					{
						nupcurrent++;
						nuptotal++;
						ChNum[nuptotal]=AChName[c].chnum;
						ChVal[nuptotal]=AChName[c].tbias+NewAval*AChName[c].tfcn;
						LastAval[k]=TempChVal2;
					}
				}

				count++;
				UpdateNum[count]=nupcurrent;

			}//Done this element of the TMatrix

		}//done scanning over times array

		//Find the largest of the arrays
//		bigger=count;
//		if(nuptotal>bigger) {bigger=nuptotal;}

		// read some menu options
		GetMenuBarAttribute (menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, &UseCompression);
		GetMenuBarAttribute (menuHandle, MENU_PREFS_SHOWARRAY, ATTR_CHECKED, &ArraysToDebug);
		newcount=0;

		if(UseCompression)
		{
			OptimizeTimeLoop(UpdateNum,count,&newcount);
		}

		if(ArraysToDebug)	  // only used if we suspect the ADwin code of being really faulty.
		{					  // ArraysToDebug is a declare in vars.h
			//imax=newcount;
			//if(newcount==0) {imax=count;}
			//if(imax>1000){imax=1000;}
			//for(i=1;i<imax+1;i++)


			// this loop just writes out the first 1000 updates to the stdio window in the format
			// i  UpdateNum    ChNum   Chval
			k=1;
			for(i=1;i<1000;i++)
			{
				printf("%-5d%d\t",i,UpdateNum[i]);
				if(UpdateNum[i]>0)
				{
					printf("%d\t%f\n",ChNum[k],ChVal[k]);
					k++;
					for(j=1;j<UpdateNum[i];j++)
					{
						printf("\t\t%d\t%f\n",ChNum[k],ChVal[k]);
						k++;
					}
				}
				else
				{
					printf("\n");
				}

			}
		}


		tstop=clock();


		if(processorT1x==0)
		{
			if(didboot==FALSE) // is the ADwin booted?  if not, then boot
			{
				Boot("C:\\ADWIN\\ADWIN10.BTL",0);
				didboot=1;
			}
			if (didprocess==FALSE) // is the ADwin process already loaded?
			{
				processnum=Load_Process("TransferData.TA1");
				didprocess=1;
			}
		}
		else if(processorT1x==1)
		{
		 	if(didboot==FALSE) // is the ADwin booted?  if not, then boot
			{
				Boot("C:\\ADWIN\\ADWIN11.BTL",0);
				didboot=1;
			}
			if (didprocess==FALSE) // is the ADwin process already loaded?
			{
				processnum=Load_Process("TransferData.TB1");
				didprocess=1;
			}
		 }

		 //printf("Processor type = %hi\n",Processor_Type());

		//Commented out by Dave June 6, 2006. Reason: repeated few lines down? (access of -1 index?)
		//for(p-1;p<=NUMBERANALOGCHANNELS;p++) {ResetToZeroAtEnd[p]=AChName[p].resettozero;}

		if(UseCompression)
		{
			SetPar(1,newcount);  	//Let ADwin know how many counts (read as Events) we will be using.
			SetData_Long(1,UpdateNum,1,newcount+1);
		}
		else
		{
		  	SetPar(1,count);  	//Let ADwin know how many counts (read as Events) we will be using.
			SetData_Long(1,UpdateNum,1,count+1);
		}


	//	printf("\nbegin at: %s \n", TimeStr());
	//	timestamp = Timer();

	//	size1 = sizeof(ChNum)*(nuptotal+1);
	//	size2 = sizeof(ChVal)*(nuptotal+1);

	// Send the Array to the AdWin Sequencer
// ------------------------------------------------------------------------------------------------------------------
		SetPar(2,GlobalDelay);
	//	timestamp2 = Timer();
		SetData_Long(2,ChNum,1,nuptotal+1);
	//	timestamp3 = Timer();
		SetData_Float(3,ChVal,1,nuptotal+1);
// ------------------------------------------------------------------------------------------------------------------

	//	 printf("end at: %s \n", TimeStr());
	//	 printf("elapsed time (calculating):  %0.3f s\n", timestamp-timestamp1);
	//	 printf("elapsed time (sending):  %0.3f s\n", Timer()-timestamp);
	//	 printf("elapsed time (sending global delay):  %0.3f s\n", timestamp2-timestamp);
	//	 printf("elapsed time (sending ChNum):  %0.3f s, size = %d\n", timestamp3-timestamp2,size1);
	//	 printf("elapsed time (sending ChVal):  %0.3f s, size = %d\n", Timer()-timestamp3,size2);
	//	 printf("elapsed time (total):  %0.3f s\n", Timer()-timestamp1);


		// determine if we should reset values to zero after a cycle
		GetMenuBarAttribute (menuHandle, MENU_SETTINGS_RESETZERO, ATTR_CHECKED,&checkresettozero);

     	for(i=1;i<=NUMBERANALOGCHANNELS;i++)
		{	ResetToZeroAtEnd[i-1]=1; } // Reset Array to ones

		for(i=1;i<=NUMBERANALOGCHANNELS;i++) // Writing resetToZero array in order of channels (fixed 2012-11-29 / was written in order of _lines_ before!)
		{
			if ((AChName[i].chnum>0) && (AChName[i].chnum<=NUMBERANALOGCHANNELS))
			{
				ResetToZeroAtEnd[AChName[i].chnum-1]=AChName[i].resettozero;
			}
			//printf("Line = %d, Channel = %d, Reset-to-zero = %d \n", i, AChName[i].chnum, ResetToZeroAtEnd[i-1]);

		}

		digval=0;
		digval2=0;

		for(k=1;k<=NUMBERDIGITALCHANNELS;k++)
			{
				digchannel=DChName[k].chnum;

				if(digchannel<=32)
				{
					digval=digval+DChName[k].resettolow*int_power(2,DChName[k].chnum-1);
				}

				if((digchannel>=101)&&(digchannel<=132))
				{
					digval2=digval2+DChName[k].resettolow*int_power(2,(DChName[k].chnum-100)-1);
				}
	//			printf("DChName[%d].chnum %d,DChName[k].resettolow %d, digval %x, digval2 %x \n",k,DChName[k].chnum,DChName[k].resettolow,digval,digval2);
			}// finished computing current digital data


		//	printf("digval %x, digval %x \n",digval,digval2);


		//ResetToZeroAtEndDig[1]=digval;// 1-32

		//ResetToZeroAtEndDig[2]=digval2;// 101-132

		//ResetToZeroAtEnd[25]=0;// lower 16 digital channels
		//ResetToZeroAtEnd[26]=0;// digital channels 17-24
//		ResetToZeroAtEnd[27]=0;// master override....if ==1 then reset none
//		if(checkresettozero==0) { ResetToZeroAtEnd[27]=1;}

		SetData_Long(4,ResetToZeroAtEnd,1,NUMBERANALOGCHANNELS);
		SetPar(5,digval);
		SetPar(6,digval2);

		// done evaluating channels that are reset to  zero (low)
		ChangedVals = FALSE;

	}









	// more debug info
	//tstop=clock();
	//timeused=tstop-tstart;

	timeStartProcess = Timer();
	t = Start_Process(processnum);
	timeEndProcess = Timer();

	//tstop=clock();
	//sprintf(buff,"Time to transfer and start ADwin:   %d",timeused);


	GetMenuBarAttribute (menuHandle, MENU_PREFS_STREAM_SETTINGS, ATTR_CHECKED,&StreamSettings);
	if(StreamSettings==TRUE)
	{
		//Fill IN

	}

	SetCtrlAttribute (panelHandle, PANEL_CMD_RUN,ATTR_CMD_BUTTON_COLOR, 0x00B0B0B0);

	// NOTE: This should better be part of the Timer Callback routine!
	//re-enable the timer if necessary
	GetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,&repeat);
	// Check if scan is over: reset repeat button
	if((PScan.Scan_Active==TRUE)&&(PScan.ScanDone==TRUE))
	{
		repeat=FALSE;
		SetCtrlVal(panelHandle,PANEL_TOGGLEREPEAT,repeat);
	}
	// Reenable and reset timer if awaiting next cycle.
	if(repeat==TRUE)
	{
		SetCtrlAttribute(panelHandle,PANEL_TIMER,ATTR_ENABLED,1);
		SetCtrlAttribute (panelHandle, PANEL_TIMER, ATTR_INTERVAL, cycletime);
		ResetTimer(panelHandle,PANEL_TIMER);
	}

//	SetCtrlVal(panelHandle, PANEL_LED_RED, 0); // Build-indicator LEDs
//	SetCtrlVal(panelHandle, PANEL_LED_YEL, 0);
//	SetCtrlVal(panelHandle, PANEL_LED_GRE, 1);


	timeEnd = Timer();
	//printf("BuildUpdateTable: EndProcess-StartProcess: %f seconds.\n", timeEndProcess-timeStartProcess);
	//printf("BuildUpdateTable: End-Start: %f seconds.\n", timeEnd-timeStart);

}

//*****************************************************************************************
double CalcFcnValue(int fcn,double Vinit,double Vfinal, double timescale,double telapsed,double celltime)
{
	double value=-99,amplitude,slope,aconst,bconst,tms,frequency,newtime;
	frequency=timescale;
	if( UseSimpleTiming==TRUE ){
		timescale=celltime-EventPeriod;
	}
	if( timescale<=0 ){
		//printf("timescale<=0\n");
		//printf("    fcn: %d\n", fcn);
		timescale=1;
	}
	tms=telapsed*EventPeriod;


	//if( fcn==3 && tms<2.0 ){
	//	printf("fcn: %d\n", fcn);
	//	printf("Vinit: %f\n", Vinit);
	//	printf("Vfinal: %f\n", Vfinal);
	//	printf("timescale: %f\n", timescale);
	//	printf("telapsed: %f\n", telapsed);
	//	printf("tms: %f\n", tms);
	//	printf("cell time: %f\n", celltime);
	//}


	switch(fcn)
	{
		case 1 ://step function
			value=Vfinal;
			break;
		case 2 ://linear ramp
			amplitude=Vfinal-Vinit;
			slope=amplitude/timescale;
			if(tms>timescale) { value=Vfinal;}
			else {value=Vinit+slope*tms;}
			break;
		case 3 ://exponential
			amplitude=Vfinal-Vinit;
			newtime =timescale;
			if(UseSimpleTiming==TRUE)
			{
				newtime=timescale/fabs(log(fabs(amplitude))-log(0.001));
			}
			value=Vfinal-amplitude*exp(-tms/newtime);
			break;
		case 4 :
			amplitude=Vfinal-Vinit;
			aconst=3*amplitude/pow(timescale,2);
			bconst=-2*amplitude/pow(timescale,3);
			if(tms>timescale)
			{
				value=Vfinal;
			}
			else
			{
				value=Vinit+(aconst*pow(tms,2)+bconst*pow(tms,3));
			}
			break;
		case 5 : // generate a sinewave.  Use Vfinal as the amplitude and timescale as the frequency
			// ignore the 'Simple Timing' option...use the user entered value.

			amplitude=Vfinal;
		//	frequency=timescale; //consider it to be Hertz (tms is time in milliseconds)
			value=amplitude * sin(2*3.14159*frequency*tms/1000);
	}
	// Check if the value exceeds the allowed voltage limits.
	return value;
}

//*****************************************************************************************
void OptimizeTimeLoop(int *UpdateNum,int count, int *newcount)
{
// This routine compresses the updatenum list by replacing long strings of 0 with a single line.
// i.e.  if we see 2000 zero's in a row, just write -2000 instead.

	int i=0,k=0; // i is the counter through the original UpdateNum list
	int j=0; // t is the counter through the NewUpdateNum list
	int t=0;
	int LowZeroThreshold,HighZeroThreshold;
	int LastFound=0;
	int numberofzeros;
	i=1;
	t=1;
	LowZeroThreshold=0;	  // minimum number of consecutive zero's to encounter before optimizing
	HighZeroThreshold=100000;  // maximum number of consecutive zero's to optimize
							//  We do not want to exceed the counter on the ADwin
							//  ADwin uses a 40MHz clock, 1 ms implies counter set to 40,000
	while( i<count+1)
	{
		if(UpdateNum[i]!=0)
		{
			UpdateNum[t]=UpdateNum[i];
			i++;
			t++;
		}
		else   							// found a 0
		{     							// now we need to scan to find the # of zeros
			j=1;
			while(((i+j)<(count+1))&&(UpdateNum[i+j]==0))
			{
				j++;
			} 						//if this fails, then AA[i+j]!=0
			if((i+j)<(count+1))
			{
				numberofzeros=j;
				if(numberofzeros<=LowZeroThreshold)
				{
					for(k=1;k<=numberofzeros;k++) { UpdateNum[t]=0;t++;i++;}

				}
				else
				{
					while(numberofzeros>HighZeroThreshold)
					{
						numberofzeros=numberofzeros-HighZeroThreshold;
						UpdateNum[t]=-HighZeroThreshold;
						t++;
					}
					UpdateNum[t]=-numberofzeros;
					t++;
					UpdateNum[t]=UpdateNum[i+j];
					t++;
					i=i+j+1;
				}
			}
			else
			{
				UpdateNum[t]=-(count+1-i-j);
				i=i+j+1;
			}

		}
	}
	*newcount=t;
}



//*********************************************************************
 void DrawNewTable(int isdimmed)
// if isdimmed=0/FALSE  Draw everything, editing mode
// if isdimmed=1/TRUE   Dim out appropriate columns....
// Make isdimmed a global I guess...
// Jan 24,2006:   Now dim/invisible appropriate 'arrows' that indicate loop points.
// May 12, 2005:  Updated to change color of cell that is active by a parameter scan.  This cell (ANalog, Time or DDS) now
//                turns dark yellow. might pick a better color.
// Oct 07,2012:	  Stefan Trotzky -- As noticed above: dark yellow is a bad choice. Going to orange. Also including
//				  update for MultiScan. Cleaned up the code a bit. Removed redundant assignments of
//				  cell properties. Introduced color constants (CLR_...) defined in vars.h.
{
	int i,j,k,m,cfcn,picmode,page,cmode,pic,thiscolor;
	int analogtable_visible = 0;
	int digtable_visible = 0;
 	double vlast=0,vnow=0,vshadow=0;
 	int dimset=0,nozerofound,val;
 	int DDSChannel1,DDSChannel2,DDSChannel3;

 	int ispicture=1,celltype=0; //celtype has 3 values.  0=Numeric, 1=String, 2=Picture

 	// list of colours used for different rows, alternate after every 3 rows
 	// ColourTable is a bit larger than it has to be.
 	int ColourTable [127];
 	ColourTable[0] = VAL_BLACK;
 	for (i=0;i<20;i++)
 	{
 	   ColourTable[6*i+1] = CLR_DIGITAL_LO;
 	   ColourTable[6*i+2] = CLR_DIGITAL_LO;
 	   ColourTable[6*i+3] = CLR_DIGITAL_LO;
 	   ColourTable[6*i+4] = CLR_DIGITAL_LO_ALT;
 	   ColourTable[6*i+5] = CLR_DIGITAL_LO_ALT;
 	   ColourTable[6*i+6] = CLR_DIGITAL_LO_ALT;
 	}

 	// Get visibilities and page number (and then hide tables)
 	GetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, &analogtable_visible);
	GetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, &digtable_visible);
 	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, 0);
 	SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_VISIBLE, 0);
 	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_VISIBLE, 0);
	page=GetPage();

	GetCtrlVal(panelHandle, PANEL_TGL_NUMERICTABLE,&celltype);
	if(celltype==2) {ispicture=TRUE;}
	else { ispicture=FALSE;}


 	// Dimm out complete page if not checked (THIS APPEARS TO BE OBSOLETE HERE!)
 	checkActivePages();
 	if(ischecked[page]==FALSE)
 	{   // dim the tables
 		SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_DIMMED, 1);
 		SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_DIMMED, 1);
		SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_DIMMED, 1);
 	}
	else
	{   //undim the tables
		SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_DIMMED, 0);
		SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_DIMMED, 0);
		SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_DIMMED, 0);
	}

 	picmode=0;

 	// Coloring the tables columnwise
	for(i=1;i<=NUMBEROFCOLUMNS;i++)  // scan over the columns
	{

	// Update time values and un-dimm cells, reset background to white
		SetTableCellAttribute (panelHandle, PANEL_TIMETABLE, MakePoint(i,1),
				ATTR_CELL_DIMMED, 0);
		SetTableCellVal(panelHandle,PANEL_TIMETABLE,MakePoint(i,1),TimeArray[i][page]);
		SetTableCellAttribute(panelHandle,PANEL_TIMETABLE,MakePoint(i,1),
				ATTR_TEXT_BGCOLOR, VAL_WHITE);
	// Update info panel
		SetTableCellVal(panelHandle,PANEL_INFOTABLE,MakePoint(i,1),InfoArray[i][page].text);


	// Draw analog channels according to AnalogTable[][][]
		for(j=1;j<=NUMBERANALOGROWS;j++) // scan over analog channels
		{
			// Reading in this  cell's parameters.
			cmode=AnalogTable[i][j][page].fcn;
			vlast=AnalogTable[i-1][j][page].fval;
			vnow=AnalogTable[i][j][page].fval;

			// Set cell type to numeric.
			SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
				   ATTR_CELL_TYPE, VAL_CELL_NUMERIC);

			// Write end-of-ramp value (fval) to cell
			if(cmode!=6)
				// Write fval into the cell
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, vnow);
			else if((cmode==6)&&(i==1)&&(page==1))
			{   // Give error message when attempting to copy previous to first of all columns
				ConfirmPopup("User Error","Do not use \"Same as Previous\" Setting for Column 1 Page 1.\nThis results in unstable behaviour.\nResetting Cell Function to default: step");
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, 0.0);
				// Write default values instead (step to zero)
				cmode=1;
				AnalogTable[1][j][1].fcn=cmode;
				AnalogTable[1][j][1].fval=0.0;
				AnalogTable[1][j][1].tscale=0.0;
			}
			else
			{	// Copy last value using findLastVal()
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, findLastVal(j,i,page));
			}

			// Un-dimm cells (analog channels)
			SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
				ATTR_CELL_DIMMED, 0);

			// Select the cell color depending on the function type
			switch (cmode)
			{
			case 1: // stepped value (steps to zero in grey)
				if (vnow==0)
				{ 	thiscolor = ColourTable[j]; }
				else
				{ 	thiscolor = CLR_ANALOG_STEP; }
				break;
			case 2: // linear ramp
				thiscolor = CLR_ANALOG_LINEAR; break;
			case 3: // exponential ramp
				thiscolor = CLR_ANALOG_EXP; break;
			case 4: // minjerk ramp
				thiscolor = CLR_ANALOG_MINJERK; break;
			case 5: // sine wave
				thiscolor = CLR_ANALOG_SINEWAVE; break;
			case 6: // copy previous
				thiscolor = CLR_ANALOG_COPYPREV; break;
			default:
				thiscolor = ColourTable[j];
			}
			// Change the cell color
			SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_TEXT_BGCOLOR, thiscolor);
		} // Done drawing analog chanels


	// Draw digital channels according to DigTableValues[][][]
		for(j=1;j<=NUMBERDIGITALCHANNELS;j++) // scan over digital channels
		{
			SetTableCellAttribute (panelHandle, PANEL_DIGTABLE, MakePoint(i,j),
				ATTR_CELL_DIMMED, 0);
			// if a digital value is high, colour the cell red
			if(DigTableValues[i][j][page]==1)
			{   // cell is hi
				SetTableCellVal (panelHandle, PANEL_DIGTABLE, MakePoint(i,j), 1);
				SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,MakePoint(i,j),
						ATTR_TEXT_BGCOLOR, CLR_DIGITAL_HI);
			}
			else
			{   // cell is lo
				SetTableCellVal (panelHandle, PANEL_DIGTABLE, MakePoint(i,j), 0);
				SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,MakePoint(i,j),
						ATTR_TEXT_BGCOLOR, ColourTable[j]);
			}
		} // Done drawing digital channels


	// Draw DDS rows according to ddstable[][], dds2table[][] and dds3table[][]
		// Where to find the DDS rows.
		DDSChannel1=NUMBERANALOGCHANNELS+1;
		DDSChannel2=NUMBERANALOGCHANNELS+2;
		DDSChannel3=NUMBERANALOGCHANNELS+3;

	// DDS1
		// Writing end-frequency to cell and un-dimming it.
		SetTableCellVal(panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel1),
				ddstable[i][page].end_frequency);
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel1),
				ATTR_CELL_DIMMED, 0);

		// Select the cell color depending on what's going on in that cell
		if (ddstable[i][page].amplitude == 0.0 || ddstable[i][page].is_stop)
		{   thiscolor = ColourTable[DDSChannel1]; }
		else
		{   // actual colors
			if (ddstable[i][page].start_frequency>ddstable[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPDOWN; } // ramping down
			else if  (ddstable[i][page].start_frequency<ddstable[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPUP; } // ramping up
			else
			{   thiscolor = CLR_DDS_HOLD; } // holding

		}
		// Change the cell color.
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE,MakePoint(i,DDSChannel1),
				ATTR_TEXT_BGCOLOR,thiscolor);

	// DDS2
		// Writing end-frequency to cell and un-dimming it.
		SetTableCellVal(panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel2),
				dds2table[i][page].end_frequency);
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel2),
				ATTR_CELL_DIMMED, 0);

		// Select the cell color depending on what's going on in that cell
		if (dds2table[i][page].amplitude == 0.0 || dds2table[i][page].is_stop)
		{   thiscolor = ColourTable[DDSChannel2]; }
		else
		{   // actual colors
			if (dds2table[i][page].start_frequency>dds2table[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPDOWN; } // ramping down
			else if  (dds2table[i][page].start_frequency<dds2table[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPUP; } // ramping up
			else
			{   thiscolor = CLR_DDS_HOLD; } // holding

		}
		// Change the cell color.
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE,MakePoint(i,DDSChannel2),
				ATTR_TEXT_BGCOLOR,thiscolor);

	// DDS3
		// Writing end-frequency to cell and un-dimming it.
		SetTableCellVal(panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel3),
				dds3table[i][page].end_frequency);
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,DDSChannel3),
				ATTR_CELL_DIMMED, 0);

		// Select the cell color depending on what's going on in that cell
		if (dds3table[i][page].amplitude == 0.0 || dds3table[i][page].is_stop)
		{   thiscolor = ColourTable[DDSChannel3]; }
		else
		{   // actual colors
			if (dds3table[i][page].start_frequency>dds3table[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPDOWN; } // ramping down
			else if  (dds3table[i][page].start_frequency<dds3table[i][page].end_frequency)
			{   thiscolor = CLR_DDS_RAMPUP; } // ramping up
			else
			{   thiscolor = CLR_DDS_HOLD; } // holding

		}
		// Change the cell color.
		SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE,MakePoint(i,DDSChannel3),
				ATTR_TEXT_BGCOLOR,thiscolor);
		// Done drawing the DDS rows


	// Draw "laser" channels according to LaserTable[][][]
		for(j=NUMBERANALOGCHANNELS+NUMBERDDS+1;j<=NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS;j++)
		{   // scan through the laser rows
			// Read in values from LaserTable
			cmode = LaserTable[j-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][i][page].fcn;
			vnow = LaserTable[j-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][i][page].fval;
			vshadow = findLastVal(j,i,page);

			// Write the value into the cell
			if(cmode!=0)
				// write the ending value into the cell
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, vnow);
			else if((cmode==0)&&(i==1)&&(page==1))
			{   // Give error message when attempting to copy previous to first of all columns
				ConfirmPopup("User Error","Do not use \"Same as Previous\" Setting for Column 1 Page 1.\nThis results in unstable behaviour.\nResetting Cell Function to default: step");
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, 0.0);
				cmode=1;
				LaserTable[j-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][1][1].fcn=cmode;
				LaserTable[j-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][1][1].fval=0.0;
			}
			else
			{
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
						ATTR_CTRL_VAL, vshadow);
			}

			// Select the cell color depending on what's going on in the cell
			switch(cmode)
			{
			case 1:		// step
				thiscolor = CLR_LASER_STEP; break;
			case 2:		// linear ramp
				if (vshadow>vnow)
				{   thiscolor = CLR_LASER_RAMPDOWN; } // ramping down
				else if (vshadow<vnow)
				{   thiscolor = CLR_LASER_RAMPUP; } // ramping up
				else
				{   thiscolor = CLR_LASER_HOLD; } // holding
				break;
			case 0:		// hold previous
				thiscolor = CLR_LASER_HOLD; break;
			}
			if(vnow==0.0 && (cmode!=2))
			{	thiscolor = ColourTable[j]; }

			// Change the cell color
			SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE,MakePoint(i,j),
					ATTR_TEXT_BGCOLOR, thiscolor);

		} // Done drawing laser rows
	}

	// So far, all columns are undimmed. Now check if we need to dim out any columns
	if(isdimmed==TRUE)
	{
		nozerofound=TRUE;// haven't encountered a zero yet... so keep going
		for(i=1;i<=NUMBEROFCOLUMNS;i++)
		{
			dimset=FALSE;
			if((nozerofound==FALSE)||(TimeArray[i][page]==0)) // if we have seen a zero before, or we see one now, then
			{
				nozerofound=FALSE;   // Flag that tells us to dim out all remaining columns
				dimset=TRUE;		 // Flag to dim out this column
			}
			else if((nozerofound==TRUE)&&(TimeArray[i][page]<0))// if we haven't seen a zero, but this column has a negative time then...
			{
				dimset=TRUE;
			}
			SetTableCellAttribute(panelHandle,PANEL_TIMETABLE,MakePoint(i,1),ATTR_CELL_DIMMED,dimset);
			if(ispicture==0) {picmode=0;}
			if(ispicture==1) {picmode=2;}
			if(dimset==TRUE) {picmode=2;}

			for(j=1;j<=NUMBERANALOGROWS;j++)
			{
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),ATTR_CELL_DIMMED, dimset);
				SetTableCellAttribute (panelHandle, PANEL_DIGTABLE, MakePoint(i,j),ATTR_CELL_DIMMED, dimset);
				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),ATTR_CELL_TYPE,picmode);
			}
		}
	}


	// Mark scanned parameters in case scan is active. Made a separate block for MultiScans in order not to interfere with
	// the existing stuff (Stefan Trotzky -- 2012-10-07 -- V16.0.0)
	if (parameterscanmode == 0)
	{ 	// MultiScan -- check for each scan parameter whether it is on the current page and highlight if scan is active
		if (MultiScan.Active == TRUE)
		{   // Only highlight if  scan is active.
			for(j=0;j<MultiScan.NumPars;j++)
			{   // Go through all scanned parameters
				if ((currentpage == MultiScan.Par[j].Page) && (MultiScan.Par[j].CellExists))
				{	// Highlight if parameter is on this page
					if (MultiScan.Par[j].Row == 0)
					{	// Time value?
						SetTableCellAttribute(panelHandle,PANEL_TIMETABLE,
							MakePoint(MultiScan.Par[j].Column,1),ATTR_TEXT_BGCOLOR,CLR_SCAN_TIME);
					}
					if ((MultiScan.Par[j].Row > 0) &&
						(MultiScan.Par[j].Row <= NUMDIGITALSCANOFFSET))
					{   // Analog channel, DDS, Laser or Anritsu scan
						SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE,
							MakePoint(MultiScan.Par[j].Column,MultiScan.Par[j].Row),
							ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
					}
					if (MultiScan.Par[j].Row > NUMDIGITALSCANOFFSET)
					{   // Digital channel scan
						if (MultiScan.Par[j].CurrentScanValue > 0.0)
						{
							SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,
								MakePoint(MultiScan.Par[j].Column,MultiScan.Par[j].Row-NUMDIGITALSCANOFFSET),
								ATTR_TEXT_BGCOLOR, CLR_SCAN_DIGITAL_HI);
						}
						else
						{
							SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,
								MakePoint(MultiScan.Par[j].Column,MultiScan.Par[j].Row-NUMDIGITALSCANOFFSET),
								ATTR_TEXT_BGCOLOR, CLR_SCAN_DIGITAL_LO);
						}
					}
				}
			}
		}
	}
	else
	{	// This part is left as it was
		if((currentpage==PScan.Page)&&(PScan.Scan_Active==TRUE))//display the cell active for a parameter scan
		{
	 		switch(PScan.ScanMode)
	 		{
	 			case 0:
	 				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan.Column,PScan.Row),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
					break;
				case 1:
					SetTableCellAttribute(panelHandle,PANEL_TIMETABLE,MakePoint(PScan.Column,1),ATTR_TEXT_BGCOLOR,CLR_SCAN_TIME);
					break;
				case 2:
					SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan.Column,25),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
					break;
				case 4:
					SetTableCellAttribute(panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan.Column,PScan.Row),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
					break;
			}
		}

		if(TwoParam)
		{
			if((currentpage==PScan2.Page)&&(PScan.Scan_Active==TRUE))
			{
			 	switch(PScan2.ScanMode)
		 		{
			 		case 0:
		 				SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan2.Column,PScan2.Row),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
						break;
					case 1:
						SetTableCellAttribute(panelHandle,PANEL_TIMETABLE,MakePoint(PScan2.Column,1),ATTR_TEXT_BGCOLOR,CLR_SCAN_TIME);
						break;
					case 2:
						SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan2.Column,25),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
						break;
					case 4:
						SetTableCellAttribute(panelHandle, PANEL_ANALOGTABLE, MakePoint(PScan2.Column,PScan2.Row),ATTR_TEXT_BGCOLOR, CLR_SCAN_ANALOG);
						break;
				}
			}
		}
	} // Done highlighting scanned values.

	// Dimm or un-dimm tools for inserting the Imaging page
	if(currentpage==10)
	{
		SetCtrlAttribute (panelHandle, PANEL_NUM_INSERTIONPAGE, ATTR_DIMMED, 0);
		SetCtrlAttribute (panelHandle, PANEL_NUM_INSERTIONCOL, ATTR_DIMMED, 0);
	}
	else
	{
		SetCtrlAttribute (panelHandle, PANEL_NUM_INSERTIONPAGE, ATTR_DIMMED, 1);
		SetCtrlAttribute (panelHandle, PANEL_NUM_INSERTIONCOL, ATTR_DIMMED, 1);
	}

	// Draw the arrows indicating loop position
	// DrawLoopIndicators();

	// Make tables visible again
	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, analogtable_visible);
	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, digtable_visible);
	SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_VISIBLE, 1);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_VISIBLE, 1);

	// Reset all toggle buttons
	for (i=1;i<NUMBEROFPAGES;i++)
	{
		SetCtrlVal(panelHandle, PANEL_TB_SHOWPHASE[i], 0);
	}

	// Press active page's toggle button
	SetCtrlVal (panelHandle, PANEL_TB_SHOWPHASE[currentpage], 1);
}

//*****************************************************************************************
// MENU:Analog Settings  option chosen
//
void CVICALLBACK ANALOGSET_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// It is possible to not click "set changes" and then click done after changing say the
	//		proportionality. This would not cause the values in the analog arrays that the
	//		sequencer looks at when building the sequence to be changed. However, opening
	//		the analogSettings panel again to check what the values were would show the values
	//		that had been entered and not the values the sequencer sees! This is because the
	//		analogSettings panel is persistent and the number boxes do not lose their values
	//		when the analogSettings panel is hidden. Changing the line number to a different
	//		line and back again would reload the indicators from the analog arrays that the
	//		sequencer sees, but that requires the user to know to do that.
	// So, lets explicitly reload the values from the analog arrays that the sequencer sees
	//		each time the panel is made visible.

	int line = 0;
	GetCtrlVal( panelHandle2, ANLGPANEL_NUM_ACH_LINE, &line );

	SetCtrlVal( panelHandle2, ANLGPANEL_NUM_ACHANNEL, AChName[line].chnum );
	SetCtrlVal( panelHandle2, ANLGPANEL_NUM_CHANNELPROP,AChName[line].tfcn );
	SetCtrlVal( panelHandle2, ANLGPANEL_NUM_CHANNELBIAS,AChName[line].tbias );
	SetCtrlVal( panelHandle2, ANLGPANEL_STR_CHANNELNAME, AChName[line].chname );
	SetCtrlVal( panelHandle2, ANLGPANEL_STRING_UNITS, AChName[line].units );
	SetCtrlVal( panelHandle2, ANLGPANEL_CHKBOX_RESET, AChName[line].resettozero );
	SetCtrlVal( panelHandle2, ANLGPANEL_NUM_MINV, AChName[line].minvolts );
	SetCtrlVal( panelHandle2, ANLGPANEL_NUM_MAXV, AChName[line].maxvolts );

	DisplayPanel(panelHandle2);
}
//*****************************************************************************************
// MENU:Anritsu MG37022A Settings  option chosen
//
void CVICALLBACK ANRITSUSET_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	ChangedVals = TRUE;
	DisplayPanel(panelHandleANRITSUSETTINGS);
}

//***********************************************************************************

void CVICALLBACK DIGITALSET_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	ChangedVals = TRUE;
	DisplayPanel(panelHandle3);
}

//***********************************************************************************
void CVICALLBACK PROCESSOR_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	ChangedVals = TRUE;
	DisplayPanel(panelHandle12);
}

//***********************************************************************************
void CVICALLBACK MENU_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	switch(menuItem)
	{
		case MENU_FILE_SAVESET_V16: SaveSettings(16); break;
		case MENU_FILE_LOADSET_V16: LoadSettings(16); break;
		case MENU_FILE_LOADSET_V15: LoadSettings(15); break;
		case MENU_FILE_LOADSET_V13: LoadSettings(13); break;
	}
}

//***********************************************************************************
int CVICALLBACK ANALOGTABLE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int cx=0,cy=0,pixleft=0,pixtop=0;
	Point cpoint={0,0};
	char buff[80]="";
	int leftbuttondown,rightbuttondown,keymod;
	int panel_to_display;

	//ChangedVals = TRUE;// V16.1.6: Commented out.
	switch (event)
		{
		case EVENT_LEFT_DOUBLE_CLICK:
			cpoint.x=0;cpoint.y=0;
			GetActiveTableCell(panelHandle, PANEL_ANALOGTABLE, &cpoint);
			//now set the indicators in the control panel title..ie units
			currentx=cpoint.x;
			currenty=cpoint.y;
			currentpage=GetPage();

			if((currenty>NUMBERANALOGCHANNELS)&&(currenty<=NUMBERANALOGCHANNELS+NUMBERDDS))
			{
			    Active_DDS_Panel=currenty-NUMBERANALOGCHANNELS;

				SetDDSControlPanel(Active_DDS_Panel);
				panel_to_display = panelHandle6;	 // Program DDS
			}
			else if (currenty<=NUMBERANALOGCHANNELS){

				SetControlPanel();
				panel_to_display = panelHandle4;	  // Program Analog Channel

			}
			else if ((currenty>NUMBERANALOGCHANNELS+NUMBERDDS)&&(currenty<=NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS))
			{
				SetLaserControlPanel(currenty-NUMBERANALOGCHANNELS-NUMBERDDS-1);
				panel_to_display = panelHandle11;
				printf("Fuck me\n");
			}
			else if ((currenty>NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS)&&(currenty<=NUMBERANALOGROWS))
			{
				SetAnritsuControlPanel();
				panel_to_display = panelHandleANRITSU;
				//SetControlPanel();
				//panel_to_display = panelHandle4;

			}

			sprintf(buff,"x:%d   y:%d    z:%d",currentx,currenty,currentpage);
			SetPanelAttribute (panel_to_display, ATTR_TITLE, buff);
			//Read the mouse position and open window there.
			GetGlobalMouseState (&panelHandle, &pixleft, &pixtop, &leftbuttondown,
								 &rightbuttondown, &keymod);
			SetPanelAttribute (panel_to_display, ATTR_LEFT, pixleft);
			SetPanelAttribute (panel_to_display, ATTR_TOP, pixtop);

			DisplayPanel(panel_to_display);

			break;
		}
	return 0;
}

//********************************************************************************************
int GetPage(void)
{
	return currentpage;
	//New function only returns global variable.
}


//*************************************************************************************
// 2012-10-08 Stefan Trotzky -- V16.0.1
// Made a single callback function for the toggle buttons that switch between pages..
// This makes adding pages (and hence buttons and labels) a bit easier. One might store
// all toggle button and label IDS in arrays to clean up this code even more.
int CVICALLBACK TOGGLE_ALL_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int oldpage, newpage, i;

	oldpage = currentpage;

	switch (event)
	{
	case EVENT_COMMIT:

		// Evaluate which toggle button was pressed
		switch (control)
		// (all this would work so much better if the control ids were stored in
		// a global array)
		{
		case PANEL_TB_SHOWPHASE1: newpage = 1; break;
		case PANEL_TB_SHOWPHASE2: newpage = 2; break;
		case PANEL_TB_SHOWPHASE3: newpage = 3; break;
		case PANEL_TB_SHOWPHASE4: newpage = 4; break;
		case PANEL_TB_SHOWPHASE5: newpage = 5; break;
		case PANEL_TB_SHOWPHASE6: newpage = 6; break;
		case PANEL_TB_SHOWPHASE7: newpage = 7; break;
		case PANEL_TB_SHOWPHASE8: newpage = 8; break;
		case PANEL_TB_SHOWPHASE9: newpage = 9; break;
		case PANEL_TB_SHOWPHASE10: newpage = 10; break;
		default: newpage = 0;
		}

		for (i=1;i<NUMBEROFPAGES;i++)
		{
			SetCtrlVal(panelHandle, PANEL_TB_SHOWPHASE[i], 0);
		}
		SetCtrlVal(panelHandle, PANEL_TB_SHOWPHASE[newpage], 1);

		// Do nothing if the respective page is already shown.
		if (newpage == oldpage) { return 0; }

		// Catch callbacks from other then included controls.
		if (newpage == 0)
		{
			MessagePopup("Programmer Error","This control does not work with this callback routine!");
			return 0;
		}

		// Set and display current page
		currentpage = newpage;
		//ChangedVals = TRUE;// V16.1.6: Commented out.
		DrawNewTable(isdimmed);

		break;
	}

	return 0;
}

//***************************************************************************************************
void checkActivePages(void)// Check the checkboxes and put the info in global var ischecked
{
  	int bool;
	for( int i = 1; i < NUMBEROFPAGES; ++i ){// remember that PANEL_CHKBOX[0] is undset/undefined
		GetCtrlVal(panelHandle, PANEL_CHKBOX[i], &bool);
		ischecked[i] = bool;// ischecked[0] is also unset/undefined
	}
	/*
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX, &bool);
  	ischecked[1]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_2, &bool);
  	ischecked[2]=bool;
 	GetCtrlVal (panelHandle, PANEL_CHECKBOX_3, &bool);
  	ischecked[3]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_4, &bool);
  	ischecked[4]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_5, &bool);
  	ischecked[5]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_6, &bool);
  	ischecked[6]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_7, &bool);
	ischecked[7]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_8, &bool);
	ischecked[8]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_9, &bool);
	ischecked[9]=bool;
  	GetCtrlVal (panelHandle, PANEL_CHECKBOX_10, &bool);
	ischecked[10]=bool;
	*/
}

//***************************************************************************************************
void setActivePages(void)
{
	for( int i = 1; i< NUMBEROFPAGES; ++i ){
		SetCtrlVal(panelHandle, PANEL_CHKBOX[i], ischecked[i]);
	}
}


//***************************************************************************************************
int CVICALLBACK DIGTABLE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int status=0,i_pict=0,page,digval;
	Point pval={0,0};

	//ChangedVals = TRUE;// V16.1.6: Commented out.
	page=GetPage();
	switch (event)
		{
		case EVENT_LEFT_DOUBLE_CLICK:
			GetActiveTableCell(PANEL,PANEL_DIGTABLE,&pval);
			digval=DigTableValues[pval.x][pval.y][page];
			if(digval==0)
			{
				SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,0xFF3366L);
				DigTableValues[pval.x][pval.y][page]=1;
			}
			else
			{
				SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,VAL_GRAY);
				DigTableValues[pval.x][pval.y][page]=0;
			}
			break;
		}
	return 0;
}



//***************************************************************************************************
int CVICALLBACK TIMETABLE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dval,oldtime,ratio,tscaleold;
	Point pval={0,0};
	int page,i,j;
	int ctrlmode;
	//ChangedVals = TRUE;// V16.1.6: Commented out.
	switch (event)
		{
		case EVENT_COMMIT: //EVENT_VAL_CHANGED:
		//EVENT_VAL_CHANGED only seems to pick up the last changed values, or values changed using the
		// May 31,2006 -- TimeArray is updated just fine (Dave Burns)
		//controls (i.e. increment arrows).  Similar problems with other events...reading all times(for the page)
		// at a change event seems to work.
			page=GetPage();

			for(i=1;i<=NUMBEROFCOLUMNS;i++)
			{
				oldtime=TimeArray[i][page];
				GetTableCellVal(PANEL,PANEL_TIMETABLE,MakePoint(i,1),&dval);

				TimeArray[i][page]=dval;  //double TimeArray[NUMBEROFCOLUMNS][NUMBEROFPAGES];
				//now rescale the time scale for waveform fcn. Go over all 16 analog channels
				ratio=dval/oldtime;
				if(oldtime==0) {ratio=1;}

				for(j=1;j<=NUMBERANALOGCHANNELS;j++)
				{
					tscaleold=AnalogTable[i][j][page].tscale;	// use this  and next line to auto-scale the time
					AnalogTable[i][j][page].tscale=tscaleold*ratio;
			//		AnalogTable[i][j][page].tscale=dval;	  //use this line to force timescale to period
				}

				//set the delta time value for the dds array
				//	ddstable[i][page].delta_time = dval/1000;
			}

			/* for testing prints TimeArray to STDIO
			for(i=1;i<=NUMBEROFCOLUMNS;i++)
				printf("%0.1f\t",TimeArray[i][page]);*/

			break;
		case EVENT_LEFT_DOUBLE_CLICK:
			isdimmed=0;
			DrawNewTable(isdimmed);
			break;
		}
	return 0;
}

//***************************************************************************************************
int CVICALLBACK INFOTABLE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{

	char buff[32];

	switch (event)
		{ // Read value from INFOTABLE into InfoArray[][].text
		case EVENT_COMMIT: 	// Value changed (note: GetActiveTableCell returns cell to right of
							// changed one if return was pressed!)
			GetTableCellVal(PANEL, PANEL_INFOTABLE, MakePoint(eventData2,eventData1), buff);
			strcpy(InfoArray[eventData2][currentpage].text, buff);
			break;
		}
	return 0;
}


//*********************************************************************************
// A large number of similar menu bar callback functions
void CVICALLBACK TITLE1_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 1 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE2_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 2 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE3_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 3 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE4_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 4 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE5_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 5 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE6_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 6 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE7_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 7 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE8_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 8 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE8,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE8,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLE9_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 9 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE9,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE9,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK TITLEX_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{

  char buff[80];
	int status;
	status=PromptPopup ("Enter control button label", "Enter a new label for Phase 10 control button",buff, sizeof buff-2);
	if(status==0)
	{
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE10,ATTR_OFF_TEXT, buff);
		status = SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE10,ATTR_ON_TEXT, buff);
	}
}

void CVICALLBACK SETGD5_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.005;

}

void CVICALLBACK SETGD10_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
    SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.010;
}

void CVICALLBACK SETGD15_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
    SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.015;
}

void CVICALLBACK SETGD25_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
    SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.025;
}

void CVICALLBACK SETGD50_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
    SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.050;
}

void CVICALLBACK SETGD100_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	EventPeriod=0.100;
}

void CVICALLBACK SETGD1000_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 1);
	EventPeriod=1.00;
}

// Scott Smale - 2020-05-10
// Get the update period (in us) from the menu bar cause why not.
// Performs minimal checking that there is only one choice chosen.
// I am writing this initially as a helper function for saving the update period to file.
int getUpdatePeriodFromMenu(void)
{
	int usupd5,usupd10,usupd15,usupd25,usupd50,usupd100,usupd1000;

	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, &usupd5);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, &usupd10);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, &usupd15);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, &usupd25);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, &usupd50);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, &usupd100);
	GetMenuBarAttribute(menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, &usupd1000);

	if( usupd5+usupd10+usupd15+usupd25+usupd50+usupd100+usupd1000 > 1 )
	{// if more than one of these {0,1} values are true then there has been a mistake
		return -1;// negative for error
	}

	// Having passed checks on the validity of the values, return the one that was selected
	if( usupd5 == 1 ){
		return 5;
	}
	if( usupd10 == 1 ){
		return 10;
	}
	if( usupd15 == 1 ){
		return 15;
	}
	if( usupd25 == 1 ){
		return 25;
	}
	if( usupd50 == 1 ){
		return 50;
	}
	if( usupd100 == 1 ){
		return 100;
	}
	if( usupd1000 == 1 ){
		return 1000;
	}

	return -1;// if none of the if stmts were true then error
}

// Scott Smale - 2020-05-10
// Set the given updatePeriod (in us) to the correct menu option.
// Returns value to set as global var EventPeriod if successful; negative if error.
// Performs minimal checking.
// I am writing this initially as a helper function for loading the update period from file.
double setUpdatePeriodToMenu(int updatePeriod)
{
	double ev_period = -1.0;// prepare an invalid value for error

	if( updatePeriod == 5 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 1);
		ev_period = 0.005;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 10 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 1);
		ev_period = 0.010;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 15 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 1);
		ev_period=0.015;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 25 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 1);
		ev_period=0.025;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 50 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 1);
		ev_period=0.050;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 100 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 1);
		ev_period=0.100;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	}
	if( updatePeriod == 1000 ){
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 1);
		ev_period = 1.000;
	}
	else {
		SetMenuBarAttribute( menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	}

	return ev_period;
}

//*********************************************************************************************************
void CVICALLBACK BOOTADWIN_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{

	if(processorT1x==0)
	{
	Boot("C:\\ADWIN\\ADWIN10.BTL",0);
	processnum=Load_Process("TransferData.TA1");
	}
	else if(processorT1x==1)
	{
	Boot("C:\\ADWIN\\ADWIN11.BTL",0);
	processnum=Load_Process("TransferData.TB1");
	}
	else if(processorT1x==2)
	{
	Boot("C:\\ADWIN\\ADWIN9.BTL",0);
	processnum=Load_Process("TransferData.T91");
	}

}

//*********************************************************************************************************
void CVICALLBACK BOOTLOAD_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	  int wantBootload = 0;
	  GetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED,&wantBootload);
	  SetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED,abs(wantBootload-1));

}

//**********************************************************************************************
void CVICALLBACK CLEARPANEL_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// add code to clear all the analog and digital information.....
	int i=0,j=0,k=0,status=0;

	status=ConfirmPopup("Clear Panel","Do you really want to clear the panel?") ;
	if(status==1)
	{
		ChangedVals = TRUE;
		for(i=1;i<=NUMBEROFCOLUMNS;i++)
		{
			for(j=1;j<=NUMBERANALOGCHANNELS;j++)
			{
				for(k=0;k<NUMBEROFPAGES;k++)
				{
					AnalogTable[i][j][k].fcn=1;
					AnalogTable[i][j][k].fval=0;
					AnalogTable[i][j][k].tscale=0;
					DigTableValues[i][j][k]=0;
					TimeArray[i][k]=0;
					ddstable[i][k].start_frequency=0;
					ddstable[i][k].end_frequency=0;
					ddstable[i][k].amplitude=0;
					ddstable[i][k].is_stop=0;
					dds2table[i][k].start_frequency=0;
					dds2table[i][k].end_frequency=0;
					dds2table[i][k].amplitude=0;
					dds2table[i][k].is_stop=0;
					dds3table[i][k].start_frequency=0;
					dds3table[i][k].end_frequency=0;
					dds3table[i][k].amplitude=0;
					dds3table[i][k].is_stop=0;
				}
			}
		}
		DrawNewTable(0);
	}
}

//**********************************************************************************************
void CVICALLBACK INSERTCOLUMN_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char buff[20]="",buff2[100]="";
	int page,status,i=0,j=0;
	Point cpoint={0,0};
	page=GetPage();

	GetActiveTableCell (panelHandle, PANEL_TIMETABLE, &cpoint);
	sprintf(buff,"%d",cpoint.x);
	strcat(buff2,"Really insert column at ");
	strcat(buff2,buff);
	status=ConfirmPopup("insert Array",buff2);        //User Confirmation of Column Insert
	if(status==1)
		ShiftColumn3(cpoint.x,page,-1);   //-1 for shifting columns right
}


//**********************************************************************************************
void CVICALLBACK DELETECOLUMN_CALLBACK (int menuBar, int menuItem, void *callbackData,
	int panel)
{
	char buff[20]="",buff2[100]="";
	int page,status,i=0,j=0;
	Point cpoint={0,0};
	page=GetPage();
	//PromptPopup ("Array Manipulation:Delete", "Delete which column?", buff, 3);
	GetActiveTableCell (panelHandle, PANEL_TIMETABLE, &cpoint);
	sprintf(buff,"%d",cpoint.x);

	strcat(buff2,"Really delete column ");
	strcat(buff2,buff);
	status=ConfirmPopup("Delete Array",buff2);

	if(status==1)
		ShiftColumn3(cpoint.x,page,1); //1 for shifting columns left

}

//**********************************************************************************************
void ShiftColumn3(int col, int page,int dir)
//Takes all columns starting at col on page and shifts them to the left or right depending on the value of dir
//dir==1 for a deletion: all the columns starting at col+1 are shifted one to the left, the page's last column is new
//dir==-1 for an insertion: all the columns starting at col are shifted one to the right, the column at col is new and the
//		  last column is lost
//The new column can be set with all values at 0 or with the attributes previously in place


//Replaced Malfunctioning ShiftColumn and ShiftColumn2(see previous AdwinGUI releases)
{

	int i,j,status,start,zerocol;
	//printf("col %d",col);

	if (dir==-1)			//shifts cols right (insertion)
		start=NUMBEROFCOLUMNS;
	else if (dir==1)		//shifts cols left  (deletion)
		start=col;



	//shift columns left or right depending on dir
	for(i=0;i<NUMBEROFCOLUMNS-col;i++)
	{
		TimeArray[start+dir*i][page]=TimeArray[start+dir*(i+1)][page];
		for(j=1;j<=NUMBERANALOGCHANNELS;j++)
		{
			AnalogTable[start+dir*i][j][page].fcn=AnalogTable[start+dir*(i+1)][j][page].fcn;
			AnalogTable[start+dir*i][j][page].fval=AnalogTable[start+dir*(i+1)][j][page].fval;
			AnalogTable[start+dir*i][j][page].tscale=AnalogTable[start+dir*(i+1)][j][page].tscale;
		}

		AnalogTable[start+dir*i][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fcn=AnalogTable[start+dir*(i+1)][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fcn;
		AnalogTable[start+dir*i][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fval=AnalogTable[start+dir*(i+1)][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fval;
		//AnalogTable[start+dir*i][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].tscale=AnalogTable[start+dir*(i+1)][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].tscale;



		for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
			DigTableValues[start+dir*i][j][page]=DigTableValues[start+dir*(i+1)][j][page];

		for(j=0;j<NUMBERLASERS;j++)
		{
			LaserTable[j][start+dir*i][page].fcn=LaserTable[j][start+dir*(i+1)][page].fcn;
			LaserTable[j][start+dir*i][page].fval=LaserTable[j][start+dir*(i+1)][page].fval;
		}

		ddstable[start+dir*i][page].start_frequency=ddstable[start+dir*(i+1)][page].start_frequency;
		ddstable[start+dir*i][page].end_frequency=ddstable[start+dir*(i+1)][page].end_frequency;
		ddstable[start+dir*i][page].amplitude=ddstable[start+dir*(i+1)][page].amplitude;
		ddstable[start+dir*i][page].is_stop=ddstable[start+dir*(i+1)][page].is_stop;

		dds2table[start+dir*i][page].start_frequency=dds2table[start+dir*(i+1)][page].start_frequency;
		dds2table[start+dir*i][page].end_frequency=dds2table[start+dir*(i+1)][page].end_frequency;
		dds2table[start+dir*i][page].amplitude=dds2table[start+dir*(i+1)][page].amplitude;
		dds2table[start+dir*i][page].is_stop=dds2table[start+dir*(i+1)][page].is_stop;

		dds3table[start+dir*i][page].start_frequency=dds3table[start+dir*(i+1)][page].start_frequency;
		dds3table[start+dir*i][page].end_frequency=dds3table[start+dir*(i+1)][page].end_frequency;
		dds3table[start+dir*i][page].amplitude=dds3table[start+dir*(i+1)][page].amplitude;
		dds3table[start+dir*i][page].is_stop=dds3table[start+dir*(i+1)][page].is_stop;
	}

	//if we inserted a column, then set all values to zero
	// prompt and ask if we want to duplicate the selected column



	if(dir==1)
	{
		zerocol=NUMBEROFCOLUMNS;
		status=ConfirmPopup("Duplicate","Do you want the last column to be duplicated?");
	}
	else if(dir==-1)
	{
		zerocol=col;
		status=ConfirmPopup("Duplicate","Do you want to duplicate the selected column?");
	}

	//Sets all values to zero
	if (status!=1)
	{
		if (dir==1)
			TimeArray[zerocol][page]=0;

		for(j=1;j<=NUMBERANALOGCHANNELS;j++)
		{
			AnalogTable[zerocol][j][page].fcn=1;
			AnalogTable[zerocol][j][page].fval=0;
			AnalogTable[zerocol][j][page].tscale=1;
		}
			AnalogTable[zerocol][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fcn=1;
			AnalogTable[zerocol][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][page].fval=0;

		for(j=0;j<NUMBERLASERS;j++) //Lasers set to hold last value
		{
			LaserTable[j][zerocol][page].fcn=0;
			LaserTable[j][zerocol][page].fval=0;
		}

		for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
			DigTableValues[start+dir*i][j][page]=DigTableValues[start+dir*(i+1)][j][page];

		ddstable[zerocol][page].start_frequency=0;
		ddstable[zerocol][page].end_frequency=0;
		ddstable[zerocol][page].amplitude=0;
		ddstable[zerocol][page].is_stop=FALSE;

		dds2table[zerocol][page].start_frequency=0;
		dds2table[zerocol][page].end_frequency=0;
		dds2table[zerocol][page].amplitude=0;
		dds2table[zerocol][page].is_stop=FALSE;

		dds3table[zerocol][page].start_frequency=0;
		dds3table[zerocol][page].end_frequency=0;
		dds3table[zerocol][page].amplitude=0;
		dds3table[zerocol][page].is_stop=FALSE;
	}
	ChangedVals = TRUE;
	DrawNewTable(0);

}

//***************************************************************************************************************
void CVICALLBACK COPYCOLUMN_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	//All attributes of active column are replaced with those of the global "clip" variables (from ClipColumn)

	char buff[20]="",buff2[100]="";
	int page,status,i=0,j=0;
	Point cpoint={0,0};

	page=GetPage();
	GetActiveTableCell (panelHandle, PANEL_TIMETABLE, &cpoint);

	//User Confirmation of Selected Column
	sprintf(buff,"%d",cpoint.x);
	strcat(buff2,"Confirm Copy of column ");
	strcat(buff2,buff);
	status=ConfirmPopup("Array Manipulation:Copy",buff2);

	if(status==1)
	{
		ClipColumn=cpoint.x;
		TimeClip=TimeArray[cpoint.x][page];
		for(j=1;j<=NUMBERANALOGCHANNELS;j++)
		{
			AnalogClip[j].fcn=AnalogTable[cpoint.x][j][page].fcn;
			AnalogClip[j].fval=AnalogTable[cpoint.x][j][page].fval;
			AnalogClip[j].tscale=AnalogTable[cpoint.x][j][page].tscale;
		}

		for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
			DigClip[j]=DigTableValues[cpoint.x][j][page];

		for (j=1;j<=NUMBERLASERS;j++)
		{
			LaserClip[j].fcn=LaserTable[j-1][cpoint.x][page].fcn;
			LaserClip[j].fval=LaserTable[j-1][cpoint.x][page].fval;

		}


		ddsclip.start_frequency=ddstable[cpoint.x][page].start_frequency;
		ddsclip.end_frequency=ddstable[cpoint.x][page].end_frequency;
		ddsclip.amplitude=ddstable[cpoint.x][page].amplitude;
		ddsclip.is_stop=ddstable[cpoint.x][page].is_stop;

		dds2clip.start_frequency=dds2table[cpoint.x][page].start_frequency;
		dds2clip.end_frequency=dds2table[cpoint.x][page].end_frequency;
		dds2clip.amplitude=dds2table[cpoint.x][page].amplitude;
		dds2clip.is_stop=dds2table[cpoint.x][page].is_stop;

		dds3clip.start_frequency=dds3table[cpoint.x][page].start_frequency;
		dds3clip.end_frequency=dds3table[cpoint.x][page].end_frequency;
		dds3clip.amplitude=dds3table[cpoint.x][page].amplitude;
		dds3clip.is_stop=dds3table[cpoint.x][page].is_stop;


		DrawNewTable(0); //draws undimmed table if already dimmed
	}

}

//**********************************************************************************
void CVICALLBACK PASTECOLUMN_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)

// Replaces all the values in the selected column with the global "clip" values

{
	char buff[100]="";
	int page,status,i=0,j=0;
	Point cpoint={0,0};
	//ChangedVals = TRUE;// V16.1.6: Commented out.
	page=GetPage();

	//Ensures a column has been copied to the clipboard
	if (ClipColumn>0)
	{
		// User Confirmation of Copy Function
		GetActiveTableCell (panelHandle, PANEL_TIMETABLE, &cpoint);
		sprintf(buff,"Confirm Copy of Column %d to %d?\nContents of Column %d will be lost.",ClipColumn,cpoint.x,cpoint.x);
		status=ConfirmPopup("Paste Column",buff);

		if(status==1)
		{
			TimeArray[cpoint.x][page]=TimeClip;
			for(j=1;j<=NUMBERANALOGCHANNELS;j++)
			{
				AnalogTable[cpoint.x][j][page].fcn=AnalogClip[j].fcn;
				AnalogTable[cpoint.x][j][page].fval=AnalogClip[j].fval;
				AnalogTable[cpoint.x][j][page].tscale=AnalogClip[j].tscale;
			}
			for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
			{
				DigTableValues[cpoint.x][j][page]=DigClip[j];
			}

			for (j=1;j<=NUMBERLASERS;j++)
			{
				LaserTable[j-1][cpoint.x][page].fcn=LaserClip[j].fcn;
				LaserTable[j-1][cpoint.x][page].fval=LaserClip[j].fval;
			}


			ddstable[cpoint.x][page].start_frequency=ddsclip.start_frequency;
			ddstable[cpoint.x][page].end_frequency=ddsclip.end_frequency;
			ddstable[cpoint.x][page].amplitude=ddsclip.amplitude;
			ddstable[cpoint.x][page].is_stop=ddsclip.is_stop;

			dds2table[cpoint.x][page].start_frequency=dds2clip.start_frequency;
			dds2table[cpoint.x][page].end_frequency=dds2clip.end_frequency;
			dds2table[cpoint.x][page].amplitude=dds2clip.amplitude;
			dds2table[cpoint.x][page].is_stop=dds2clip.is_stop;

			dds3table[cpoint.x][page].start_frequency=dds3clip.start_frequency;
			dds3table[cpoint.x][page].end_frequency=dds3clip.end_frequency;
			dds3table[cpoint.x][page].amplitude=dds3clip.amplitude;
			dds3table[cpoint.x][page].is_stop=dds3clip.is_stop;

			DrawNewTable(0);
		}
	}
	else
		ConfirmPopup("Copy Column","No Column Selected");
}

//**********************************************************************************
int CVICALLBACK TGLNUMERIC_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int val=0;
	switch (event)
		{
		case EVENT_COMMIT:
			// find out if the button is pressed or not
			// change buttons to appropriate mode and draw new table
			GetCtrlVal(panelHandle, PANEL_TGL_NUMERICTABLE,&val);
			if(val==0)
			{
				SetDisplayType(VAL_CELL_NUMERIC);
			}
			else
			{
				SetDisplayType(VAL_CELL_PICTURE);
			}


			break;
		}
	return 0;
}

//**********************************************************************************
void CVICALLBACK DDSSETUP_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	LoadDDSSettings();
	DisplayPanel(panelHandle5);
}

//**********************************************************************************
void CVICALLBACK LASERSET_CALLBACK(int menubar, int menuItem, void *callbackData, int panel)
/* Callback function for menu selection of Settings --> Laser Setup, opens LaserSettings.uir and centers it*/
{
	DisplayPanel(panelHandle10);
	SetPanelPos (panelHandle10, VAL_AUTO_CENTER, VAL_AUTO_CENTER);
}

//********************************************************************************************
int CVICALLBACK DISPLAYDIAL_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int dialval=0;
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_DISPLAYDIAL, &dialval);
			// Now change the size of the tables depending on the dial value.


			SetChannelDisplayed(dialval);

			break;
		}
	return 0;
}
/************************************************************************
Author: Stefan
-------
Date Created: August 2004
-------
Date Modified: October 06, 2012 (S. Trotzky, V16.0.0)
-------
Description: Resizes the analog table on the gui
-------
Return Value: void
-------
Parameters: new top, left and height values for the list box
*************************************************************************/
void ReshapeAnalogTable( int top,int left,int height)
{
	int j,istext=0,celltype=0,ddsrowtop,rowheight;
	int modheight;

	rowheight = (height)/(NUMBERANALOGCHANNELS+NUMBERDDS);

	for (j=1;j<=NUMBERANALOGROWS;j++)
	{
		SetTableRowAttribute (panelHandle, PANEL_ANALOGTABLE, j,ATTR_SIZE_MODE, VAL_USE_EXPLICIT_SIZE);
		SetTableRowAttribute (panelHandle, PANEL_ANALOGTABLE, j, ATTR_ROW_HEIGHT, rowheight);
	    SetTableRowAttribute (panelHandle, PANEL_TBL_ANAMES, j,ATTR_SIZE_MODE, VAL_USE_EXPLICIT_SIZE);
		SetTableRowAttribute (panelHandle, PANEL_TBL_ANAMES, j, ATTR_ROW_HEIGHT, rowheight);
	}
	for (j=1;j<=NUMBERANALOGCHANNELS;j++)
	{

		SetTableRowAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, j,ATTR_SIZE_MODE, VAL_USE_EXPLICIT_SIZE);
		SetTableRowAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, j, ATTR_ROW_HEIGHT, rowheight);
	}

	modheight=(NUMBERANALOGROWS)*(int)((height)/(NUMBERANALOGCHANNELS+NUMBERDDS))+3;

	//resize the analog table and all it's related list boxes
  	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_HEIGHT, modheight);
  	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_LEFT, left);
	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_TOP, top);

	SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_LEFT, left-165);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_TOP, top);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_HEIGHT, modheight);

	SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_HEIGHT,modheight);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_TOP, top);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_LEFT, left+705);

   	// move the DDS offsets
   	// V16.1.3: Don't set the left hand side of the DDS offsets here. Let GUIDesign.uir do it.
   	ddsrowtop = height * NUMBERANALOGCHANNELS/NUMBERANALOGROWS + 2*NUMBERANALOGCHANNELS;
	SetCtrlAttribute (panelHandle, PANEL_NUM_DDS_OFFSET,ATTR_TOP,ddsrowtop);
	//SetCtrlAttribute (panelHandle, PANEL_NUM_DDS_OFFSET,ATTR_LEFT,877);
	SetCtrlAttribute (panelHandle, PANEL_NUM_DDS2_OFFSET,ATTR_TOP,ddsrowtop+rowheight);
	//SetCtrlAttribute (panelHandle, PANEL_NUM_DDS2_OFFSET,ATTR_LEFT,877);
	SetCtrlAttribute (panelHandle, PANEL_NUM_DDS3_OFFSET,ATTR_TOP,ddsrowtop+rowheight*2);
	//SetCtrlAttribute (panelHandle, PANEL_NUM_DDS3_OFFSET,ATTR_LEFT,877);

	// move the SRS control (2013-01-30, ST: hidden)
	// V16.1.3: Don't set the left hand side of the SRS freq here. Let GUIDesign.uir do it.
	SetCtrlAttribute (panelHandle, PANEL_SRS_FREQ,ATTR_TOP,ddsrowtop+rowheight*5);
	//SetCtrlAttribute (panelHandle, PANEL_SRS_FREQ,ATTR_LEFT,877);

	// move the ANRITSU offset
	// V16.1.3: Don't set the left hand side of the ANRITSU offset here. Let GUIDesign.uir do it.
	SetCtrlAttribute (panelHandle, PANEL_ANRITSU_OFFSET,ATTR_TOP,ddsrowtop+rowheight*(NUMBERDDS+NUMBERLASERS));
	//SetCtrlAttribute (panelHandle, PANEL_ANRITSU_OFFSET,ATTR_LEFT,877);

}



/************************************************************************
Author: Stefan
-------
Date Created: August 2004
-------
Description: Resizes the digital table on the gui
-------
Return Value: void
-------
Parameters: new top, left and height values for the list box
*************************************************************************/
void ReshapeDigitalTable( int top,int left,int height)
{
 int j;

  	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_HEIGHT, height);
  	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_LEFT, left);
	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_TOP, top);
	SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_TOP, top);
	SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_LEFT, left-165);
	SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_HEIGHT, height);

	for (j=1;j<=NUMBERDIGITALCHANNELS;j++)
	{
		SetTableRowAttribute (panelHandle, PANEL_DIGTABLE, j, ATTR_SIZE_MODE, VAL_USE_EXPLICIT_SIZE);
		SetTableRowAttribute (panelHandle, PANEL_DIGTABLE, j, ATTR_ROW_HEIGHT, (height)/(NUMBERDIGITALCHANNELS));
		SetTableRowAttribute (panelHandle, PANEL_TBL_DIGNAMES, j, ATTR_SIZE_MODE, VAL_USE_EXPLICIT_SIZE);

		SetTableRowAttribute (panelHandle, PANEL_TBL_DIGNAMES, j, ATTR_ROW_HEIGHT, (height)/(NUMBERDIGITALCHANNELS));
	}

}
/************************************************************************
Author: David McKay
-------
Date Created: August 23, 2004
-------
Date Modified: August 23, 2004
-------
Description: Sets how to display the data, graphically or numerically
-------
Return Value: void
-------
Parameter1:
int display_setting: type of display
VAL_CELL_NUMERIC : graphic
VAL_CELL_PICTURE : numeric
*************************************************************************/
void SetDisplayType(int display_setting)
{

	int i,j;

	//set button status
	if (display_setting == VAL_CELL_NUMERIC)
	{
		SetCtrlVal(panelHandle, PANEL_TGL_NUMERICTABLE, 0);
	}
	else if (display_setting == VAL_CELL_PICTURE)
	{
		SetCtrlVal(panelHandle, PANEL_TGL_NUMERICTABLE, 1);
	}

//	printf("called Display Type with value:   %d \n",display_setting);
	for(i=1;i<=NUMBEROFCOLUMNS;i++)
	{
		for(j=1;j<=NUMBERANALOGCHANNELS;j++)
		{
			SetTableCellAttribute (panelHandle, PANEL_ANALOGTABLE, MakePoint(i,j),
				   ATTR_CELL_TYPE, display_setting);
		 }
	}
}

/************************************************************************
Author: David McKay
-------
Date Created: August 23, 2004
-------
Date Modified: October 06, 2012 (S. Trotzky, V16.0.0)
-------
Description: Sets which channel to display: analog, digital or both
-------
Return Value: void
-------
Parameter1:
int display_setting: channel to display
1: both
2: analog
3: digital
*************************************************************************/
void SetChannelDisplayed(int display_setting)
{
	int toppos1,toppos2,toppos3,leftpos,heightpos1,heightpos2,heightpos3,width;
	//hide everything
	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_VISIBLE, 0);
	//ATTR_LABEL_VISIBLE

	heightpos1=695;
	heightpos2=582;
	heightpos3=240;
	toppos1=140;
	leftpos=170;
	toppos2=toppos1+heightpos1;
	toppos3=toppos2+heightpos2+15;

	// Reshape Timetable
	GetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_WIDTH, &width);
	SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_LEFT, leftpos);
	SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_TOP, toppos1-25);
	SetCtrlAttribute (panelHandle, PANEL_TIMETABLE, ATTR_WIDTH, width);

	// Reshape Infotable (TO BE IMPLEMENTED!!!)
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_LEFT, leftpos);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_TOP, toppos1-50);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_WIDTH, width);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_VISIBLE, FALSE); // hide for now


	switch (display_setting)
		{
		case 1:		// both
			ReshapeAnalogTable(toppos1,leftpos,heightpos1);   //passed top, left and height
			ReshapeDigitalTable(toppos2+100,leftpos,heightpos2);
			ReshapeMultiScanTable(toppos3+100,leftpos,heightpos3);

			SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_VISIBLE, 1);

			break;

		case 2:		// analog tableonly

			ReshapeAnalogTable(toppos1,170,heightpos1);   //passed top, left and height
			ReshapeMultiScanTable(toppos2+105,leftpos,heightpos3);

			SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_ANALOGUNITS, ATTR_VISIBLE, 1);
			break;

		case 3:		 // digital table only
			ReshapeDigitalTable(toppos1,170,heightpos2);
			ReshapeMultiScanTable(toppos1+heightpos2+25,leftpos,heightpos3);

			SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_VISIBLE, 1);
			break;

		}

		SetCtrlVal(panelHandle, PANEL_DISPLAYDIAL, display_setting);
	DrawNewTable(0);
	return;
}

//***********************************************************************************************
double CheckIfWithinLimits(double OutputVoltage, int linenumber)
{
	double NewOutput;
	NewOutput=OutputVoltage;
	if(OutputVoltage>AChName[linenumber].maxvolts) NewOutput=AChName[linenumber].maxvolts;
	if(OutputVoltage<AChName[linenumber].minvolts) NewOutput=AChName[linenumber].minvolts;
	return NewOutput;
}

//***********************************************************************************************
void CVICALLBACK RESETZERO_CALLBACK (int
menuBar, int menuItem, void *callbackData,
		int panel)
{
	int wantResetZero = 0;
	GetMenuBarAttribute (menuHandle, MENU_SETTINGS_RESETZERO, ATTR_CHECKED,&wantResetZero);
	SetMenuBarAttribute (menuHandle, MENU_SETTINGS_RESETZERO, ATTR_CHECKED,abs(wantResetZero-1));
}


//********************************************************************************************************************
void CVICALLBACK EXPORT_PANEL_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char fexportname[200];

	FileSelectPopup("", "*.export", "", "Export Panel?", VAL_SAVE_BUTTON, 0, 0, 1, 1,fexportname);

	ExportPanel(fexportname,strlen(fexportname));
}


//***********************************************************************************************
void CVICALLBACK CONFIG_EXPORT_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char fconfigname[290];

	FileSelectPopup("", "*.config", "", "Save Configuration", VAL_SAVE_BUTTON, 0, 0, 1, 1,fconfigname);

	ExportConfig(fconfigname,strlen(fconfigname));
}

//**************************************************************************************************************
void CVICALLBACK MENU_ALLLOW_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
}

//**************************************************************************************************************
void CVICALLBACK MENU_HOLD_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
}

//**************************************************************************************************************
void CVICALLBACK MENU_BYCHANNEL_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
}


//**************************************************************************************************************
int CVICALLBACK NUM_INSERTIONPAGE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			ChangedVals = TRUE;
		}
	return 0;
}
//**************************************************************************************************************
int CVICALLBACK NUM_INSERTIONCOL_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			ChangedVals = TRUE;
			break;
		}
	return 0;
}
//**************************************************************************************************************
void CVICALLBACK COMPRESSION_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	BOOL UseCompression;
	GetMenuBarAttribute (menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, &UseCompression);
	if(UseCompression)
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, FALSE);
	}
	else
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, TRUE);
	}
}

//**************************************************************************************************************
void CVICALLBACK SHOWARRAY_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	BOOL ShowArray;
	GetMenuBarAttribute (menuHandle, MENU_PREFS_SHOWARRAY, ATTR_CHECKED, &ShowArray);
	if(ShowArray)
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_SHOWARRAY, ATTR_CHECKED, FALSE);
	}
	else
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_SHOWARRAY, ATTR_CHECKED, TRUE);
	}


}
//**************************************************************************************************************
void CVICALLBACK DDS_OFF_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int i=0,j=0;
	for(j=0;j<NUMBEROFPAGES;j++)
	{
		for(i=0;i<NUMBEROFCOLUMNS;i++)
		{
			ddstable[i][j].is_stop=TRUE;
		}
	}

}

//**************************************************************************************************************
void CVICALLBACK SIMPLETIMING_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	BOOL Simple_Timing;
	GetMenuBarAttribute (menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, &Simple_Timing);
	if(Simple_Timing)
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, FALSE);
	}
	else
	{
		SetMenuBarAttribute (menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, TRUE);
	}
}

//**************************************************************************************************************
void CVICALLBACK SCANSETTING_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SetCtrlVal(panelHandle7,SCANPANEL_TXT_LASIDENT,"Lasername");
	InitializeScanPanel();
}


//**************************************************************************************************************
void CVICALLBACK NOTECHECK_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	BOOL status;
	GetMenuBarAttribute (menuHandle, MENU_SETTINGS_NOTECHECK, ATTR_CHECKED,&status);
	SetMenuBarAttribute (menuHandle, MENU_SETTINGS_NOTECHECK, ATTR_CHECKED,!status);

	if(status=1)
	{
		DisplayPanel(panelHandle_sub1);
	}
	else
	{
		HidePanel(panelHandle_sub1);
	}

}

//**************************************************************************************************************
void CVICALLBACK STREAM_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	BOOL status;
	char fstreamdir[290];
	GetMenuBarAttribute (menuHandle, MENU_PREFS_STREAM_SETTINGS, ATTR_CHECKED,&status);
	SetMenuBarAttribute (menuHandle, MENU_PREFS_STREAM_SETTINGS, ATTR_CHECKED,!status);
	if (!status==TRUE)
	{
		DirSelectPopup ("","Select Stream Folder", 1, 1,fstreamdir);
	}
}

//**************************************************************************************************************
// 06 October 2012 -- for multiparameter scans
// Increase number of columns according to numeric ctrl value
int CVICALLBACK MULTICSAN_NUM_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int iscols, becols;

	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &iscols); // assumes same number of cols in position and value tables
	GetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, &becols);

	// In case the number of requested parameters exceeds the allowed number
	if (becols>NUMMAXSCANPARAMETERS)
	{
		becols = NUMMAXSCANPARAMETERS;
		SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, becols);
	}

	// Add or remove columns
	if ((becols>iscols) && (becols<=NUMMAXSCANPARAMETERS)) // add column(s) behind last
	{
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, -1, becols-iscols, 0);
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, -1, becols-iscols, 0);
	}
    else if ((becols<iscols)  && (becols>=1)) // remove last column(s)
	{
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, becols+1, -1);
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, becols+1, -1);
	}
	return 0;
}

//**************************************************************************************************************
// 10 January 2013 -- for multiparameter scans
// Increase number of scan table rows according to numeric ctrl value
int CVICALLBACK MULTISCAN_NUMROWS_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int isrows, berows;

	GetNumTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &isrows);
	GetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_ROWS, &berows);

	// Add or remove rows
	if ((berows>isrows)) // add rows(s) behind last
	{
		InsertTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, -1, berows-isrows, 0);
	}
    else if ((berows<isrows)  && (berows>=1)) // remove last row(s) only if scan is not active
	{
		if (MultiScan.Active == FALSE)
		{
			DeleteTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, berows+1, -1);
		}
		else
		{
			SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_ROWS, isrows);
		}
	}
	return 0;
}







//**************************************************************************************************************
void MoveCanvasStart(int XPos,int IsVisible)
{
int x0=170,dx=40,xwidth=25;

SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_LEFT,x0+(XPos-1)*dx);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_TOP,141);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_VISIBLE,IsVisible);

}
//**************************************************************************************************************
void MoveCanvasEnd(int XPos,int IsVisible)
{
int x0=170,dx=40,xwidth=25;

SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_LEFT,x0+(XPos-1)*dx+xwidth);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_TOP,141);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_VISIBLE,IsVisible);

}

//*************************************************************************************
void DrawLoopIndicators()
{
    int page,x0,dx,xendoffset,xstartoffset;
    int xposstart,xposend;
    int canvaswidth;

    GetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_WIDTH,&canvaswidth);
	page=GetPage();
	CanvasClear(panelHandle,PANEL_CANVAS_LOOPLINE,VAL_ENTIRE_OBJECT);

	if(Switches.loop_active)
 	{
 		if(page==LoopPoints.startpage)
 		{
 			MoveCanvasStart(LoopPoints.startcol,TRUE);
 		}
 		else
 		{
 			MoveCanvasStart(LoopPoints.startcol,FALSE);
 		}
 		if(page==LoopPoints.endpage)
 		{
 			MoveCanvasEnd(LoopPoints.endcol,TRUE);
 		}
 		else
 		{
			MoveCanvasEnd(LoopPoints.endcol,FALSE);
		}

 	}
 	else
 	{
 	    MoveCanvasStart(LoopPoints.startcol,FALSE);
		MoveCanvasEnd(LoopPoints.endcol,FALSE);
 	}

 	// If good, then draw a connecting line
 	dx=40;
 	xstartoffset=10;
 	xendoffset=10;

 	if(Switches.loop_active)
 	{
 		SetCtrlAttribute (panelHandle, PANEL_CANVAS_LOOPLINE, ATTR_PEN_WIDTH,2);
		SetCtrlAttribute (panelHandle, PANEL_CANVAS_LOOPLINE, ATTR_PEN_COLOR,0x00FF00L);

		xposstart=(LoopPoints.startcol-1)*dx+xstartoffset;
 		xposend=(LoopPoints.endcol)*dx-xstartoffset;

 		if((page==LoopPoints.startpage)&&!(page==LoopPoints.endpage))  {xposend=canvaswidth;}
 		if(!(page==LoopPoints.startpage)&&(page==LoopPoints.endpage))  {xposstart=0+xstartoffset;}
 		if((page>LoopPoints.startpage)&&(page<LoopPoints.endpage))
 		{
 			xposstart=0+xstartoffset;
 			xposend=canvaswidth;
 		}
 		CanvasDrawLine (panelHandle, PANEL_CANVAS_LOOPLINE, MakePoint(xposstart,8), MakePoint(xposend,8));
		if((page<LoopPoints.startpage)||(page>LoopPoints.endpage))
		{
		  	CanvasClear(panelHandle,PANEL_CANVAS_LOOPLINE,VAL_ENTIRE_OBJECT);
		}

 	}

}

//**************************************************************************************************************
//Jan24, 2006
// ensure start/stop ordering is correct, if not, then swap them
// Move Loop start and stop indicators, and refresh display
int CVICALLBACK SWITCH_LOOP_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int pagestart,pageend,colstart,colend;
	int temppage,tempcol;
	BOOL ValuesGood,LoopSwitchOn;
	switch (event)
		{
		case EVENT_COMMIT:
			ValuesGood=FALSE;
			LoopSwitchOn=FALSE;
			Switches.loop_active=FALSE;    // set default loop condition to false

			GetCtrlVal (panelHandle, PANEL_SWITCH_LOOP, &LoopSwitchOn);

			GetCtrlVal (panelHandle, PANEL_NUM_LOOPPAGE1, &pagestart);
			GetCtrlVal (panelHandle, PANEL_NUM_LOOPPAGE2, &pageend);
			GetCtrlVal (panelHandle, PANEL_NUM_LOOPCOL1, &colstart);
			GetCtrlVal (panelHandle, PANEL_NUM_LOOPCOL2, &colend);


			if((pagestart>pageend)||((pagestart==pageend)&&(colstart>colend)))
			{
				ValuesGood=FALSE;
				MessagePopup("Bad Conditions","Start and end cells do not compute");
				SetCtrlVal (panelHandle, PANEL_SWITCH_LOOP, FALSE);

				MoveCanvasStart(colstart,FALSE);
				MoveCanvasEnd(colend,FALSE);
			}
			else
			{
				ValuesGood=TRUE;
				LoopPoints.startpage=pagestart;
				LoopPoints.endpage=pageend;
				LoopPoints.startcol=colstart;
				LoopPoints.endcol=colend;
			}


			if(ValuesGood&&LoopSwitchOn)
			{
				Switches.loop_active=TRUE;
				MoveCanvasStart(colstart,TRUE);
				MoveCanvasEnd(colend,TRUE);
			}
			else
			{
			 	Switches.loop_active=FALSE;
				MoveCanvasStart(colstart,FALSE);
				MoveCanvasEnd(colend,FALSE);
			}
			DrawNewTable(1);
			break;
		}
	return 0;
}


//********************************* CONTEXT MENU CALLBACKS *****************************************************
//**************************************************************************************************************
void CVICALLBACK Dig_Cell_Copy(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	//Copies the value of the active DigitalTabel value into the clipboard

	int page;
	Point pval={0,0};

	page=GetPage();
	GetActiveTableCell(PANEL,PANEL_DIGTABLE,&pval);
	DigClip[0]=DigTableValues[pval.x][pval.y][page];

}
//**************************************************************************************************************
void CVICALLBACK Dig_Cell_Paste(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	//Pastes the value store in DigClip[0] by Dig_Cell_Copy into the selected Digital Table Cells

	int page;
	Rect selection;
	Point pval={0,0};

	page=GetPage();
	GetTableSelection (panelHandle, PANEL_DIGTABLE, &selection);  //note: returns a 0 to all values if only 1 cell selected

	//Pasting into multiple cells
	if(selection.top>0)
	{
		for(pval.y=selection.top;pval.y<=selection.top+(selection.height-1);pval.y++)
		{
			for(pval.x=selection.left;pval.x<=selection.left+(selection.width-1);pval.x++)
			{

				DigTableValues[pval.x][pval.y][page]=DigClip[0];
				if(DigClip[0]==1)
					SetTableCellAttribute(panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,0xFF3366L);
				else if(DigClip[0]==0)
					SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,VAL_GRAY);
			}
		}
	}
	//Pasting into single cell
	else if(selection.top==0)
	{
		GetActiveTableCell(PANEL,PANEL_DIGTABLE,&pval);
		DigTableValues[pval.x][pval.y][page]=DigClip[0];
		if(DigClip[0]==1)
			SetTableCellAttribute(panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,0xFF3366L);
		else if(DigClip[0]==0)
			SetTableCellAttribute (panelHandle, PANEL_DIGTABLE,pval, ATTR_TEXT_BGCOLOR,VAL_GRAY);
	}
}
//**************************************************************************************************************
void CVICALLBACK Analog_Cell_Copy(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	//This function copies the contents of the active AnalogTable Cell to the Clipboard Globals
	//Handles Analog Channels DDS1,DDS2,DDS3,Lasers. It is called by right clicking on the Analog
	//Table and Selecing "Copy"

	int page,laserNum;
	Point pval={0,0};

	page=GetPage();
	GetActiveTableCell(PANEL,PANEL_ANALOGTABLE,&pval);

	if(pval.y<=NUMBERANALOGCHANNELS)
	{
		AnalogClip[0].fcn = AnalogTable[pval.x][pval.y][page].fcn;
		AnalogClip[0].fval = AnalogTable[pval.x][pval.y][page].fval;
		AnalogClip[0].tscale = AnalogTable[pval.x][pval.y][page].tscale;
	}
	else if(pval.y<=NUMBERANALOGCHANNELS+NUMBERDDS)
	{
		ddsclip.start_frequency=ddstable[pval.x][page].start_frequency;
		ddsclip.end_frequency=ddstable[pval.x][page].end_frequency;
		ddsclip.amplitude=ddstable[pval.x][page].amplitude;
		ddsclip.is_stop=ddstable[pval.x][page].is_stop;
	}
	else if(pval.y<=NUMBERANALOGROWS)
	{
		laserNum=pval.y-(NUMBERANALOGCHANNELS+NUMBERDDS+1);
		LaserClip[0].fcn=LaserTable[laserNum][pval.x][page].fcn;
		LaserClip[0].fval=LaserTable[laserNum][pval.x][page].fval;

	}

}
//**************************************************************************************************************
void CVICALLBACK Analog_Cell_Paste(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	//Replaces Highlighted Cell contents with the values copied to the clipboard using Analog_Cell_Copy
	//This function Handles copies and pastes of analog channel and dds data.

	int page,col,row,laserNum;
	Rect selection;
	Point pval={0,0};

	page=GetPage();
	GetTableSelection (panelHandle, PANEL_ANALOGTABLE, &selection);  //returns a 0 to all values if only 1 cell selected

	//Paste made into multiple cells of analog channels
	if(selection.top<=NUMBERANALOGCHANNELS&&selection.top>0)
	{
		row=selection.top;
		while ((row<=selection.top+(selection.height-1))&&(row<=NUMBERANALOGCHANNELS))
		{
			for(col=selection.left;col<=selection.left+(selection.width-1);col++)
			{
				AnalogTable[col][row][page].fcn=AnalogClip[0].fcn;
				AnalogTable[col][row][page].fval=AnalogClip[0].fval;
				AnalogTable[col][row][page].tscale=AnalogClip[0].tscale;
			}
			row++;
		}
	}

	//Paste made into multiple cells of DDS channels
	else if(selection.top<=NUMBERANALOGCHANNELS+NUMBERDDS&&selection.top>0)
	{
		for(row=selection.top;row<=selection.top+(selection.height-1);row++)
		{

			for(col=selection.left;col<=selection.left+(selection.width-1);col++)
			{
				if(row==NUMBERANALOGCHANNELS+1)
				{
					ddstable[col][page].start_frequency=ddsclip.start_frequency;
					ddstable[col][page].end_frequency=ddsclip.end_frequency;
					ddstable[col][page].amplitude=ddsclip.amplitude;
					ddstable[col][page].is_stop=ddsclip.is_stop;
				}
				else if(row==NUMBERANALOGCHANNELS+2)
				{
					dds2table[col][page].start_frequency=ddsclip.start_frequency;
					dds2table[col][page].end_frequency=ddsclip.end_frequency;
					dds2table[col][page].amplitude=ddsclip.amplitude;
					dds2table[col][page].is_stop=ddsclip.is_stop;
				}
				else if(row==NUMBERANALOGCHANNELS+3)
				{
					dds3table[col][page].start_frequency=ddsclip.start_frequency;
					dds3table[col][page].end_frequency=ddsclip.end_frequency;
					dds3table[col][page].amplitude=ddsclip.amplitude;
					dds3table[col][page].is_stop=ddsclip.is_stop;
				}
			}
		}

	}
	//Paste made into multiple Laser cells
	else if(selection.top<=NUMBERANALOGROWS&&selection.top>0)
	{
		row=selection.top;
		while ((row<=selection.top+(selection.height-1))&&(row<=NUMBERANALOGROWS))
		{
			laserNum=row-(NUMBERANALOGCHANNELS+NUMBERDDS+1);
			for(col=selection.left;col<=selection.left+(selection.width-1);col++)
			{
				LaserTable[laserNum][col][page].fcn=LaserClip[0].fcn;
				LaserTable[laserNum][col][page].fval=LaserClip[0].fval;
			}
			row++;
		}
	}

	//Paste Made into single Cell
	else if(selection.top==0);
	{
		GetActiveTableCell (panelHandle, PANEL_ANALOGTABLE, &pval);
		if(pval.y<=NUMBERANALOGCHANNELS)
		{
			AnalogTable[pval.x][pval.y][page].fcn=AnalogClip[0].fcn;
			AnalogTable[pval.x][pval.y][page].fval=AnalogClip[0].fval;
			AnalogTable[pval.x][pval.y][page].tscale=AnalogClip[0].tscale;
		}
		else if (pval.y==NUMBERANALOGCHANNELS+1)
		{
			ddstable[pval.x][page].start_frequency=ddsclip.start_frequency;
			ddstable[pval.x][page].end_frequency=ddsclip.end_frequency;
			ddstable[pval.x][page].amplitude=ddsclip.amplitude;
			ddstable[pval.x][page].is_stop=ddsclip.is_stop;
		}
		else if (pval.y==NUMBERANALOGCHANNELS+2)
		{
			dds2table[pval.x][page].start_frequency=ddsclip.start_frequency;
			dds2table[pval.x][page].end_frequency=ddsclip.end_frequency;
			dds2table[pval.x][page].amplitude=ddsclip.amplitude;
			dds2table[pval.x][page].is_stop=ddsclip.is_stop;
		}
		else if (pval.y==NUMBERANALOGCHANNELS+3)
		{
			dds3table[pval.x][page].start_frequency=ddsclip.start_frequency;
			dds3table[pval.x][page].end_frequency=ddsclip.end_frequency;
			dds3table[pval.x][page].amplitude=ddsclip.amplitude;
			dds3table[pval.x][page].is_stop=ddsclip.is_stop;
		}
		else if(pval.y<=NUMBERANALOGROWS)
		{
			laserNum=pval.y-(NUMBERANALOGCHANNELS+NUMBERDDS+1);
			LaserTable[laserNum][pval.x][page].fcn=LaserClip[0].fcn;
			LaserTable[laserNum][pval.x][page].fval=LaserClip[0].fval;
		}
	}
	//ChangedVals = TRUE;// V16.1.6: Commented out.
	DrawNewTable(0);
}


// Callback so that right clicking on an EOR numeric entry element will add that EOR to the scan table.
int CVICALLBACK DDS_OFFSET_CALLBACK (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_RIGHT_CLICK:
			MultiScan_AddDdsEorToScan(control);
			break;
	}
	return 0;
}


//**************************************************************************************************************
void CVICALLBACK EXIT (int menuBar, int menuItem, void *callbackData,int panel)
{
	int status;
	status=ConfirmPopup("Exit","Are you sure you want to quit?\nUnsaved data will be lost.");
	if (status==1)
		QuitUserInterface(1);		// kills the GUI and ends program

}

//**************************************************************************************************************
double findLastVal(int row, int column, int page)
/*  This column finds the (final) value in the cell occuring before (in time) the one given by the parameters
 	accounting for the insert page and skipping same as cells. Works for Analog Rows, DDS Rows, and Laser Rows

	Caveat: If the imaging page in being "inserted" into a page which is not checked this altgorithm wont work,
	Be my guest if you would like to fix it */
{
	// find the last value
	int CellFound,cx,cz,insertpage,insertcolumn,i;
	double lastVal;

	cx=column; //col index
	cz=page; //page index
	CellFound=0;

	GetCtrlVal (panelHandle, PANEL_NUM_INSERTIONPAGE, &insertpage);
	GetCtrlVal (panelHandle, PANEL_NUM_INSERTIONCOL, &insertcolumn);


	while(CellFound<1)
	{
		//go back one step
		cx--;
		if (cx<1)								//Move to previous page
		{
			if (cz==NUMBEROFPAGES-1&&insertpage<NUMBEROFPAGES-1)  //Check if were looking at the insert page && that its inserted somewhere
			{
				cz=insertpage;
				cx=insertcolumn-1;
				insertcolumn=-1;				//Set this so we dont get caught into thinking page is inserted again
			}
			else
			{
				cx=NUMBEROFCOLUMNS;					   //Check that the page were moving to is checked
				do{
					cz--;
				}while((!ischecked[cz])&&(cz>=1));
			}
			if(cz<1)
				CellFound=2;   //Means we hit column1 page1 with nothing found
		}


		//Check if cell is insert page
		if((cz==insertpage)&&(cx==insertcolumn-1)&&(ischecked[NUMBEROFPAGES-1]))
		{
			cz=NUMBEROFPAGES-1;
			cx=NUMBEROFCOLUMNS;
		}

		//Check to see a valid value has been found
		if (TimeArray[cx][cz]>0.0)
		{
			i=cx-1;
			//ensure no zeroes preceded it in page
			if(i>0)
			{
				while(i>0&&TimeArray[i][cz]!=0.0){i--;}
				if (i==0)
					CellFound=1;
				else
					cx=i;

			}
			else
				CellFound=1;
		}

	}//done search


	if(CellFound==1)
	{
		if(row<=NUMBERANALOGCHANNELS&&row>=1)
		{
			if(AnalogTable[cx][row][cz].fcn!=6)				// checks to see if found cell is a same as previous
				lastVal=AnalogTable[cx][row][cz].fval;
			else
				lastVal=findLastVal(row,cx,cz);
		}
		else if (row==NUMBERANALOGCHANNELS+1)
			lastVal=ddstable[cx][cz].end_frequency;
		else if (row==NUMBERANALOGCHANNELS+2)
			lastVal=dds2table[cx][cz].end_frequency;
		else if (row==NUMBERANALOGCHANNELS+3)
			lastVal=dds3table[cx][cz].end_frequency;
		else if (row>NUMBERANALOGCHANNELS+NUMBERDDS&&row<=NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS)
		{
			if(LaserTable[row-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][cx][cz].fcn!=0)             // checks for same as previous condition
			{
				lastVal=LaserTable[row-(NUMBERANALOGCHANNELS+NUMBERDDS+1)][cx][cz].fval;
				//printf("Laser row: %d Col: %d Page: %d",row-(NUMBERANALOGCHANNELS+NUMBERDDS+1),cx,cz);
			}
			else
				lastVal=lastVal=findLastVal(row,cx,cz);

		}
		//And for the anritsu rows
		else if (row > NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS && row<= NUMBERANALOGROWS)
		{
			lastVal = 0;
		}
	}
	else
		lastVal=0.0;

	return lastVal;

}

//**************************************************************************************************************
void SeqError(char * msg)
{
	if(SeqErrorCount<MAXERRS)
	{
		SeqErrorBuffer[SeqErrorCount]=msg;
		SeqErrorCount++;
	}
}
/*************************************************************************************************************************/
int int_power(int base, int power)
{
	int i,num=1;
	for (i=0;i<power;i++)
		num*=base;
	return num;
}

/*************************************************************************************************************************/
void CVICALLBACK GPIB_SRS_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
		SetGPIBPanel();
		DisplayPanel(panelHandle13);
}

/**************************************************************************************/


