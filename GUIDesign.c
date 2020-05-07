
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

			// In case the start of the  scan was confirmed (common to either scanmode)
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

//*********************************************************************
// 2016_09_19 --- Scott Smale --- Started in Version 16.1.E
// For scans, save the sequencer files and also create the run directory
// and commands directory. Sets MultiScan.CommandsFilePath to the
// directory to look for commands files.
// Returns 1 if setup went smoothly and the scan should start.
// Returns -1 otherwise.
//*********************************************************************
int SetupScanFiles(int version, char *outputCmdsDirPath){


	FILE *fpini;
	char c;
	int inisize = 0;
	char iniFilename[MAX_PATHNAME_LEN];
	int returnStatus;
	int loadonboot;
	int confirmStatus;

	int panStatus;
	int panFilePathLength;
	char panFilePath[MAX_PATHNAME_LEN];
	char panFileDir[MAX_PATHNAME_LEN];
	char panFileName[MAX_PATHNAME_LEN];

	int runDirStatus;
	char runDirPath[MAX_PATHNAME_LEN];
	int commandsDirStatus;
	char commandsDirPath[MAX_PATHNAME_LEN];
	int plotsDirStatus;
	char plotsDirPath[MAX_PATHNAME_LEN];
	int imgsDirStatus;
	char imgsDirPath[MAX_PATHNAME_LEN];

	char mscanFileDirPath[MAX_PATHNAME_LEN];
	char *buffPtr;

	char buff[MAX_PATHNAME_LEN];

	//Check if .ini file exists.  Load it if it does.
	// Scott Note: The behaviour of always opening the latest folder should already be present in
	// 	labwindows/cvi. Except for the very first time, this likely isn't doing anything. Pfft, I
	//	suppose I will keep it for now.
	if(!(fpini=fopen("gui.ini","r"))==NULL)		// if "gui.ini" exists, then read it  Just contains a filename.
	{												//If not, do nothing
		while (fscanf(fpini,"%c",&c) != -1) iniFilename[inisize++] = c;  //Read the contained name
	}
 	fclose(fpini);

 	printf("iniFilename:\n>%s<\n", iniFilename);

	// Select file name for the .pan, .gpib, and .arr files
	panStatus = FileSelectPopup ("", "*.pan", "", "Save Settings", VAL_SAVE_BUTTON, 0, 0, 1, 1, panFilePath);
	// Return value of FileSelectPopup is
	// 		negative on error
	//		0	VAL_NO_FILE_SELECTED
	//		1	VAL_EXISTING_FILE_SELECTED
	//		2	VAL_NEW_FILE_SELECTED
	if( panStatus < 0 ){// Error
		printf("Error in selecting file to save parameters. Error code: %d\n", panStatus);
		MessagePopup ("File Error", "An error occured while selecting the filename to save paramters");
		returnStatus = -1;
	}
	else if( panStatus == 0 ){
		MessagePopup ("File Error", "No file was selected");
		returnStatus = -1;
	}
	else if( panStatus == 1 || panStatus == 2 ){
		// Now we should have a valid filename in. So write the save files.

		printf("panFilePath:\n>%s<\n", panFilePath);

		// First write the panel gui file (.pan)
		// This one can be problematic when elements have been removed from the GUI!!!
		// The third argument to SavePanelState is a unique state index so that you can save multiple
		// panel states in the same file. Here it is arbitrarily set to 1.
		SavePanelState(PANEL, panFilePath, 1);

		// Next write the .ini file in the GUI progam directory
		GetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED, &loadonboot);
		if(!(fpini=fopen("gui.ini","w"))==NULL)
		{
			fprintf(fpini, panFilePath);
			fprintf(fpini, "\n%d", loadonboot);
		}
		fclose(fpini);

		// Save the arrays and the laser data together ie. V16 not V15.
		panFilePathLength = strlen(panFilePath);
		SaveArraysV16(panFilePath, panFilePathLength);

		// Add the name of the .pan file to the program header
		SplitPath(panFilePath, NULL, panFileDir, panFileName);
		strcpy(buff, SEQUENCER_VERSION);
		strcat(buff, panFileName);
		SetPanelAttribute(panelHandle, ATTR_TITLE, buff);

		// Now we have saved everything that used to be saved.
		// Now want to create folder with same name as .pan file (minus .pan)
		//	and to create the commands folder inside it.

		// Try creating the run folder
		strcpy(buff, panFileName);
		buff[strlen(buff)-4] = '\0';// Assume the last 4 characters of panFileName are the ".pan"
		MakePathname(panFileDir, buff, runDirPath);

		printf("runDirPath:\n>%s<\n", runDirPath);


		runDirStatus = MakeDir(runDirPath);

		if( runDirStatus == 0 ){// Successfully created directory

			// Form the commands subdirectory path and print it.
			strcpy(buff, "commands");
			MakePathname(runDirPath, buff, commandsDirPath);
			printf("commandsDirPath:\n>%s<\n", commandsDirPath);

			// Form the plots subdirectory path and print it.
			strcpy(buff, "plots");
			MakePathname(runDirPath, buff, plotsDirPath);
			printf("plotsDirPath:\n>%s<\n", plotsDirPath);

			// Form the imgs subdirectory path and print it.
			strcpy(buff, "imgs");
			MakePathname(runDirPath, buff, imgsDirPath);
			printf("imgsDirPath:\n>%s<\n", imgsDirPath);

			// Attempt to create the subdirectories
			commandsDirStatus = MakeDir(commandsDirPath);
			plotsDirStatus = MakeDir(plotsDirPath);
			imgsDirStatus = MakeDir(imgsDirPath);

			// Check status of subdirectories and ask user to start scan if okay
			if( commandsDirStatus < 0 ){// Error
				printf("Error when creating commands directory. Error code: %d\n", commandsDirStatus);
				MessagePopup ("Directory Error", "An error occured while creating the commands directory.");
				returnStatus = -1;
			}
			else if( plotsDirStatus < 0 ){//Error
				printf("Error when creating plots directory. Error code: %d\n", plotsDirStatus);
				MessagePopup ("Directory Error", "An error occured while creating the plots directory.");
				returnStatus = -1;
			}
			else if( imgsDirStatus < 0 ){//Error
				printf("Error when creating images directory. Error code: %d\n", imgsDirStatus);
				MessagePopup ("Directory Error", "An error occured while creating the images directory.");
				returnStatus = -1;
			}
			else if( commandsDirStatus == 0 && plotsDirStatus == 0 && imgsDirStatus == 0 ){// Success
				// We have saved the sequence files and created the run and commands directories
				// so return 0 for not failure.
				// Note that the only way to get here was to have panStatus == 1 or 2, runDirStatus == 0,
				// and commandsDirStatus == 0 and plotsDirStatus == 0 and imgsDirStatus == 0.
				confirmStatus = ConfirmPopup ("Confirm Scan",
							"Confirm: Begin Experiment using Multi-parameter Scan?");
				if( confirmStatus == 0 ){// User selected no
					returnStatus = -1;
				}
				else if( confirmStatus == 1 ){// User selected yes
					// Set the output buffer from this function to point to the commands file.
					strcpy(outputCmdsDirPath, commandsDirPath);
					// Create the mscan file name and path and save it in the MultiScan struct.
					strcpy(buff, panFileName);
					buffPtr = buff+strlen(buff)-3;//Set buffPtr to point to 'p' in ".pan"
					strcpy(buffPtr, "mscan\0");
					MakePathname(runDirPath, buff, mscanFileDirPath);
					//printf("mscanFileDirPath:\n>%s<\n", mscanFileDirPath);
					strcpy(MultiScan.ScanDirPath, mscanFileDirPath);
					// Notify of success
					returnStatus = 1;
				}
				else {// Error
					printf("Error in confirming scan popup. Error code: %d\n", confirmStatus);
					MessagePopup ("Confirmation Error", "Error in confirming scan popup");
					returnStatus = -1;
				}
			}
			else {
				MessagePopup ("Directory Error", "Unknown status code from MakeDir");
				returnStatus = -1;
			}

		}
		else if( runDirStatus == -9 ){// The directory (or file) already exists

			// Ask for confimation to use existing directory.
			confirmStatus = ConfirmPopup ("Confirm use of existing directory",
							"Warning! Use existing scan directory?");
			if( confirmStatus == 0 ){// User selected no
				returnStatus = -1;
			}
			else if( confirmStatus == 1 ){// User selected yes
				// Since the scan directory already exists the subdirectories probably also
				// already exist so we should accept those that exist as is.

				// Form the commands subdirectory path and print it.
				strcpy(buff, "commands");
				MakePathname(runDirPath, buff, commandsDirPath);
				printf("commandsDirPath:\n>%s<\n", commandsDirPath);

				// Form the plots subdirectory path and print it.
				strcpy(buff, "plots");
				MakePathname(runDirPath, buff, plotsDirPath);
				printf("plotsDirPath:\n>%s<\n", plotsDirPath);

				// Form the imgs subdirectory path and print it.
				strcpy(buff, "imgs");
				MakePathname(runDirPath, buff, imgsDirPath);
				printf("imgsDirPath:\n>%s<\n", imgsDirPath);

				// Attempt to create the subdirectories
				commandsDirStatus = MakeDir(commandsDirPath);
				plotsDirStatus = MakeDir(plotsDirPath);
				imgsDirStatus = MakeDir(imgsDirPath);

				// Check status of subdirectories and ask user to start scan if okay
				if( commandsDirStatus < 0 && commandsDirStatus != -9 ){// Error (but not error if dir already exists)
					printf("Error when creating commands directory. Error code: %d\n", commandsDirStatus);
					MessagePopup ("Directory Error", "An error occured while creating the commands directory.");
					returnStatus = -1;
				}
				else if( plotsDirStatus < 0 && plotsDirStatus != -9 ){// Error (but not error if dir already exists)
					printf("Error when creating plots directory. Error code: %d\n", plotsDirStatus);
					MessagePopup ("Directory Error", "An error occured while creating the plots directory.");
					returnStatus = -1;
				}
				else if( imgsDirStatus < 0 && imgsDirStatus != -9 ){// Error (but not error if dir already exists)
					printf("Error when creating images directory. Error code: %d\n", imgsDirStatus);
					MessagePopup ("Directory Error", "An error occured while creating the images directory.");
					returnStatus = -1;
				}
				else if( (commandsDirStatus == 0 || commandsDirStatus == -9) &&
						 (plotsDirStatus == 0 || plotsDirStatus == -9) &&
						 (imgsDirStatus == 0 || imgsDirStatus == -9) ){// Success
					// We have saved the sequence files and created the run and commands directories
					// so return 0 for not failure.
					confirmStatus = ConfirmPopup ("Confirm Scan",
								"Confirm: Begin Experiment using Multi-parameter Scan?");
					if( confirmStatus == 0 ){// User selected no
						returnStatus = -1;
					}
					else if( confirmStatus == 1 ){// User selected yes
						// Set the output buffer from this function to point to the commands file.
						strcpy(outputCmdsDirPath, commandsDirPath);
						// Create the mscan file name and path and save it in the MultiScan struct.
						strcpy(buff, panFileName);
						buffPtr = buff+strlen(buff)-3;//Set buffPtr to point to 'p' in ".pan"
						strcpy(buffPtr, "mscan\0");
						MakePathname(runDirPath, buff, mscanFileDirPath);
						//printf("mscanFileDirPath:\n>%s<\n", mscanFileDirPath);
						strcpy(MultiScan.ScanDirPath, mscanFileDirPath);
						// Notify of success
						returnStatus = 1;
					}
					else {// Error
						printf("Error in confirming scan popup. Error code: %d\n", confirmStatus);
						MessagePopup ("Confirmation Error", "Error in confirming scan popup");
						returnStatus = -1;
					}
				}
				else {
					MessagePopup ("Directory Error", "Unknown status code from MakeDir");
					returnStatus = -1;
				}


			}
			else {// Error
				printf("Error in confirm overwrite scan directory popup. Error code: %d\n", confirmStatus);
				MessagePopup ("Confirmation Error", "Error in confirm overwrite scan directory popup");
				returnStatus = -1;
			}
		}
		else if( runDirStatus < 0 ){// Error
			printf("Error when creating run directory. Error code: %d\n", runDirStatus);
			MessagePopup ("Directory Error", "An error occured while creating the run directory.");
			returnStatus = -1;
		}
		else {
			MessagePopup ("Directory Error", "Unknown status code from MakeDir");
			returnStatus = -1;
		}
	}
	else {
		MessagePopup ("File Error", "Unknown status code from FileSelectPopup");
		returnStatus = -1;
	}

	return returnStatus;
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
	static ddsoptions_struct MetaDDS2Array[NUMBEROFMETACOLUMNS];
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
					 ddsoptions_struct DDS2Array[500],
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
	//InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);

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

	/*	#ifdef PRINT_TO_DEBUG
			sprintf(buff,"count %d",count);
			InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
			sprintf(buff,"compressed count %d",newcount);
			InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
			sprintf(buff,"updates %d",nuptotal);
			InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
			sprintf(buff,"time to generate arrays:   %d",tstop-tstart);
			InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);

		#endif		   */


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

	// even more debug info
/*	InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
	memused=(count+2*nuptotal)*4;//in bytes
	sprintf(buff,"MB of data sent:   %f",(double)memused/1000000);
	InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
	sprintf(buff,"Transfer Rate:   %f   MB/s",(double)memused/(double)timeused/1000);
	InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
 */

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

//*****************************************************************************************
//June 7, 2005 :  Completed Scan capability, added on-screen display of scan progress.
// May 11, 2005:  added capability to change time and DDS settings too.  Redesigned Scan structure
// May 03, 2005
// existing problem: if the final value isn't exactly reached by the steps, then the last stage is skipped and the
// cycle doesn't end
// has to do with numsteps.  Should be programmed with ceiling(), not abs
void UpdateScanValue(int Reset)
{
	if(TwoParam)
	{
	UpdateScan2Value(Reset);

	}
	else
	{
	UpdateScan1Value(Reset);

	}


}

//*****************************************************************************************
//June 7, 2005 :  Completed Scan capability, added on-screen display of scan progress.
// May 11, 2005:  added capability to change time and DDS settings too.  Redesigned Scan structure
// May 03, 2005
// existing problem: if the final value isn't exactly reached by the steps, then the last stage is skipped and the
// cycle doesn't end
// has to do with numsteps.  Should be programmed with ceiling(), not abs
void UpdateScan1Value(int Reset)
{
  	static int scanstep,iteration;
  	int i,cx,cy,cz,numsteps;
	double tempnumsteps,cellval,nextcell;
	BOOL LastScan,UseList;
	int hour,minute,second;
  	static BOOL ScanUp;
  	static int timesdid,counter;
  	char buff[400];

  	cx = PScan.Column;
  	cy = PScan.Row;
  	cz = PScan.Page;

	// SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,1);

	//if Use_List is checked, then read values off of the SCAN_TABLE on the main panel.
	GetCtrlVal(panelHandle7,SCANPANEL_CHECK_USE_LIST,&UseList); // Sets UseList

	// Initialization on first iteration
	if(Reset==TRUE)
	{
		PScan.ScanDone = FALSE;
		counter = 0;
		for(i=0;i<1000;i++)
		{   // Reset scan buffer
		 	ScanBuffer[i].Step=0;
		 	ScanBuffer[i].Iteration=0;
		 	ScanBuffer[i].Value=0;
		}

		//Copy information from the appropriate scan mode to the variables.
  		switch(PScan.ScanMode)
 	 	{
  			case 0: // set to analog
  				ScanVal.End =		PScan.Analog.End_Of_Scan;
  				ScanVal.Start =		PScan.Analog.Start_Of_Scan;
		  		ScanVal.Step =		PScan.Analog.Scan_Step_Size;
  				ScanVal.Iterations= PScan.Analog.Iterations_Per_Step;
  				break;
	  		case 1:   // time scan
  				ScanVal.End=		PScan.Time.End_Of_Scan;
  				ScanVal.Start=		PScan.Time.Start_Of_Scan;
  				ScanVal.Step=		PScan.Time.Scan_Step_Size;
	  			ScanVal.Iterations=	PScan.Time.Iterations_Per_Step;
  				break;
	  		case 2:
  				ScanVal.End=		PScan.DDS.End_Of_Scan;
  				ScanVal.Start=		PScan.DDS.Start_Of_Scan;
  				ScanVal.Step=	   	PScan.DDS.Scan_Step_Size;
 	 			ScanVal.Iterations=	PScan.DDS.Iterations_Per_Step;
  				break;
  			case 3:
  				ScanVal.End=		PScan.DDSFloor.Floor_End;
  				ScanVal.Start=		PScan.DDSFloor.Floor_Start;
				ScanVal.Step=		PScan.DDSFloor.Floor_Step;
				ScanVal.Iterations=	PScan.DDSFloor.Iterations_Per_Step;
			case 4: //Laser Scan
  				ScanVal.End=		PScan.Laser.End_Of_Scan;
  				ScanVal.Start=		PScan.Laser.Start_Of_Scan;
				ScanVal.Step=		PScan.Laser.Scan_Step_Size;
				ScanVal.Iterations=	PScan.Laser.Iterations_Per_Step;
			case 5: //SRS
  				ScanVal.End=		PScan.SRS.SRS_End;
  				ScanVal.Start=		PScan.SRS.SRS_Start;
				ScanVal.Step=		PScan.SRS.SRS_Step;
				ScanVal.Iterations=	PScan.SRS.Iterations_Per_Step;
			break;


  		}

  	    if(UseList)// if we are set to use the scan list instead of a linear scan, then read first value
  	    {
  	    	GetTableCellVal(panelHandle, PANEL_SCAN_TABLE, MakePoint(1,1), &cellval);
  	    	ScanVal.Start=cellval;
  	    }
  		timesdid = 0;
  		scanstep = 0;
 	 	ScanVal.Current_Step=0;
  		ScanVal.Current_Iteration=-1;
 	 	ScanVal.Current_Value=ScanVal.Start;


  		// determine the sign of the step and correct if necessary
  		if(ScanVal.End>=ScanVal.Start)
  		{
  			ScanUp=TRUE;
  			if(ScanVal.Step<0) {ScanVal.Step=-ScanVal.Step;}
  		}
  		else
  		{
  			ScanUp=FALSE;	  // ie. we scan downwards
  			if(ScanVal.Step>0) {ScanVal.Step=-ScanVal.Step;}
  		}
  	}//Done setting/resetting values


  	// numsteps to depend on mode
  	if(UseList)// UseList=TRUE .... therefore using table of Scan Values
	{
		ScanVal.Current_Iteration++;
		if(ScanVal.Current_Iteration>=ScanVal.Iterations)
		{
			ScanVal.Current_Iteration=0;
			ScanVal.Current_Step++;
			// read next element of scan list
			GetTableCellVal(panelHandle, PANEL_SCAN_TABLE, MakePoint(1,ScanVal.Current_Step+1), &cellval);
			// indicate which element of the list we are currently using
			SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step+1), ATTR_TEXT_BGCOLOR,
							   VAL_LT_GRAY); // This value grey
			SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step), ATTR_TEXT_BGCOLOR,
							   VAL_WHITE); // last value white again
			ScanVal.Current_Value=cellval;
			ChangedVals = TRUE;


			//check for end condition
 			// Scanning ends if we program any value <= -999 into a cell of the Scan List
			if (ScanVal.Current_Value<=-999.0)
			{
				PScan.ScanDone=TRUE;
				SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step), ATTR_TEXT_BGCOLOR,
							   VAL_WHITE);

			}
		}
	}



	else // UseList=FALSE.... therefor assume linear scanning
	{
		// calculate number of steps in the ramp
		numsteps=(int)ceil(abs(((double)ScanVal.Start-(double)ScanVal.End)/(double)ScanVal.Step));
  		PScan.ScanDone=FALSE;
		timesdid++;
  		ScanVal.Current_Iteration++;

		if((ScanVal.Current_Iteration>=ScanVal.Iterations)&&(ScanVal.Current_Step<numsteps)) // update the step at correct time
		{
			ScanVal.Current_Iteration=0;
			ScanVal.Current_Step++;
			ScanVal.Current_Value=ScanVal.Current_Value + ScanVal.Step;
			ChangedVals = TRUE;
		}
		//if we are at the last step, then set the scan value to the last value (in case the step size causes the scan to go too far
		if((ScanVal.Current_Step==numsteps)&&(ScanVal.Current_Iteration>=ScanVal.Iterations)) //ScanVal.Current_Value>=ScanVal.End
		{
			ScanVal.Current_Step++;
			ScanVal.Current_Iteration = 0;
			ScanVal.Current_Value = ScanVal.End;
		}
		if((ScanVal.Current_Step>numsteps)&&(ScanVal.Current_Iteration>=ScanVal.Iterations))
			PScan.ScanDone=TRUE;


	}

	//insert current scan values into the tables , so they are included in the next BuildUpdateList
	switch(PScan.ScanMode)
	{
		case 0:// Analog value
			AnalogTable[cx][cy][cz].fval = ScanVal.Current_Value;
			AnalogTable[cx][cy][cz].fcn = PScan.Analog.Analog_Mode;
			break;
		case 1:// Time duration
			TimeArray[cx][cz] = ScanVal.Current_Value;
			break;
		case 2:// DDS frequency
			ddstable[cx][cz].amplitude=PScan.DDS.Current;
			ddstable[cx][cz].start_frequency = PScan.DDS.Base_Freq;
			ddstable[cx][cz].end_frequency = ScanVal.Current_Value;
			break;
		case 3: // DDS offset/floor
			SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,ScanVal.Current_Value);
			break;
		case 4: //LaserFreq
			LaserTable[cy-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx][cz].fval=ScanVal.Current_Value;
			break;
		case 5: // SRS Frequency
			SetCtrlVal(panelHandle,PANEL_SRS_FREQ,ScanVal.Current_Value);
			break;

	}

	// Record current scan information into a string buffer, so we can write it to disk later.
	GetSystemTime(&hour,&minute,&second);
	ScanBuffer[counter].Step=ScanVal.Current_Step;
	ScanBuffer[counter].Iteration=ScanVal.Current_Iteration;
	ScanBuffer[counter].Value=ScanVal.Current_Value;
	ScanBuffer[0].BufferSize=counter;
	sprintf(ScanBuffer[counter].Time,"%d:%d:%d",hour,minute,second);
	counter++;

	// display current scan parameters on screen
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANVAL, ScanVal.Current_Value);
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANSTEP, ScanVal.Current_Step);
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANITER, ScanVal.Current_Iteration);

    // if the scan is done, then cleanup and write the starting values back into the tables
	if(PScan.ScanDone==TRUE)
	{   // reset initial values in the tables
		switch(PScan.ScanMode)
		{
			case 0:// Analog value
				AnalogTable[cx][cy][cz].fval=PScan.Analog.Start_Of_Scan;
				break;
			case 1:// Time duration
				TimeArray[cx][cz]=PScan.Time.Start_Of_Scan;
				break;
			case 2:// DDS frequency
				ddstable[cx][cz].end_frequency=PScan.DDS.Start_Of_Scan;
				break;
			case 3: // DDS offset/floor
				SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,PScan.DDSFloor.Floor_Start);
				break;
			case 4:
				LaserTable[cy-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx][cz].fval=PScan.Laser.Start_Of_Scan;
				break;
			case 5:
				SetCtrlVal(panelHandle,PANEL_SRS_FREQ,PScan.SRS.SRS_Start);
				break;

		}

		//hide the information panel
		// SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,0);
		ExportScanBuffer();// prompt to write out information
		ChangedVals = TRUE;
	}
	if( ChangedVals ){
		DrawNewTable(1);
	}

}




//*****************************************************************************************
//June 7, 2005 :  Completed Scan capability, added on-screen display of scan progress.
// May 11, 2005:  added capability to change time and DDS settings too.  Redesigned Scan structure
// May 03, 2005
// existing problem: if the final value isn't exactly reached by the steps, then the last stage is skipped and the
// cycle doesn't end
// has to do with numsteps.  Should be programmed with ceiling(), not abs
void UpdateScan2Value(int Reset)
{
  	static int scanstep,iteration;
  	int i,cx,cy,cz,cx2,cy2,cz2,numsteps;
	double tempnumsteps,cellval,nextcell;
	BOOL LastScan,UseList;
	int hour,minute,second;
  	static BOOL ScanUp,Scan2Up;
  	static int timesdid,counter;
  	char buff[400];

  	cx=PScan.Column;
  	cy=PScan.Row;
  	cz=PScan.Page;

  	cx2=PScan2.Column;
  	cy2=PScan2.Row;
  	cz2=PScan2.Page;

	SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,1);

	// Initialization on first iteration
	if(Reset==TRUE)
	{
		PScan.ScanDone=FALSE;
		ChangeScan1Param=FALSE;

		counter=0;
		for(i=0;i<1000;i++)
		{
		 	ScanBuffer[i].Step=0;
		 	ScanBuffer[i].Iteration=0;
		 	ScanBuffer[i].Value=0;
		}

		//Copy information from the appropriate scan mode to the variables.
  		switch(PScan.ScanMode)
 	 	{
  			case 0: // set to analog
  				ScanVal.End =		PScan.Analog.End_Of_Scan;
  				ScanVal.Start =		PScan.Analog.Start_Of_Scan;
		  		ScanVal.Step =		PScan.Analog.Scan_Step_Size;
  				ScanVal.Iterations= PScan.Analog.Iterations_Per_Step;
  				break;
	  		case 1:   // time scan
  				ScanVal.End=		PScan.Time.End_Of_Scan;
  				ScanVal.Start=		PScan.Time.Start_Of_Scan;
  				ScanVal.Step=		PScan.Time.Scan_Step_Size;
	  			ScanVal.Iterations=	PScan.Time.Iterations_Per_Step;
  				break;
	  		case 2:
  				ScanVal.End=		PScan.DDS.End_Of_Scan;
  				ScanVal.Start=		PScan.DDS.Start_Of_Scan;
  				ScanVal.Step=	   	PScan.DDS.Scan_Step_Size;
 	 			ScanVal.Iterations=	PScan.DDS.Iterations_Per_Step;
  				break;
  			case 3:
  				ScanVal.End=		PScan.DDSFloor.Floor_End;
  				ScanVal.Start=		PScan.DDSFloor.Floor_Start;
				ScanVal.Step=		PScan.DDSFloor.Floor_Step;
				ScanVal.Iterations=	PScan.DDSFloor.Iterations_Per_Step;
			case 4: //Laser Scan
  				ScanVal.End=		PScan.Laser.End_Of_Scan;
  				ScanVal.Start=		PScan.Laser.Start_Of_Scan;
				ScanVal.Step=		PScan.Laser.Scan_Step_Size;
				ScanVal.Iterations=	PScan.Laser.Iterations_Per_Step;
			case 5: //SRS
  				ScanVal.End=		PScan.SRS.SRS_End;
  				ScanVal.Start=		PScan.SRS.SRS_Start;
				ScanVal.Step=		PScan.SRS.SRS_Step;
				ScanVal.Iterations=	PScan.SRS.Iterations_Per_Step;
			break;


  		}

  		switch(PScan2.ScanMode)
 	 	{
  			case 0: // set to analog
  				Scan2Val.End =		PScan2.Analog.End_Of_Scan;
  				Scan2Val.Start =		PScan2.Analog.Start_Of_Scan;
		  		Scan2Val.Step =		PScan2.Analog.Scan_Step_Size;
  				Scan2Val.Iterations= PScan2.Analog.Iterations_Per_Step;
  				break;
	  		case 1:   // time scan
  				Scan2Val.End=		PScan2.Time.End_Of_Scan;
  				Scan2Val.Start=		PScan2.Time.Start_Of_Scan;
  				Scan2Val.Step=		PScan2.Time.Scan_Step_Size;
	  			Scan2Val.Iterations=	PScan2.Time.Iterations_Per_Step;
  				break;
	  		case 2:
  				Scan2Val.End=		PScan2.DDS.End_Of_Scan;
  				Scan2Val.Start=		PScan2.DDS.Start_Of_Scan;
  				Scan2Val.Step=	   	PScan2.DDS.Scan_Step_Size;
 	 			Scan2Val.Iterations=	PScan2.DDS.Iterations_Per_Step;
  				break;
  			case 3:
  				Scan2Val.End=		PScan2.DDSFloor.Floor_End;
  				Scan2Val.Start=		PScan2.DDSFloor.Floor_Start;
				Scan2Val.Step=		PScan2.DDSFloor.Floor_Step;
				Scan2Val.Iterations=	PScan2.DDSFloor.Iterations_Per_Step;
			case 4: //Laser Scan
  				Scan2Val.End=		PScan2.Laser.End_Of_Scan;
  				Scan2Val.Start=		PScan2.Laser.Start_Of_Scan;
				Scan2Val.Step=		PScan2.Laser.Scan_Step_Size;
				Scan2Val.Iterations=	PScan2.Laser.Iterations_Per_Step;
			case 5: //SRS
  				Scan2Val.End=		PScan2.SRS.SRS_End;
  				Scan2Val.Start=		PScan2.SRS.SRS_Start;
				Scan2Val.Step=		PScan2.SRS.SRS_Step;
				Scan2Val.Iterations=	PScan2.SRS.Iterations_Per_Step;
			break;


  		}


  	   	GetTableCellVal(panelHandle, PANEL_SCAN_TABLE, MakePoint(1,1), &cellval);
  	   	ScanVal.Start=cellval;
  	   	ScanVal.Current_Step=-1;
  		ScanVal.Current_Iteration=-1;
 	 	ScanVal.Current_Value=ScanVal.Start;


  	   	GetTableCellVal(panelHandle, PANEL_SCAN_TABLE_2, MakePoint(1,1), &cellval);
  	   	Scan2Val.Start=cellval;
  	   	Scan2Val.Current_Step=-1;
  		Scan2Val.Current_Iteration=-1;
 	 	Scan2Val.Current_Value=Scan2Val.Start;




	}//Done setting/resetting values

	Scan2Val.Current_Iteration++;
	Scan2Val.Current_Step++;

	// read next element of scan list
	GetTableCellVal(panelHandle, PANEL_SCAN_TABLE_2, MakePoint(1,Scan2Val.Current_Step+1), &cellval);

	// indicate which element of the list we are currently using
	SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step+1), ATTR_TEXT_BGCOLOR,
					   VAL_LT_GRAY);
	SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step), ATTR_TEXT_BGCOLOR,
					   VAL_WHITE);
	Scan2Val.Current_Value=cellval;
	ChangedVals = TRUE;

	if(Scan2Val.Current_Value<=-999.0)
	{

		Scan2Val.Current_Step=0;
		ChangeScan1Param=TRUE;

			// read next element of scan list
		GetTableCellVal(panelHandle, PANEL_SCAN_TABLE_2, MakePoint(1,Scan2Val.Current_Step+1), &cellval);

		// indicate which element of the list we are currently using
		SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step+1), ATTR_TEXT_BGCOLOR,
						   VAL_LT_GRAY);
		SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step), ATTR_TEXT_BGCOLOR,
						   VAL_WHITE);
		Scan2Val.Current_Value=cellval;
		ChangedVals = TRUE;

	}

	if((ChangeScan1Param)||(ScanVal.Current_Step<0))
	{  //CHANGE TO NEXT SCAN ONE PARAMETER
		ChangeScan1Param=FALSE;

		ScanVal.Current_Iteration++;
		ScanVal.Current_Step++;
		// read next element of scan list
		GetTableCellVal(panelHandle, PANEL_SCAN_TABLE, MakePoint(1,ScanVal.Current_Step+1), &cellval);

		// indicate which element of the list we are currently using
		SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step+1), ATTR_TEXT_BGCOLOR,
						   VAL_LT_GRAY);
		SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE,MakePoint(1,ScanVal.Current_Step), ATTR_TEXT_BGCOLOR,
						   VAL_WHITE);
		ScanVal.Current_Value=cellval;
		ChangedVals = TRUE;

		//check for end condition
		// Scanning ends if we program -999 into a cell of the Scan List
		if (ScanVal.Current_Value<=-999.0)
		{
			PScan.ScanDone=TRUE;
			SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,ScanVal.Current_Step), ATTR_TEXT_BGCOLOR,
						   VAL_WHITE);
			SetTableCellAttribute (panelHandle, PANEL_SCAN_TABLE_2,MakePoint(1,Scan2Val.Current_Step), ATTR_TEXT_BGCOLOR,
						   VAL_WHITE);

			}
	}



	//insert current scan values into the tables , so they are included in the next BuildUpdateList
	switch(PScan.ScanMode)
	{
		case 0:// Analog value
			AnalogTable[cx][cy][cz].fval=ScanVal.Current_Value;
			AnalogTable[cx][cy][cz].fcn=PScan.Analog.Analog_Mode;
			break;
		case 1:// Time duration
			TimeArray[cx][cz]=ScanVal.Current_Value;
			break;
		case 2:// DDS frequency
			ddstable[cx][cz].amplitude=PScan.DDS.Current;
			ddstable[cx][cz].start_frequency=PScan.DDS.Base_Freq;
			ddstable[cx][cz].end_frequency=ScanVal.Current_Value;
			break;
		case 3: // DDS offset/floor
			SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,ScanVal.Current_Value);
			break;
		case 4: //LaserFreq
			LaserTable[cy-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx][cz].fval=ScanVal.Current_Value;
			break;
		case 5: // SRS Frequency
			SetCtrlVal(panelHandle,PANEL_SRS_FREQ,ScanVal.Current_Value);
			break;

	}

	switch(PScan2.ScanMode)
	{
		case 0:// Analog value
			AnalogTable[cx2][cy2][cz2].fval=Scan2Val.Current_Value;
			AnalogTable[cx2][cy2][cz2].fcn=PScan2.Analog.Analog_Mode;
			break;
		case 1:// Time duration
			TimeArray[cx2][cz2]=Scan2Val.Current_Value;
			break;
		case 2:// DDS frequency
			ddstable[cx2][cz2].amplitude=PScan2.DDS.Current;
			ddstable[cx2][cz2].start_frequency=PScan2.DDS.Base_Freq;
			ddstable[cx2][cz2].end_frequency=Scan2Val.Current_Value;
			break;
		case 3: // DDS offset/floor
			SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,Scan2Val.Current_Value);
			break;
		case 4: //LaserFreq
			LaserTable[cy2-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx2][cz2].fval=Scan2Val.Current_Value;
			break;
		case 5: // SRS Frequency
			SetCtrlVal(panelHandle,PANEL_SRS_FREQ,Scan2Val.Current_Value);
			break;

	}


	// Record current scan information into a string buffer, so we can write it to disk later.
	GetSystemTime(&hour,&minute,&second);
	Scan2Buffer[counter].Step1=ScanVal.Current_Step;
	Scan2Buffer[counter].Iteration1=ScanVal.Current_Iteration;
	Scan2Buffer[counter].Value1=ScanVal.Current_Value;
	Scan2Buffer[counter].Step2=Scan2Val.Current_Step;
	Scan2Buffer[counter].Iteration2=Scan2Val.Current_Iteration;
	Scan2Buffer[counter].Value2=Scan2Val.Current_Value;
	Scan2Buffer[0].BufferSize=counter;
	sprintf(ScanBuffer[counter].Time,"%d:%d:%d",hour,minute,second);
	counter++;

	// display current scan parameters on screen
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANVAL, ScanVal.Current_Value);
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANSTEP, ScanVal.Current_Step);
	SetCtrlVal (panelHandle_sub2, SUBPANEL2_NUM_SCANITER, ScanVal.Current_Iteration);



    // if the scan is done, then cleanup and write the starting values back into the tables
	if(PScan.ScanDone==TRUE)
	{   // reset initial values in the tables
		switch(PScan.ScanMode)
		{
			case 0:// Analog value
				AnalogTable[cx][cy][cz].fval=PScan.Analog.Start_Of_Scan;
				break;
			case 1:// Time duration
				TimeArray[cx][cz]=PScan.Time.Start_Of_Scan;
				break;
			case 2:// DDS frequency
				ddstable[cx][cz].end_frequency=PScan.DDS.Start_Of_Scan;
				break;
			case 3: // DDS offset/floor
				SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,PScan.DDSFloor.Floor_Start);
				break;
			case 4:
				LaserTable[cy-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx][cz].fval=PScan.Laser.Start_Of_Scan;
				break;
			case 5:
				SetCtrlVal(panelHandle,PANEL_SRS_FREQ,PScan.SRS.SRS_Start);
				break;

		}

				switch(PScan2.ScanMode)
		{
			case 0:// Analog value
				AnalogTable[cx2][cy2][cz2].fval=PScan2.Analog.Start_Of_Scan;
				break;
			case 1:// Time duration
				TimeArray[cx2][cz2]=PScan2.Time.Start_Of_Scan;
				break;
			case 2:// DDS frequency
				ddstable[cx2][cz2].end_frequency=PScan2.DDS.Start_Of_Scan;
				break;
			case 3: // DDS offset/floor
				SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,PScan2.DDSFloor.Floor_Start);
				break;
			case 4:
				LaserTable[cy2-NUMBERANALOGCHANNELS-NUMBERDDS-1][cx2][cz2].fval=PScan2.Laser.Start_Of_Scan;
				break;
			case 5:
				SetCtrlVal(panelHandle,PANEL_SRS_FREQ,PScan2.SRS.SRS_Start);
				break;

		}

		//hide the information panel
		SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,0);
		ExportScan2Buffer();// prompt to write out information

		//Redraw Table
		ChangedVals = TRUE;
		DrawNewTable(1);
	}

}

//*****************************************************************************************
// 2012-10-06 --- Stefan Trotzky --- Started:V16.0.0
// Including the Capability to scan multiple parameters from a list.
// Feeds on the previous implementation of one- and two-parameter scans.
// Extended by the capability to scan digital channels as well.
//*****************************************************************************************
void UpdateMultiScanValues(int Reset)
{
  	int i, j, d;

  	int page, col, row, drow;
  	double value;
  	int numPars;

  	int hour, minute, second;

    char outputBuffer[512];
	char paramsBuffer[512];

	char outputFileNameWithPath[MAX_PATHNAME_LEN];
	int incrOutputFileHandle;


	// Info panel for scan progress (not used for multiscan for now ...)
	// SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,1);

	// This line should be void, since there shouldn't be a way to get here with
	// another setting of this value (have it in for button-based testing)
	parameterscanmode = 0;

	printf("Starting UpdateMultiScanValues function\n");

	sprintf(outputBuffer, "incremental.txt");
	MakePathname(MultiScan.CommandsFilePath, outputBuffer, outputFileNameWithPath);
	printf("Incremental file name with path:\n");
	printf("-%s-\n", outputFileNameWithPath);

	printf("Finished creating incremental file name\n");

	// Initialization on first iteration
	if(Reset==TRUE)
	{
		//printf("UMSV inside reset==TRUE\n");

		// Reset scan structure
		MultiScan.Done = FALSE;
		MultiScan.Counter = 0;				// is stepped up upon each update (below)
 		MultiScan.CurrentStep = 0;			// is stepped up upon each update (below)
 		MultiScan.CurrentIteration = -1; 	// is stepped up upon each update (below)
 											// CurrentIteration = 0 means "first"

 		// Set the default holding pattern to none or stop instead of repeat.
 		MultiScan.HoldingPattern = 0;

 		MultiScan.SentinelFoundValue = 0.0;

		// Read in number of iterations from numeric control.
		GetCtrlVal(panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, &numPars);
		MultiScan.Iterations = numPars;

		// Read in number of parameters to scan from numeric control.
		GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &numPars);
		MultiScan.NumPars = numPars;

		// Go through parameters and read in positions and first value, check existance,
		// evaluate types and save starting conditions
		for(j=0;j<numPars;j++)
		{
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,1), &page);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,2), &col);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,3), &row);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j+1,1), &value);

			// Initialization of MultiScan structure. Should be done upon pressing  "scan"
			// Saving the information saved in the tables. Update table with first value
			// RFI:: 	Might consider rewriting things like e.g. ramp types upon every update to
			//			secure panel against user input.
			MultiScan.Par[j].Page = page;
			MultiScan.Par[j].Column = col;
			MultiScan.Par[j].Row = row;
			MultiScan.Par[j].CurrentScanValue = value;

			//printf("UMSV inside reset, par: %i\n", j);
			//printf("UMSV page: %i\n", page);
  			//printf("UMSV col: %i\n", col);
  			//printf("UMSV row: %i\n", row);
  			//printf("UMSV value: %f\n", value);




			if (value<=-999.0)// Pathological, yet not impossible.
			{
				if( abs(value - REPEAT_LAST_SENTINEL) < 0.001 )
				{	// If the sentinel is -1111.0, then we should repeat the
					// last scan line before the sentinel again.
					MultiScan.HoldingPattern = 1;
				}
				else if( abs(value - REPEAT_ORIGINAL_SENTINEL) < 0.001 )
				{	// If the sentinel is -1110.0, then we should repeat the
					// original values that were present before the scan.
					MultiScan.HoldingPattern = 2;
				}
				else
				{	// Old code in else statement.
					MultiScan.Done = TRUE;
					// Also set explicitly set holding pattern to none (again).
					MultiScan.HoldingPattern = 0;
				}
				MultiScan.SentinelFoundValue = value;
			}

			// Check for existance of cell in the analog rows or digital rows and read in the
			// values if existing. Note that analog rows contains analog channels,
			// DDS channels, laser channels, and Anritsu channels.
			if (
				(col>0)
				&&
				(col<=NUMBEROFCOLUMNS)
				&&
				(
					(
						(row>=0)// row = 0 is time row which can't have a timescale
						&&
						(row<=NUMBERANALOGROWS)
					)
					||
				 	(
				 		(row>NUMDIGITALSCANOFFSET)// offset row numbers for digital channels
				 		&&
				  		(row<=NUMDIGITALSCANOFFSET+NUMBERDIGITALCHANNELS)
				  	)
				)
				&&
				(page>0)
				&&
				(page<=NUMBEROFPAGES)
			   )
			{

				//printf("UMSV inside reset, inside valid cell\n");

				MultiScan.Par[j].CellExists = TRUE;

				//printf("UMSV inside reset, inside valid cell, cellexists: %i\n", MultiScan.Par[j].CellExists);


				// Evaluate scan type. RFI:: Use switch instead of if
				if (row==0)
				{   // time scan
					MultiScan.Par[j].Type = 0;
					MultiScan.Par[j].Time = TimeArray[col][page];
					//TimeArray[col][page] = value;
				}
				if ((row>=1) && (row<=NUMBERANALOGCHANNELS))
				{   // analog ch scan
					MultiScan.Par[j].Type = 1;
					MultiScan.Par[j].Analog.fcn = AnalogTable[col][row][page].fcn;
					MultiScan.Par[j].Analog.fval = AnalogTable[col][row][page].fval;
					MultiScan.Par[j].Analog.tscale = AnalogTable[col][row][page].tscale;
					//AnalogTable[col][row][page].fval = value;
				}
				if ((row>NUMBERANALOGCHANNELS) && (row<=NUMBERANALOGCHANNELS+NUMBERDDS))
				{   // DDS scan
					MultiScan.Par[j].Type = 2;
					MultiScan.Par[j].DDS.fcn = AnalogTable[col][row][page].fcn;
					MultiScan.Par[j].DDS.tscale = AnalogTable[col][row][page].tscale;
					switch (row-NUMBERANALOGCHANNELS){ // ugly because of the way the ddstables are built
						case 1:
							MultiScan.Par[j].DDS.fval = ddstable[col][page].end_frequency;
							//ddstable[col][page].end_frequency = value;
							break;
						case 2:
							MultiScan.Par[j].DDS.fval = dds2table[col][page].end_frequency;
							//dds2table[col][page].end_frequency = value;
							break;
						case 3:
							MultiScan.Par[j].DDS.fval = dds3table[col][page].end_frequency;
							//dds3table[col][page].end_frequency = value;
							break;
					}
				}
				if ((row>NUMBERANALOGCHANNELS+NUMBERDDS) && (row<=NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS))
				{   // Laser scan
					MultiScan.Par[j].Type = 3;
					MultiScan.Par[j].Laser.fcn = LaserTable[row-NUMBERANALOGCHANNELS-NUMBERDDS-1][col][page].fcn;
					MultiScan.Par[j].Laser.fval = LaserTable[row-NUMBERANALOGCHANNELS-NUMBERDDS-1][col][page].fval;
					//LaserTable[row-NUMBERANALOGCHANNELS-NUMBERDDS-1][col][page].fval = value;
				}
				if ((row>NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS)&&(row<=NUMDIGITALSCANOFFSET))
				{   // Anritsu scan
					MultiScan.Par[j].Type = 4;
					MultiScan.Par[j].Anritsu.fcn = AnalogTable[col][row][page].fcn;
					MultiScan.Par[j].Anritsu.fval = AnalogTable[col][row][page].fval;
					MultiScan.Par[j].Anritsu.tscale = AnalogTable[col][row][page].tscale;
					//AnalogTable[col][row][page].fval = value;
				}
				if ((row>NUMDIGITALSCANOFFSET))
				{   // Digital channel scan -- interprets positive values as high
					MultiScan.Par[j].Type = 9;
					drow = row-NUMDIGITALSCANOFFSET;
					MultiScan.Par[j].Digital = DigTableValues[col][drow][page];
					MultiScan.Par[j].CurrentScanValue = (double)(ToDigital(value)*1.0);
					//DigTableValues[col][drow][page] = ToDigital(value);
				}

				// Highlight first scan value.
				SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j+1,MultiScan.CurrentStep+1),
					ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);

				ChangedVals = TRUE;
			}
			// Check for possible analog cell timescale scans
			else if (
				(col>0)
				&&
				(col<=NUMBEROFCOLUMNS)
				&&
				(row>NUMANALOGTIMESCALEOFFSET)// row = 0 is time row which can't have a timescale
				&&
				(row<=NUMANALOGTIMESCALEOFFSET+NUMBERANALOGROWS)// offset row number by timescale offset
				&&
				(page>0)
				&&
				(page<=NUMBEROFPAGES)
			   )
			{
				// We have a valid cell
				MultiScan.Par[j].CellExists = TRUE;

				// Set the scan type for this scan parameter.
				// Create new type 11 for analog cell timescale.
				MultiScan.Par[j].Type = 11;

				// Save the before scan values in the MultiScan struct.
				MultiScan.Par[j].Analog.fcn = AnalogTable[col][row][page].fcn;
				MultiScan.Par[j].Analog.fval = AnalogTable[col][row][page].fval;
				MultiScan.Par[j].Analog.tscale = AnalogTable[col][row][page].tscale;

			}
			// Check for possible DDS EOR offset scans
			else if (
				(col == DDS_EOR_SCAN_COLUMN_VALUE)
				&&
				(row>NUMBERANALOGCHANNELS)// DDS freq channels start after analog channels
				&&
				(row<=NUMBERANALOGCHANNELS+NUMBERDDS)
				&&
				(page == 1)// Set arbitrarily in DDS_OFFSET_CALLBACK
			   )
			{
				// We have a valid cell
				MultiScan.Par[j].CellExists = TRUE;

				// Set the scan type for this scan parameter.
				// Create new type 22 for DDS EOR offset scan.
				MultiScan.Par[j].Type = 22;

				// Save the before scan values in the MultiScan struct.
				MultiScan.Par[j].DDS.fcn = AnalogTable[col][row][page].fcn;
				MultiScan.Par[j].DDS.tscale = AnalogTable[col][row][page].tscale;
				switch (row-NUMBERANALOGCHANNELS)
				{ // ugly because of the way the ddstables are built
				double ddsEOR;
				case 1:
					GetCtrlVal(panelHandle, PANEL_NUM_DDS_OFFSET, &ddsEOR);
					MultiScan.Par[j].DDS.fval = ddsEOR;
					printf("ddsEOR: %f\n", ddsEOR);
					break;
				case 2:
					//MultiScan.Par[j].DDS.fval = dds2table[col][page].end_frequency;
					GetCtrlVal(panelHandle, PANEL_NUM_DDS2_OFFSET, &ddsEOR);
					MultiScan.Par[j].DDS.fval = ddsEOR;
					printf("dds2EOR: %f\n", ddsEOR);
					break;
				case 3:
					//MultiScan.Par[j].DDS.fval = dds3table[col][page].end_frequency;
					GetCtrlVal(panelHandle, PANEL_NUM_DDS3_OFFSET, &ddsEOR);
					MultiScan.Par[j].DDS.fval = ddsEOR;
					printf("dds3EOR: %f\n", ddsEOR);
					break;
				}
			}
			// Check for possible GPIB value scans
			else if (row>NUMGPIBSCANOFFSET && row<=NUMGPIBSCANOFFSET+NUMBERGPIBDEV
				&& col>=1 && col<=NUMGPIBPROGVALS)
			{
				SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j+1,MultiScan.CurrentStep+1),
					ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);

				MultiScan.Par[j].CellExists = TRUE;
				MultiScan.Par[j].Type = 5;
				MultiScan.Par[j].GPIBvalue = GPIBDev[row-NUMGPIBSCANOFFSET-1].value[col-1];
				//GPIBDev[row-NUMGPIBSCANOFFSET-1].value[col-1] = value;
				strcpy(GPIBDev[row-NUMGPIBSCANOFFSET-1].lastsent,"VOID/0"); // ensure reprogramming

				ChangedVals = TRUE;
			}
			else
			{
				MultiScan.Par[j].CellExists = FALSE; // If the cell specified does not exist, it will be skipped
			}

			//printf("UMSV inside reset, after checking exist, par: %i, exist: %i\n", j, MultiScan.Par[j].CellExists);

		} // Done reading individual parameter settings


		//printf("UMSV done reading individual param settings\n");

		// Check if we should set the next scan or not.
		// If the first value was >-999.0 ie. a valid value then we should set each scan
		// table column the normal way.
		// If the first value was not a holding sentinel, then MultiScan.Done is TRUE and we
		// don't need to set anything because it will just be set back at the end of this
		// function (that part has not changed and in this case is redundant but whatever).
		// If the first value was a holding sentinel, then regardless of the sentinel value
		// there is no previous scan line to repeat and so we must repeat the initial values.
		if( MultiScan.Done == FALSE )
		{	// This should only be true if the first scan line does not have any full stop
			// sentinel values. Full stop as opposed to holding pattern senntinel values.

			//printf("UMSV if reset FALSE, if MultiScan.Done FALSE\n");

			// Now check for holding pattern sentinel values.
			// The only possible holding pattern with a holding sentinel in the first scan line
			// is to repeat the initial values.
			// We shouldn't need to actually do anything, but I'm paranoid, so let's be explicit.
			// Note that we are also specifying the MultiScan.Par[j].CurrentScanValue each
			// time to be sure that when we write to the ScanBuffer and the incremental file that
			// the value there is correct.
			if( MultiScan.HoldingPattern > 0 )
			{
				//printf("UMSV if reset FALSE, if MultiScan.Done FALSE, if holding pattern TRUE\n");

				updateScannedCellsWithOriginalValues();
				ChangedVals = TRUE;

				for(j=0;j<MultiScan.NumPars;j++)
				{
					if (MultiScan.Par[j].CellExists)
					{
					// Highlight current first scan line cell.
					SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
						MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);
					}
				}

				// Decrement MultiScan.CurrentStep so the next time we reach the max iteractions
				// we will increment to the holding sentinel value again.
				MultiScan.CurrentStep--;

			}
			else // There is no holding sentinel value, and so we should update normally.
			{

				//printf("UMSV if reset FALSE, if MultiScan.Done FALSE, if holding pattern FALSE\n");

				updateScannedCellsWithScanTableLine(MultiScan.CurrentStep+1);
				ChangedVals = TRUE;

				for(j=0;j<MultiScan.NumPars;j++)
				{
					if (MultiScan.Par[j].CellExists)
					{
						// Highlight current cell, no previous cell to remove highlighting from.
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);
					}
				}
			}

		}

		//printf("UMSV if reset FALSE, done initial setting\n");


		// Reset ScanBuffer to zero
		// The ScanBuffer is needed, since the scanlist can be edited during the scan
		for( i=0; i<SCANBUFFER_LENGTH; i++)
		{
		 	ScanBuffer[i].Step = 0;
		 	ScanBuffer[i].Iteration = 0;
		 	for(j=0;j<NUMMAXSCANPARAMETERS;j++)
		 	{
		 		ScanBuffer[i].MultiScanValue[j] = 0.0;
		 	}
		}


 		// Set to 'Initialized' and activate scan (functions check for
 		//'MultiScan.Activated' to do MultiScan stuff).
 		MultiScan.Initialized = TRUE;
 		MultiScan.Active = TRUE;

 		// Disable editing number of parameters, number of iterations and
 		// parameter positions by dimming out the respective controls
 		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NAMES_TABLE, ATTR_DIMMED, 1);

 		//printf("UMSV done reset\n");

  	}
  	//Done initializing/resetting the MultiScan structure.

  	//printf("UMSV after reset\n");


  	// Update next scan value. First thing: step up iteration counter
  	// and only read a new value once the iterations are completed

  	//printf("UMSV update\n");
  	//printf("UMSV CurrentIteration: %i\n", MultiScan.CurrentIteration);
  	MultiScan.CurrentIteration++;
  	//printf("UMSV CurrentIteration incremented: %i\n", MultiScan.CurrentIteration);


  	if( MultiScan.CurrentIteration >= MultiScan.Iterations )
  	{	// Go to next scan value.
  		//printf("UMSV if current iter >= iterations\n");
  		//printf("UMSV currentStep: %i\n", MultiScan.CurrentStep);

  		MultiScan.CurrentStep++;
  		MultiScan.CurrentIteration = 0;

  		//printf("UMSV currentStep: %i\n", MultiScan.CurrentStep);

  		// Iterate through the scan table line to see if there is a sentinel.
  		MultiScan.HoldingPattern = 0;
  		for(j=0;j<MultiScan.NumPars;j++)
  		{
  			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
					MakePoint(j+1,MultiScan.CurrentStep+1), &value);
			if( value <= -999.0 ){
				if( abs(value - REPEAT_LAST_SENTINEL) < 0.001 ){
					MultiScan.HoldingPattern = 1;
				}
				else if( abs(value - REPEAT_ORIGINAL_SENTINEL) < 0.001 ){
					MultiScan.HoldingPattern = 2;
				}
				else {
					MultiScan.Done = TRUE;
					MultiScan.HoldingPattern = 0;
				}
				MultiScan.SentinelFoundValue = value;
			}
  		}

  		//printf("UMSV after iterating scan table line for sentinel\n");
  		//printf("UMSV holdpatt: %i\n", MultiScan.HoldingPattern);
  		//printf("UMSV scandone: %i\n", MultiScan.Done);
  		//printf("UMSV currentStep: %i\n", MultiScan.CurrentStep);

  		// If MultiScan.Done == TRUE then we don't need to write anything since the values will
  		// be overwritten at the end of this function.
  		if( MultiScan.Done == FALSE ){

  			//printf("UMSV iter passed, done FALSE\n", MultiScan.Done);


  			// Now if there was a holding pattern sentinel value in this line then we should
  			// write the appropriate values.
  			if( MultiScan.HoldingPattern == 1 ){// Repeat the previous line in the scan table.

				//printf("UMSV iter passed, done FALSE, holding patt 1\n", MultiScan.Done);

				for( j=0; j<MultiScan.NumPars; j++ )
				{
					if (MultiScan.Par[j].CellExists)
					{
						// Highlight current cell, remove highlighting on previous one.
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep), ATTR_TEXT_BGCOLOR, VAL_WHITE);
					}
				}

				// Decrement MultiScan.CurrentStep and this time let's try just keeping the same
				// cell values in the sequence. It should work.
				MultiScan.CurrentStep--;

				ChangedVals = TRUE;// For consistency. Shouldn't actually matter other than timing.
			}
			else if( MultiScan.HoldingPattern == 2 ){// Repeat the original values.

				//printf("UMSV iter passed, done FALSE, holding patt 2\n", MultiScan.Done);

				updateScannedCellsWithOriginalValues();
				ChangedVals = TRUE;

				for(j=0;j<MultiScan.NumPars;j++)
				{
					if (MultiScan.Par[j].CellExists)
					{
						// Highlight current cell, remove highlighting on previous one.
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep), ATTR_TEXT_BGCOLOR, VAL_WHITE);
					}
				}

				// Decrement MultiScan.CurrentStep so the next time we reach the max iteractions
				// we will increment to the holding sentinel value again.
				MultiScan.CurrentStep--;

			}
			else {// Else there is no holding pattern sentinel and we should update normally.

				//printf("UMSV iter passed, done FALSE, holding patt else\n", MultiScan.Done);
  				//printf("UMSV currentStep(+1 to row index): %i\n", MultiScan.CurrentStep);
  				//printf("UMSV iterate NumPars: %i\n", MultiScan.NumPars);


  				updateScannedCellsWithScanTableLine(MultiScan.CurrentStep+1);
  				ChangedVals = TRUE;

				for(j=0;j<MultiScan.NumPars;j++)
				{
					if (MultiScan.Par[j].CellExists)
					{
						// Highlight current cell, remove highlighting on previous one.
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_LT_GRAY);
						SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
							MakePoint(j+1,MultiScan.CurrentStep), ATTR_TEXT_BGCOLOR, VAL_WHITE);

						//printf("UMSV set highlighting of cell %i %i\n", j+1, MultiScan.CurrentStep+1);
					}
				}

			}// End choosing which kind of update to do, holdings or normal.

  		}// End MultiScan.Done == FALSE if statement.
  		else {
  			// If we are done the scan, then don't write to any of the actual arrays but we
  			// will write to the CurrentScanValue for each par so that the output files have
  			// the last sentinel as the last line in the file. As it was before, so it shall be.
  			for( j=0; j<MultiScan.NumPars; j++ )
			{
				if (MultiScan.Par[j].CellExists)
				{
					// Read next parameter value from table control.
					GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
						MakePoint(j+1,MultiScan.CurrentStep+1), &value);
					MultiScan.Par[j].CurrentScanValue = value;
				}
			}
  		}
	}
	else
	{
		// No updates to be done.
	}
	// Done updating table values according to scan list.


	//printf("UMSV Before writing to ScanBuffer\n");

	// Record current scan information into a string buffer,
	// so we can write it to disk later. Includes writing out the sentinel.
	GetSystemTime(&hour,&minute,&second);
	if( MultiScan.Counter < SCANBUFFER_LENGTH ){
		ScanBuffer[MultiScan.Counter].Step = MultiScan.CurrentStep;
		ScanBuffer[MultiScan.Counter].Iteration = MultiScan.CurrentIteration;
		for (j=0;j<MultiScan.NumPars;j++)
		{
			ScanBuffer[MultiScan.Counter].MultiScanValue[j] = MultiScan.Par[j].CurrentScanValue;
		}
		ScanBuffer[0].BufferSize=MultiScan.Counter;
		sprintf(ScanBuffer[MultiScan.Counter].Time,
				"%02d:%02d:%02d",hour,minute,second);
	}
	else {
		printf("Number of scan buffer lines exceeded. The mscan file will be missing entries.\n");
	}

	//printf("UMSV After writing to ScanBuffer\n");
	//printf("UMSV Before writing to incremental file\n");

	// Always write to incremental file.
	incrOutputFileHandle = OpenFile(outputFileNameWithPath,VAL_READ_WRITE,VAL_APPEND,VAL_ASCII);
	// If we can't open the incremental file, continue, but print an error.
	if( incrOutputFileHandle < 0){
		printf("Error opening incremental file:\n-%s-\n", outputFileNameWithPath);
		CloseFile(incrOutputFileHandle);
	}
	else {
		// Assemble the line to write to the incremental file.
		sprintf(outputBuffer,"%i ", MultiScan.Counter);
		for( j=0; j<MultiScan.NumPars; j++ ){
			sprintf(paramsBuffer, "%f ", MultiScan.Par[j].CurrentScanValue);
			strcat(outputBuffer, paramsBuffer);
		}
		sprintf(paramsBuffer, "%i %i %02i:%02i:%02i",
				MultiScan.CurrentStep, MultiScan.CurrentIteration, hour, minute, second);
		strcat(outputBuffer, paramsBuffer);
		WriteLine(incrOutputFileHandle, outputBuffer, -1);
		CloseFile(incrOutputFileHandle);
	}

	//printf("UMSV After writing to incremental file\n");

	MultiScan.Counter++;
	// Done updating the ScanBuffer.


    // Check if the scan is done. If yes, then cleanup.
    // MultiScan.CurrentStep+1 should point to the line with the sentinel at this point.
	if (MultiScan.Done==TRUE)
	{
		if( abs(MultiScan.SentinelFoundValue - STOP_REPLACE_LAST_SENTINEL) < 0.001 ){
			// Check that there is a previous line to copy from.
			if( MultiScan.CurrentStep >= 1 ){
				//printf("UMSV Replace scanned cells with scan table line\n");
				updateScannedCellsWithScanTableLine(MultiScan.CurrentStep);
			}
			else {// Else just replace with original values.
				//printf("UMSV Replace scanned cells with original values\n");
				updateScannedCellsWithOriginalValues();
			}
		}
		// A bit of future proofing with this else if statement.
		else if( abs(MultiScan.SentinelFoundValue - STOP_REPLACE_ORIGINAL_SENTINEL) < 0.001 ){
			//printf("UMSV Replace scanned cells with original values\n");
			updateScannedCellsWithOriginalValues();
		}
		else {
			//printf("UMSV Replace scanned cells with original values\n");
			updateScannedCellsWithOriginalValues();
		}

		ChangedVals = TRUE;

		for(j=0;j<MultiScan.NumPars;j++)
		{
			// remove highlighting from scan list value.
			SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
				MakePoint(j+1,MultiScan.CurrentStep), ATTR_TEXT_BGCOLOR, VAL_WHITE);
			SetTableCellAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE,
				MakePoint(j+1,MultiScan.CurrentStep+1), ATTR_TEXT_BGCOLOR, VAL_WHITE);
		}
	}

}

//*********************************************************************
// 2016_07_31 --- Scott Smale --- Started in Version 16.1.E
// Check for the next commands file and apply the commands.
//*********************************************************************
void GetNewMultiScanCommands(void)
{
	int i, j;
	int	numRows, numCols;
	double cellValue;
	double sentinelValue;
	int sentinelFound;
	int n;

	int fileExists = 0;
	char inputFileNameWithPath[MAX_PATHNAME_LEN];
	int commandsInputFileHandle;
	char outputFileNameWithPath[MAX_PATHNAME_LEN];
	int changesOutputFileHandle;

	char inputBuffer[500];
	char outputBuffer[500];
	char paramsBuffer[500];


	// A bit of nomenclature: There needs to be a distinction between the number of lines in the
	// scan table that correspond to valid scan points and the number of lines/rows in the scan
	// table object itself. I propose to use "entries" or "lines" to refer to rows of the scan
	// table that are valid scan points (including the sentinel) and to use "rows" to refer to
	// the rows of the scan table object.
	int numLinesInScanTable;// Should start at 1 and include the sentinel value.
	int matlabNumLines;// How many lines that matlab thinks is in the scan table.
	char commandTypeBuffer[10];
	int commandNum;// The line in the commands_00000.txt file of the current command.
	int commandScanTableLine;// The line that the command should affect.
	double parValues[NUMMAXSCANPARAMETERS];

	//printf("GN Starting GetNewMultiScanCommands function\n");

	// Create the appropriate commands file name to open including the directory path.
	sprintf(inputBuffer, "commands_%05d.txt", MultiScan.NextCommandsFileNumber);
	MakePathname(MultiScan.CommandsFilePath, inputBuffer, inputFileNameWithPath);
	//printf("GN Commands file name with path:\n");
	//printf("-%s-\n", inputFileNameWithPath);

	// Create the file name of the changes file.
	sprintf(outputBuffer, "changes_%05d.txt", MultiScan.NextCommandsFileNumber);
	MakePathname(MultiScan.CommandsFilePath, outputBuffer, outputFileNameWithPath);
	//printf("GN Changes file name with path:\n");
	//printf("-%s-\n", outputFileNameWithPath);

	//printf("GN Checking if commands file %i exists\n", MultiScan.NextCommandsFileNumber);
	fileExists = FileExists(inputFileNameWithPath, 0);// Returns 1 if file exists, 0 if file not found. Negative on errors.
	//printf("GN Done checking if commands file exists\n");
	if( fileExists == 1 ){
		// We have a commands file to read so let's open it.
		commandsInputFileHandle = OpenFile(inputFileNameWithPath,VAL_READ_ONLY,VAL_OPEN_AS_IS,VAL_ASCII);
		if( commandsInputFileHandle < 0){
			printf("Error opening commands file:\n-%s-\n", inputFileNameWithPath);
			CloseFile(commandsInputFileHandle);
			return;// If the file exists but we can't open it, try again next time.
		}

		// Open changes_00000.txt file for writing.
		changesOutputFileHandle = OpenFile(outputFileNameWithPath,VAL_READ_WRITE,VAL_TRUNCATE,VAL_ASCII);
		if( changesOutputFileHandle < 0){
			printf("Error opening changes file:\n-%s-\n", outputFileNameWithPath);
			CloseFile(changesOutputFileHandle);
			return;
		}
	}
	else {// There is no such file, then no need to do the rest of this function.
		//printf("GN No commands file, returning from function\n");
		return;// Return without incrementing MultiScan.NextCommandsFileNumber.
	}

	//printf("GN Commands and changes files have been opened\n");

	// At this point we know that there is a commands_00000.txt file to read commands from and
	// that we have a file changes_00000.txt to write to for the log.

	// Read the header line of the commands file.
	// At the moment should only include the number of lines in the scan table.
	ReadLine(commandsInputFileHandle, inputBuffer, 500);

	//printf("GN after readline\n");

	Scan(inputBuffer, "%s>%i", &matlabNumLines);
	//Scan(inputBuffer, "%s>Scan Table Lines: %i", &matlabNumLines);
	//ScanFile(commandsInputFileHandle, "%s>%i", &matlabNumlines);

	//printf("GN after scan header\n");


	// Block the user from changing values in the scan table.
	SetCtrlAttribute( panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_DIMMED, 1);
	// And also block the user from changing the number of rows in the scan table.
	// The callback MULTISCAN_NUMROWS_CALLBACK does not allow the number of rows to be
	// decreased while MultiScan.Active == FALSE so it should be safe to dim even if the
	// interruped value is tried to be taken as the new value.
	SetCtrlAttribute( panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_DIMMED, 1);

	//printf("GN after dimming panels\n");


	// Find the sentinel in the scan table.
	// Note: if the user was making changes and overwrote the sentinel before finishing
	// her changes, then we will call the sentinel the end of the table and it will
	// obviously be different than what matlab expects and so we will disregard matlab's
	// commands and allow the user to finish the changes they were doing.
	GetNumTableRows( panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numRows);
	GetNumTableColumns( panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numCols);

	//printf("GN before search sentinel, numRows: %i, numCols: %i\n", numRows, numCols);

	for( i=1; i<=numRows; i++ ){
		for( j=1; j<=numCols; j++ ){
			GetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j,i), &cellValue);
			if( cellValue <= -999.0 ){
				break;
			}
		}
		if( cellValue <= -999.0 ){
			break;
		}
	}
	if( cellValue <= -999.0 ){
		sentinelFound = 1;
		numLinesInScanTable = i;
		sentinelValue = cellValue;
	}
	else {
		sentinelFound = 0;
		numLinesInScanTable = numRows;// Technically true, but we won't be doing anything.
	}

	//printf("GN after search sentinel, found: %i, value: %f\n", sentinelFound, sentinelValue);

	//printf("GN matlabNumLines: %i\n", matlabNumLines);
	//printf("GN numLinesInScanTable: %i\n", numLinesInScanTable);

	// If matlab has the wrong count, then it was probably due to user interference.
	// In this case, disregard the matlab commands. Also require prescence of a sentinel value
	// somewhere in the scan table rows, otherwise ignore the matlab commands.
	if( matlabNumLines == numLinesInScanTable && sentinelFound){

		//printf("GN num lines okay\n");


		// Since the count is the same, write to the output file that we will try each command.
		sprintf(outputBuffer, "%i %s",
				numLinesInScanTable, "Matlab number of lines correct.");
		WriteLine(changesOutputFileHandle, outputBuffer, -1);

		//printf("GN wrote changes file num lines okay\n");


		// Start reading the rest of the lines from the command file.
		// At the moment we will make changes as we go.

		// ReadLine will return -2 if already at EOF, -1 for IO error, 0 for an empty
		// line, and the number of bytes read otherwise.
		// ReadLine appends a trailing '\0' to the end of the bytes read.
		// The newline symbol is not included in the buffer.

		//printf("GN start read lines\n");

		while( ReadLine(commandsInputFileHandle, inputBuffer, 500-1) >= 0 ){

			//printf("GN rl top while loop\n");
			//printf("GN inputbuffer:\n");
			//printf(">%s<\n", inputBuffer);

			if( inputBuffer[0] == '\0' ){
				// If a blank line, try reading another.
				// Don't worry about writing to the changes file for an empty line.
				continue;
			}

			//printf("GN not an empty line\n");

			// Now we have a line with parameters in it.
			// What kind of commands can matlab ask?
			// Append the line to the end of the scan table lines.
			// Replace a line in the middle of the scan table (in the future or else error).
			// Replacing a line can also mean putting a sentinel there.
			// Possibly allow inserts which shift all lines after that one.
			// Define 'A' for append, 'R' for replace, and 'I' for insert.
			// A valid command for appending to a K optimization scan might be:
			// "5 A -1 4.5 0.8 0.1"
			// A valid command for replacement would be:
			// "6 R 18 4.5 0.8 0.1"
			// Where the second value is the line in the scan table to replace which is
			// meaningless for the append. (Could probably get away without characters
			// with just -1 for appending and assume >0 is a replacement but let's allow
			// for future needs including inserting a line.)


			// First parse the line and extract the command number, command type, the line,
			// and the rest of the values.
			// Notes about this Scan call:
			// wn writes at most n bytes to the char array excluding NULL termination.
			//		n here is one minus buffer size eg 10-1=9
			// numCols is set earlier and is an argument that replaces the * in %*f
			// Scan treats the array %*f as a single target. It is satisfied if at least one
			// element of the array is filled in.
			//		Therefore the number of satisfied elements for a successful conversion is
			//		1+1+1+1 without any consideration of numCols.
			n = Scan(inputBuffer, "%s>%i%s[w9q]%i%*f",
						&commandNum,
						commandTypeBuffer,
						&commandScanTableLine,
						numCols, parValues);
			// Check that we have parsed the expected number of things.

			//printf("GN scanned line\n");

			if( n != 1+1+1+1 ){
				// If we do not have the correct number of things, write a message to the
				// changes file and read the next line.
				sprintf(outputBuffer, "%i %s%s%i%s%i%s%i",
						-1, "Failed to read expected number of things on a line. ",
						"Possibly line: ", commandNum,
						", Read this many targets: ", n,
						", Expected this many: ", 1+1+1+1);
				WriteLine(changesOutputFileHandle, outputBuffer, -1);
				continue;
			}

			//printf("GN line has correct num params\n");

			// Now that we have parsed the string, check which command we are to execute.
			if( strncmp(commandTypeBuffer, "A", 1) == 0 ){

				//printf("GN command A\n");

				// We append (which includes overwriting the sentinel value and rewriting
				// the sentinel in the next line).

				// Since an outer if statement checks for sentinelFound, if numLinesInScanTable
				// is equal to numRows then there is no next row to move the sentinel value
				// to and so we will write a message to the changes file and read the next line.
				if( numLinesInScanTable == numRows ){
					sprintf(outputBuffer, "%i %s%i",
							commandNum, "Max number of lines reached: ", numRows);
					WriteLine(changesOutputFileHandle, outputBuffer, -1);
					continue;// So read a new line.
				}

				//printf("GN A, parValues[0]: %f\n", parValues[0]);
				//printf("GN A, row index: %i\n", numLinesInScanTable);

				// For each of the parameters, replace the corresponding column in the row.
				for( i=1; i<=numCols; i++ ){
					SetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE,
									MakePoint(i,numLinesInScanTable), parValues[i-1]);
				}

				// Increment the number of lines in the scan table.
				numLinesInScanTable++;

				// Now numLinesInScanTable corresponds to the row that was after the
				// sentinel line so write a new sentinel to this row, making it a line.
				// Write the sentinel in the first column regardless of which column it was
				// in before.
				SetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE,
									MakePoint(1,numLinesInScanTable), sentinelValue);

				// Write to the changes_00000.txt files that we have appended the values.
				sprintf(outputBuffer, "%i %s %i ",
						commandNum, "A", commandScanTableLine);
				for( i=0; i<numCols-1; i++ ){// numCols-1 to not append a space to the last value.
					sprintf(paramsBuffer, "%f ", parValues[i]);
					strcat(outputBuffer, paramsBuffer);
				}
				sprintf(paramsBuffer, "%f", parValues[numCols-1]);
				strcat(outputBuffer, paramsBuffer);
				WriteLine(changesOutputFileHandle, outputBuffer, -1);

			}
			else if( strncmp(commandTypeBuffer, "R", 1) == 0 ){

				//printf("GN command R\n");

				// We replace.

				// A replacement can be any line between the next line that is about to be
				// executed inclusive to the sentinel value inclusive. If the sentinel value is
				// to be replaced we will move it to the next row (becomes a line) as long as we
				// are not at the end of the scan table rows.
				// The function UpdateMultiScanValues should be called immediately after this
				// one and so assuming the number of iterations is one, we can query
				// MultiScan.CurrentStep to find the next line to be exectued in the scan table.

				// All of the indexes into the scan table in UpdateMultiScanValues function are
				// MultiScan.CurrentStep+1 since CurrentStep is zero-based.
				// The line number (aka row index one-based) of the scan table that is about to
				// be put into the cells for the next cycle is MultiScan.CurrentStep+1. Since
				// that is the next shot, it is the earliest possibility for replacement.
				if( commandScanTableLine >= MultiScan.CurrentStep+1 ){
					// Now check that the line to be replaced is less than or equal to the number
					// of lines in the scan table.
					if( commandScanTableLine <= numLinesInScanTable ){
						// Finally check that we will have room to move the sentinel down a row.
						if( commandScanTableLine < numRows ){
							// Okay, now we have a replacement line that is valid.

							// For each of the parameters in the command file line, replace
							// the corresponding column in the scan table.
							for( i=1; i<=numCols; i++ ){
								SetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE,
											MakePoint(i,commandScanTableLine), parValues[i-1]);
							}

							// Check if we need to shift the sentinel value because we just
							// overwrote it.
							// Write the sentinel in the first column regardless of which column it was
							// in before.
							if( commandScanTableLine == numLinesInScanTable ){
								numLinesInScanTable++;
								SetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE,
											MakePoint(1,numLinesInScanTable), sentinelValue);
							}

							// Write to the changes_00000.txt files that we have done a replacement.
							sprintf(outputBuffer, "%i %s %i ",
								commandNum, "R", commandScanTableLine);
							for( i=0; i<numCols-1; i++ ){// numCols-1 to not append a space to the last value.
								sprintf(paramsBuffer, "%f ", parValues[i]);
								strcat(outputBuffer, paramsBuffer);
							}
							sprintf(paramsBuffer, "%f", parValues[numCols-1]);
							strcat(outputBuffer, paramsBuffer);
							WriteLine(changesOutputFileHandle, outputBuffer, -1);
						}
						else {
							// No rows to shift sentinel value to, so do not do replacement
							// and write this fact to changes file.
							sprintf(outputBuffer, "%i %s%s%s%i%s%i",
									commandNum, "Replacement of sentinel line would shift ",
									" sentinel value beyond the end of the scan table. ",
									"Number of rows in scan table: ", numRows,
									", asked replacement line: ", commandScanTableLine);
							WriteLine(changesOutputFileHandle, outputBuffer, -1);
							continue;
						}
					}
					else {
						// Replacement would be after the sentinel value and so would not happen.
						// Do not do replacement and write this fact to changes file.
						sprintf(outputBuffer, "%i %s%s%i%s%i",
								commandNum, "Replacement would be after sentinel value. ",
								"Sentinel line: ", numLinesInScanTable,
								", asked replacement line: ", commandScanTableLine);
						WriteLine(changesOutputFileHandle, outputBuffer, -1);
						continue;
					}
				}
				else {
					// Replacement would be in the past. Do not do replacement and write error
					// to changes file.
					sprintf(outputBuffer, "%i %s%s%i%s%i",
							commandNum, "Replacement would be in the past. ",
							"Cycle about to be started is line: ", MultiScan.CurrentStep+1,
							", asked replacement line: ", commandScanTableLine);
					WriteLine(changesOutputFileHandle, outputBuffer, -1);
					continue;
				}
			}
			else if( strncmp(commandTypeBuffer, "I", 1) == 0 ){

				//printf("GN command I\n");

				// We insert.

				// Try doing the insert by inserting rows (lines) in the table?
				// This would extend the scan table which might run out of room.
				printf("Insert not implemented yet (probably never)\n");
				continue;
			}
			else {

				//printf("GN invalid command\n");

				// Invalid command type. Write a message to the log file and read the next line.
				sprintf(outputBuffer, "%i %s",
						commandNum, "Invalid command type.");
				WriteLine(changesOutputFileHandle, outputBuffer, -1);
				continue;
			}

			//printf("GN bottom while loop\n");

		// We have read the command from the file and either it was parsed correctly and applied,
		// or it wasn't either way there is nothing more to do so read the next line by closing
		// the while loop brace.
		}
	// Also close the if statement for matlab knowing the correct number of lines and that there
	// was a sentinel in the scan table rows.
	}
	else {
		//printf("GN num lines bad\n");


		// Write to changes file that matlab's count was incorrect.
		sprintf(outputBuffer, "%i %s%i%s",
				numLinesInScanTable,
				"Matlab number of lines NOT correct: ", matlabNumLines,
				"\nNot evaluating this commands file.");
		WriteLine(changesOutputFileHandle, outputBuffer, -1);
	}

	//printf("GN after num lines if stmt\n");


	// Procedure should be for matlab to write a new commands file with incremented number
	// regardless of success or failure. Increment the number of the commands_00000.txt
	// file that we will look for.
	MultiScan.NextCommandsFileNumber++;

	// Close the input and output files.
	CloseFile(commandsInputFileHandle);
	CloseFile(changesOutputFileHandle);


	// As the last thing, allow the user to change the scan table. Note that the update multi
	// scan function should always happen next and then the RunOnce function should be called
	// which should mean that the user can't change values until the next scan line cycle has
	// been started. This is a good thing.
	SetCtrlAttribute( panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_DIMMED, 0);
	SetCtrlAttribute( panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_DIMMED, 0);

	//printf("GN end GNMSC function\n");

}


//*********************************************************************
// 2016_08_04 --- Scott Smale --- Started in Version 16.1.E
// Separate out the action of changing values from the logic of which
// set of values to change to.
//*********************************************************************
void updateScannedCellsWithScanTableLine(int scanTableLine){
	// scanTableLine normally is MultiScan.CurrentStep+1

	int j;
	int page, col, row, drow;
	double value;

	for(j=0;j<MultiScan.NumPars;j++)
	{   // Read in values from scan list and update tables.

		//printf("USCWSTL next par: %i\n", j);
		//printf("USCWSTL cell exists: %i\n", MultiScan.Par[j].CellExists);

		if (MultiScan.Par[j].CellExists)
		{
			//printf("USCWSTL par %i cell exists\n", j);

			// Just for easier code reading.
			page = MultiScan.Par[j].Page;
			col = MultiScan.Par[j].Column;
			row = MultiScan.Par[j].Row;

			// Read next parameter value from table control.
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
				MakePoint(j+1,scanTableLine), &value);
			MultiScan.Par[j].CurrentScanValue = value;

			//printf("USCWSTL page: %i\n", page);
			//printf("USCWSTL col: %i\n", col);
			//printf("USCWSTL row: %i\n", row);
			//printf("USCWSTL value: %f\n", value);

			//printf("USCWSTL type: %i\n", MultiScan.Par[j].Type);

			switch (MultiScan.Par[j].Type)
			{
			case 0: // time scan
				TimeArray[col][page] = value;
				break;
			case 1: // analog channel end value scan
				AnalogTable[col][row][page].fval = value;
				break;
			case 11: // analog channel timescale scan
				drow = row - NUMANALOGTIMESCALEOFFSET;// multipurposing drow as temp variable
				AnalogTable[col][drow][page].tscale = value;
				break;
			case 2: // DDS scan TBD:: check whether this works or whether ddstables need updates!
				switch (row-NUMBERANALOGCHANNELS)
				{ // ugly because of the way the ddstables are built
				case 1: ddstable[col][page].end_frequency = value; break;
				case 2: dds2table[col][page].end_frequency = value; break;
				case 3: dds3table[col][page].end_frequency = value; break;
				}
				break;
			case 22: // DDS EOR offset scan
				switch (row-NUMBERANALOGCHANNELS)
				{ // ugly because of the way the ddstables are built
				case 1: SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,value); break;
				case 2: SetCtrlVal(panelHandle,PANEL_NUM_DDS2_OFFSET,value); break;
				case 3: SetCtrlVal(panelHandle,PANEL_NUM_DDS3_OFFSET,value); break;
				}
				break;
			case 3:	// Laser scan
				LaserTable[row-NUMBERANALOGCHANNELS-NUMBERDDS-1][col][page].fval = value;
				break;
			case 4: // Anritsu scan
				AnalogTable[col][row][page].fval = value;
				break;
			case 5: // GPIB scan
				GPIBDev[row-NUMGPIBSCANOFFSET-1].value[col-1] = value;
				break;
			case 9: // Digital channel scan -- interprets positive values as high
				drow = row-NUMDIGITALSCANOFFSET;
				MultiScan.Par[j].CurrentScanValue = (double)(ToDigital(value));
				DigTableValues[col][drow][page] = ToDigital(value);
				break;
			}
		}
	}
}

void updateScannedCellsWithOriginalValues(){

	int j;
	int page, col, row, drow;

	for( j=0; j<MultiScan.NumPars; j++ )
	{
		if (MultiScan.Par[j].CellExists)
		{
			// Just for easier code reading.
			page = MultiScan.Par[j].Page;
			col = MultiScan.Par[j].Column;
			row = MultiScan.Par[j].Row;

			switch (MultiScan.Par[j].Type)
			{
			case 0: // time scan
				TimeArray[col][page] = MultiScan.Par[j].Time;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].Time;
				break;
			case 1: // analog channel end value scan
				AnalogTable[col][row][page].fval = MultiScan.Par[j].Analog.fval;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].Analog.fval;
				break;
			case 11: // analog channel timescale scan
				drow = row - NUMANALOGTIMESCALEOFFSET;// multipurposing drow as temp variable
				AnalogTable[col][drow][page].tscale = MultiScan.Par[j].Analog.tscale;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].Analog.tscale;
				break;
			case 2: // DDS scan TBD:: check whether this works or whether ddstables need updates!
				switch (row-NUMBERANALOGCHANNELS)
				{ // ugly because of the way the ddstables are built
				case 1: ddstable[col][page].end_frequency = MultiScan.Par[j].DDS.fval; break;
				case 2: dds2table[col][page].end_frequency = MultiScan.Par[j].DDS.fval; break;
				case 3: dds3table[col][page].end_frequency = MultiScan.Par[j].DDS.fval; break;
				}
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].DDS.fval;
				break;
			case 22: // DDS EOR offset scan
				switch (row-NUMBERANALOGCHANNELS)
				{ // ugly because of the way the ddstables are built
				case 1: SetCtrlVal(panelHandle,PANEL_NUM_DDS_OFFSET,MultiScan.Par[j].DDS.fval); break;
				case 2: SetCtrlVal(panelHandle,PANEL_NUM_DDS2_OFFSET,MultiScan.Par[j].DDS.fval); break;
				case 3: SetCtrlVal(panelHandle,PANEL_NUM_DDS3_OFFSET,MultiScan.Par[j].DDS.fval); break;
				}
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].DDS.fval;
				break;
			case 3:	// Laser scan
				LaserTable[row-NUMBERANALOGCHANNELS-NUMBERDDS-1][col][page].fval = MultiScan.Par[j].Laser.fval;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].Laser.fval;
				break;
			case 4: // Anritsu scan
				AnalogTable[col][row][page].fval = MultiScan.Par[j].Anritsu.fval;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].Anritsu.fval;
				break;
			case 5: // GPIB scan
				GPIBDev[row-NUMGPIBSCANOFFSET-1].value[col-1] = MultiScan.Par[j].GPIBvalue;
				MultiScan.Par[j].CurrentScanValue = MultiScan.Par[j].GPIBvalue;
				break;
			case 9: // Digital channel scan -- interprets positive values as high
				drow = row-NUMDIGITALSCANOFFSET;
				DigTableValues[col][drow][page] = MultiScan.Par[j].Digital;// Both LHS and RHS are ints so no need for ToDigital() function.
				MultiScan.Par[j].CurrentScanValue = (double) MultiScan.Par[j].Digital;
				break;
			}
		}
	}
}


//*********************************************************************
// 2016_08_04 --- Scott Smale --- Started in Version 16.1.E
// Separate out writing the info file for communication.
// This function relies on the MultiScan object being intialized
// by UpdateMultiScanValues(TRUE).
// This function must be called after UpdateMultiScanValues() to give
// an accurate value for the cycle number about to be executed.
//*********************************************************************
void writeToScanInfoFile(void){

	int i, j;
	int numRows, numCols;
	double cellValue;
	int sentinelFound, sentinelLine;
	double sentinelValue;

	int page, col, row;

	char infoFileNameWithPath[MAX_PATHNAME_LEN];
	int infoFileHandle;

	char outputBuffer[500];


	// Create the file name of the info file. (using outputBuffer temporarily)
	sprintf(outputBuffer, "info.txt");
	MakePathname(MultiScan.CommandsFilePath, outputBuffer, infoFileNameWithPath);
	printf("Info file name with path:\n");
	printf("-%s-\n", infoFileNameWithPath);

	// Open the file.
	infoFileHandle = OpenFile(infoFileNameWithPath,VAL_WRITE_ONLY,VAL_TRUNCATE,VAL_ASCII);
	if( infoFileHandle < 0){
		printf("Error opening info file:\n-%s-\n", infoFileNameWithPath);
		CloseFile(infoFileHandle);
		return;// If the file exists but we can't open it, try again next time.
	}

	// Find the sentinel (could cache the value from GetNewMultiScanCommands but that may be very stale)
	GetNumTableRows( panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numRows);
	GetNumTableColumns( panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numCols);
	for( i=1; i<=numRows; i++ ){
		for( j=1; j<=numCols; j++ ){
			GetTableCellVal( panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j,i), &cellValue);
			if( cellValue <= -999.0 ){
				break;
			}
		}
		if( cellValue <= -999.0 ){
			break;
		}
	}
	if( cellValue <= -999.0 ){
		sentinelFound = 1;
		sentinelLine = i;
		sentinelValue = cellValue;
	}
	else {
		sentinelFound = 0;
		sentinelLine = numRows;
	}

	// Info file should have the list of params (row, col, page), total number of scan table lines,
	// the shot/cycle number of the next cycle, and the scan line we are on in the scan table.

	// First write the shot number. The count here assumes that this function is called after
	// function GetNewMultiScanCommands() but before the start of the cycle.
	sprintf(outputBuffer, "%i", MultiScan.Counter);
	WriteLine(infoFileHandle, outputBuffer, -1);

	// Then the line in the scan table that is next.
	sprintf(outputBuffer, "%i", MultiScan.CurrentStep+1);
	WriteLine(infoFileHandle, outputBuffer, -1);

	// Then the total number of lines in the scan table followed by whether it has meaning or not.
	sprintf(outputBuffer, "%i %i", sentinelLine, sentinelFound);
	WriteLine(infoFileHandle, outputBuffer, -1);

	// Then iterate through all of the cells that we are scanning. (page, column, row).
	for( j=0; j<numCols; j++ ){

		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,1), &page);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,2), &col);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,3), &row);

		if( MultiScan.Par[j].CellExists ){
			sprintf(outputBuffer, "%i %i %i valid", page, col, row);
		}
		else {
			sprintf(outputBuffer, "%i %i %i invalid", page, col, row);
		}

		WriteLine(infoFileHandle, outputBuffer, -1);

	}
	CloseFile(infoFileHandle);

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
 	CheckActivePages();
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
void CheckActivePages(void)
{
  	int bool;
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
	  int ischecked=0;
	  GetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED,&ischecked);
	  SetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED,abs(ischecked-1));

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


//****************************************************************************
// 2012-10-07 Stefan Trotzky -- Enable/disable controls according to scanmode
// Comes with the multi scan option in V16.0.0
//****************************************************************************
void EnableScanControls(void)
{
	int dimmed;

  	if ( parameterscanmode == 0) // dimm out parts depending on current parameterscanmode
  	{   // MultiScan
  		dimmed = 0;
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_SCAN_TABLE, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_SCAN_TABLE_2, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_SCAN_KEEPRUNNING_CHK, ATTR_DIMMED, 0);
   		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED1, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED2, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_LBL_DIGIOFFSET, ATTR_DIMMED, 0);
  		SetCtrlAttribute (panelHandle, PANEL_LBL_GPIBOFFSET, ATTR_DIMMED, 0);

  		if (MultiScan.Active == TRUE) { dimmed = 1;}
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, ATTR_DIMMED, dimmed);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, ATTR_DIMMED, dimmed);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_DIMMED, dimmed);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NAMES_TABLE, ATTR_DIMMED, dimmed);
 		SetCtrlMenuAttribute (panelHandle, PANEL_DIGTABLE, MNU_DIGTABLE_SCANCELL, ATTR_DIMMED, dimmed);
  		SetCtrlMenuAttribute (panelHandle, PANEL_ANALOGTABLE, MNU_ANALOGTABLE_SCANCELL, ATTR_DIMMED, dimmed);
  		SetCtrlMenuAttribute (panelHandle, PANEL_ANALOGTABLE, MNU_ANALOGTABLE_SCANCELL_TIMESCALE, ATTR_DIMMED, dimmed);
  		SetCtrlMenuAttribute (panelHandle, PANEL_TIMETABLE, MNU_TIMETABLE_SCANCELL, ATTR_DIMMED, dimmed);

  	}
  	else
  	{   // One- and two-parameter scans
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NAMES_TABLE, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED1, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED2, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_SCAN_KEEPRUNNING_CHK, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_LBL_DIGIOFFSET, ATTR_DIMMED, 1);
  		SetCtrlAttribute (panelHandle, PANEL_LBL_GPIBOFFSET, ATTR_DIMMED, 1);
  		SetCtrlMenuAttribute (panelHandle, PANEL_DIGTABLE, MNU_DIGTABLE_SCANCELL, ATTR_DIMMED, 1);
  		SetCtrlMenuAttribute (panelHandle, PANEL_ANALOGTABLE, MNU_ANALOGTABLE_SCANCELL, ATTR_DIMMED, 1);
  		SetCtrlMenuAttribute (panelHandle, PANEL_ANALOGTABLE, MNU_ANALOGTABLE_SCANCELL_TIMESCALE, ATTR_DIMMED, 1);
  		SetCtrlMenuAttribute (panelHandle, PANEL_TIMETABLE, MNU_TIMETABLE_SCANCELL, ATTR_DIMMED, 1);


  		SetCtrlAttribute (panelHandle, PANEL_SCAN_TABLE, ATTR_DIMMED, 0);
  		if (parameterscanmode == 2)
  		{
  			SetCtrlAttribute (panelHandle, PANEL_SCAN_TABLE_2, ATTR_DIMMED, 0);
  		}
  	}

}


//****************************************************************************
// 2012-10-07 Stefan Trotzky -- Organize MultiScan controls in window
// Comes with the multi scan option in V16.0.0
//****************************************************************************
void ReshapeMultiScanTable( int top,int left,int height)
{
	int j,istext=0,celltype=0,width;

	// Get width of analogchannel table to match
	GetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_WIDTH, &width);

	//resize and move the multiscan table table and all it's related list boxes
	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_TOP, top);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_LEFT, left);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_POS_TABLE, ATTR_WIDTH, width);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_TOP, top+80);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_LEFT, left);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_VAL_TABLE, ATTR_WIDTH, width);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NAMES_TABLE, ATTR_TOP, top);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NAMES_TABLE, ATTR_LEFT, left-165);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, ATTR_TOP, top+100);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, ATTR_LEFT, left-135);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, ATTR_TOP, top+150);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_ITS_NUMERIC, ATTR_LEFT, left-135);

  	SetCtrlAttribute (panelHandle, PANEL_SCAN_KEEPRUNNING_CHK, ATTR_TOP, top+190);
  	SetCtrlAttribute (panelHandle, PANEL_SCAN_KEEPRUNNING_CHK, ATTR_LEFT, left-150);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED1, ATTR_TOP, top+100);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED1, ATTR_LEFT, left+width+20);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED2, ATTR_TOP, top+125);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_LED2, ATTR_LEFT, left+width+20);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_TOP, top+185);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_NUM_ROWS, ATTR_LEFT, left+width+30);

  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_TOP, top+90);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_LEFT, left+width+10);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_WIDTH, 130);
  	SetCtrlAttribute (panelHandle, PANEL_MULTISCAN_DECORATION, ATTR_HEIGHT, 63);

  	SetCtrlAttribute (panelHandle, PANEL_LBL_DIGIOFFSET, ATTR_TOP, top+10);
  	SetCtrlAttribute (panelHandle, PANEL_LBL_DIGIOFFSET, ATTR_LEFT, left+width+20);
  	SetCtrlAttribute (panelHandle, PANEL_LBL_GPIBOFFSET, ATTR_TOP, top+30);
  	SetCtrlAttribute (panelHandle, PANEL_LBL_GPIBOFFSET, ATTR_LEFT, left+width+20);

  	EnableScanControls();

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
	int ischecked=0;
	GetMenuBarAttribute (menuHandle, MENU_SETTINGS_RESETZERO, ATTR_CHECKED,&ischecked);
	SetMenuBarAttribute (menuHandle, MENU_SETTINGS_RESETZERO, ATTR_CHECKED,abs(ischecked-1));
}


//********************************************************************************************************************
void CVICALLBACK EXPORT_PANEL_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	char fexportname[200];

	FileSelectPopup ("", "*.export", "", "Export Panel?", VAL_SAVE_BUTTON, 0, 0, 1, 1,fexportname );
	ExportPanel(fexportname,strlen(fexportname));
}


//***********************************************************************************************
void CVICALLBACK CONFIG_EXPORT_CALLBACK (int menuBar, int menuItem, void *callbackData,
		int panel)
{

	FILE *fconfig;
	int i=0,j=0,k=0;
	int xval=16,yval=16,zval=10;
	char buff[500],buff2[190],fconfigname[290],buff3[31];


	FileSelectPopup ("", "*.config", "", "Save Configuration", VAL_SAVE_BUTTON, 0, 0, 1, 1,fconfigname );


	if((fconfig=fopen(fconfigname,"w"))==NULL)
	{
	//	InsertListItem(panelHandle,PANEL_DEBUG,-1,buff,1);
		MessagePopup("Save Error","Failed to save configuration file");
	}
	// write out analog channel info
	sprintf(buff,"Analog Channels\n");
	fprintf(fconfig,buff);
	sprintf(buff,"Row,Channel,Name,tbias,tfcn,MaxVolts,MinVolts,Units\n");
	fprintf(fconfig,buff);
	for(i=1;i<=NUMBERANALOGCHANNELS;i++)
	{
		strncpy(buff3,AChName[i].chname,30);
		sprintf(buff,"%d,%d,%s,%f,%f,%f,%f,%s\n",i,AChName[i].chnum,buff3,AChName[i].tbias
			,AChName[i].tfcn,AChName[i].maxvolts,AChName[i].minvolts,AChName[i].units);
		fprintf(fconfig,buff);
	}
	// Write out digital Channel info
	sprintf(buff,"Digital Channels\n");
	fprintf(fconfig,buff);
	sprintf(buff,"Row,Channel,Name\n");
	fprintf(fconfig,buff);

	for(i=1;i<=NUMBERDIGITALCHANNELS;i++)
	{
		sprintf(buff,"%d,%d,%s\n",i,DChName[i].chnum,DChName[i].chname);
		fprintf(fconfig,buff);
	}


	fclose(fconfig);
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
void ExportScanBuffer(void)
{
	int i,j,status;
	FILE *fbuffer;
	char fbuffername[250],buff[500];
    int  step,iter,mode;
    double val;
    char date[SCANBUFFER_TIMEBUFFER_LENGTH];

	status=FileSelectPopup("", "*.scan", "", "Save Scan Buffer", VAL_SAVE_BUTTON, 0, 0, 1, 1,fbuffername );
	if(status>0)
	{
		if((fbuffer=fopen(fbuffername,"w"))==NULL)
		{
			MessagePopup("Save Error","Failed to save configuration file");
		}
		switch(PScan.ScanMode)
		{
			case 0:
				sprintf(buff,"Analog Scan\nRow,%d,Column,%d,Page,%d\n",PScan.Row,PScan.Column,PScan.Page);
				break;
			case 1:
				sprintf(buff,"Time Scan\nColumn,%d,Page,%d\n",PScan.Column,PScan.Page);
				break;
			case 2:
				sprintf(buff,"DDS Scan\nColumn,%d,Page,%d\n",PScan.Column,PScan.Page);
				break;
		}
		fprintf(fbuffer,buff);
		sprintf(buff,"Cycle,Value,Step,Iteration,Time\n");
		fprintf(fbuffer,buff);
		for(i=0;i<=ScanBuffer[0].BufferSize;i++)
		{

			val=ScanBuffer[i].Value;
			step=ScanBuffer[i].Step;
			iter=ScanBuffer[i].Iteration;
			sprintf(date,ScanBuffer[i].Time);
		 	sprintf(buff,"%d,%f,%d,%d,%s\n",i,val,step,iter,date);
			fprintf(fbuffer,buff);
	 	}
	 }
	 fclose(fbuffer);
}
//**************************************************************************************************************
void ExportScan2Buffer(void)
{
	int i,j,status;
	FILE *fbuffer;
	char fbuffername[250], buff[500];
    int  step1,iter1,step2,iter2,mode;
    double val1,val2;
    char date[100];

	status=FileSelectPopup("", "*.scan2", "", "Save Scan2 Buffer", VAL_SAVE_BUTTON, 0, 0, 1, 1,fbuffername );
	if(status>0)
	{
		if((fbuffer=fopen(fbuffername,"w"))==NULL)
		{
			MessagePopup("Save Error","Failed to save configuration file");
		}

		sprintf(buff,"Cycle,Value 1, Value 2, Step 1,Iteration 1,Step 2,Iteration 2,Time\n");
		fprintf(fbuffer,buff);
		for(i=0;i<=Scan2Buffer[0].BufferSize;i++)
		{

			val1=Scan2Buffer[i].Value1;
			step1=Scan2Buffer[i].Step1;
			iter1=Scan2Buffer[i].Iteration1;
			val2=Scan2Buffer[i].Value2;
			step2=Scan2Buffer[i].Step2;
			iter2=Scan2Buffer[i].Iteration2;
			sprintf(date,Scan2Buffer[i].Time);
		 	sprintf(buff,"%d,%f,%f,%d,%d,%d,%d,%s\n",i,val1,val2,step1,iter1,step2,iter2,date);
			fprintf(fbuffer,buff);
	 	}
	 }
	 fclose(fbuffer);
}

//*****************************************************************************************
// 2017-08-04 --- Scott Smale --- Started:V16.4.0
// Writing buffer to file for multi-parameter scans. Resorting to a simple file-structure:
// A three-line character header:
// 		LINE 1: Version of the GUI and the filename of the current panel
// 		LINE 2: The type of the scan (here: Multi-parameter scan) and the number of scanned parameters
//		LINE 3: The column headers, separated by the delimiter ';'
// The following lines contain the scan data. The first column always contains the value of the
// counter. Negative values mean that additional information is given in these rows. I.e. for multi-
// parameter scans, three rows with counter = -1 preceed the scan data and contain the positions of
// the scanned parameters (page, column and row).
// Auto-writes when scan is done to the scan directory.
//*****************************************************************************************
void AutoExportMultiScanBuffer(void)
{
	int i,j,status;
	FILE *fbuffer;
	char linebuff[1023], buff[256];
	char mscanFileNameWithPath[MAX_PATHNAME_LEN];
	int manualSaveStatus;
	int fileSelectStatus;


    manualSaveStatus = ConfirmPopup ( "Scan Finished",
    						"Save .mscan file to non-standard location?");
    if( manualSaveStatus == 0 ){// User selected no
    	strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	}
	else if( manualSaveStatus == 1 ){// User selected yes
		// So ask user where they want to put the mscan file.
		fileSelectStatus = FileSelectPopup("", "*.mscan", "", "Save MultiScan Buffer",
									VAL_SAVE_BUTTON, 0, 0, 1, 1,mscanFileNameWithPath );
		// Return value of FileSelectPopup is
		// 		negative on error
		//		0	VAL_NO_FILE_SELECTED
		//		1	VAL_EXISTING_FILE_SELECTED
		//		2	VAL_NEW_FILE_SELECTED
		if( fileSelectStatus < 0 ){//Error
    		printf("Error in selecting file to save mscan file. Error code: %d\n", fileSelectStatus);
			MessagePopup ("File Error", "An error occured while selecting the filename to save the mscan file");
			// Try to save to standard location
			strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	    }
    	else if( fileSelectStatus == 0 ){
	    	// If no file selected then try to save to standard location.
	    	// User can delete file later if they wish.
    		strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	    }
    	else if( fileSelectStatus == 1 || fileSelectStatus == 2 ){
    		// Do nothing since fbuffername already contains user specified path.
	    }
    	else {
    		MessagePopup ("File Error", "Unknown status code from FileSelectPopup");
    		// Again, try to save to standard location.
    		strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	    }
	}
	else {// Error
		printf("Error in scan finished popup. Error code: %d\n", manualSaveStatus);
		MessagePopup ("Confirmation Error", "Error in scan finished popup");
		// If the code can continue, try to save to the standard location.
		strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	}

    // We should now have a valid filename.
    // Try to open file.

    printf("mscan file name with path:\n");
    printf("~%s~\n", mscanFileNameWithPath);


    if( (fbuffer=fopen(mscanFileNameWithPath,"w")) == NULL )
	{
		MessagePopup("Save Error","Failed to open mscan file for writing.");
	}

    // First line: Sequencer version and filename.
	GetPanelAttribute (panelHandle, ATTR_TITLE, &linebuff);
	strcat(linebuff, "\n");
	fprintf(fbuffer,linebuff);

    // Second line: scan type and number of scanned parameters.
	sprintf(linebuff,"Multi-parameter scan, NumPars = %d\n",MultiScan.NumPars);
	fprintf(fbuffer,linebuff);

	// Third line: Column headers.
	strcpy(linebuff,"cycle,");
	for (j=0;j<MultiScan.NumPars;j++)
	{
		sprintf(buff,"p%d,",j+1);
		strcat(linebuff,buff);
	}
	strcat(linebuff,"step,iteration,time\n");
	fprintf(fbuffer,linebuff);

	// Write parameter positions to file: Page.
	strcpy(linebuff,"-1,");
	for (j=0;j<MultiScan.NumPars;j++)
	{
		sprintf(buff,"%d,",MultiScan.Par[j].Page);
		strcat(linebuff,buff);
	}
	strcat(linebuff,"0,0,00:00:00\n");
	fprintf(fbuffer,linebuff);

	// Write parameter positions to file: Column.
	strcpy(linebuff,"-1,");
	for (j=0;j<MultiScan.NumPars;j++)
	{
		sprintf(buff,"%d,",MultiScan.Par[j].Column);
		strcat(linebuff,buff);
	}
	strcat(linebuff,"0,0,00:00:00\n");
	fprintf(fbuffer,linebuff);

	// Write parameter positions to file: Row.
	strcpy(linebuff,"-1,");
	for (j=0;j<MultiScan.NumPars;j++)
	{
		sprintf(buff,"%d,",MultiScan.Par[j].Row);
		strcat(linebuff,buff);
	}
	strcat(linebuff,"0,0,00:00:00\n");
	fprintf(fbuffer,linebuff);


	// Go through buffered values and write them to file
	for(i=0;i<=ScanBuffer[0].BufferSize;i++)
	{
		sprintf(linebuff,"%d,",i);
		for (j=0;j<MultiScan.NumPars;j++)
		{
			sprintf(buff,"%f,",ScanBuffer[i].MultiScanValue[j]);
			strcat(linebuff,buff);
		}
		sprintf(buff,"%d,%d,%s\n", ScanBuffer[i].Step, ScanBuffer[i].Iteration, ScanBuffer[i].Time);
	 	strcat(linebuff,buff);
		fprintf(fbuffer,linebuff);
 	}

	fclose(fbuffer);
}


//*****************************************************************************************
// 2012-10-06 --- Stefan Trotzky --- Started:V16.0.0
// Writing buffer to file for multi-parameter scans. Resorting to a simple file-structure:
// A three-line character header:
// 		LINE 1: Version of the GUI and the filename of the current panel
// 		LINE 2: The type of the scan (here: Multi-parameter scan) and the number of scanned parameters
//		LINE 3: The column headers, separated by the delimiter ';'
// The following lines contain the scan data. The first column always contains the value of the
// counter. Negative values mean that additional information is given in these rows. I.e. for multi-
// parameter scans, three rows with counter = -1 preceed the scan data and contain the positions of
// the scanned parameters (page, column and row).
//*****************************************************************************************
void ExportMultiScanBuffer(void)
{
	int i,j,status;
	FILE *fbuffer;
	char fbuffername[255], linebuff[1023], buff[256];
    char date[100];

	status=FileSelectPopup("", "*.mscan", "", "Save MultiScan Buffer", VAL_SAVE_BUTTON, 0, 0, 1, 1,fbuffername );
	if(status>0)
	{
		if((fbuffer=fopen(fbuffername,"w"))==NULL)
		{
			MessagePopup("Save Error","Failed to open file for writing.");
		}

		// First line: Sequencer version and filename.
		GetPanelAttribute (panelHandle, ATTR_TITLE, &linebuff);
		strcat(linebuff, "\n");
		fprintf(fbuffer,linebuff);

		// Second line: scan type and number of scanned parameters.
		sprintf(linebuff,"Multi-parameter scan, NumPars = %d\n",MultiScan.NumPars);
		fprintf(fbuffer,linebuff);

		// Third line: Column headers.
		strcpy(linebuff,"cycle,");
		for (j=0;j<MultiScan.NumPars;j++)
		{
			sprintf(buff,"p%d,",j+1);
			strcat(linebuff,buff);
		}
		strcat(linebuff,"step,iteration,time\n");
		fprintf(fbuffer,linebuff);

		// Write parameter positions to file: Page.
		strcpy(linebuff,"-1,");
		for (j=0;j<MultiScan.NumPars;j++)
		{
			sprintf(buff,"%d,",MultiScan.Par[j].Page);
			strcat(linebuff,buff);
		}
		strcat(linebuff,"0,0,00:00:00\n");
		fprintf(fbuffer,linebuff);

		// Write parameter positions to file: Column.
		strcpy(linebuff,"-1,");
		for (j=0;j<MultiScan.NumPars;j++)
		{
			sprintf(buff,"%d,",MultiScan.Par[j].Column);
			strcat(linebuff,buff);
		}
		strcat(linebuff,"0,0,00:00:00\n");
		fprintf(fbuffer,linebuff);

		// Write parameter positions to file: Row.
		strcpy(linebuff,"-1,");
		for (j=0;j<MultiScan.NumPars;j++)
		{
			sprintf(buff,"%d,",MultiScan.Par[j].Row);
			strcat(linebuff,buff);
		}
		strcat(linebuff,"0,0,00:00:00\n");
		fprintf(fbuffer,linebuff);


		// Go through buffered values and write them to file
		for(i=0;i<=ScanBuffer[0].BufferSize;i++)
		{
			sprintf(linebuff,"%d,",i);
			for (j=0;j<MultiScan.NumPars;j++)
			{
				sprintf(buff,"%f,",ScanBuffer[i].MultiScanValue[j]);
				strcat(linebuff,buff);
			}
			sprintf(buff,"%d,%d,%s\n", ScanBuffer[i].Step, ScanBuffer[i].Iteration, ScanBuffer[i].Time);
		 	strcat(linebuff,buff);
			fprintf(fbuffer,linebuff);
	 	}
	 }
	 fclose(fbuffer);
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
//**************************************************************************************************************
void CVICALLBACK MultiScan_AddCellToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{   // Adds a column to the multiscan list to scan the current cell's value

	int numCols, ccol, crow, cpage;
	Point currentCell = {0,0};

	if (parameterscanmode == 0)
	{
		GetNumTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numCols);
		if (numCols < NUMMAXSCANPARAMETERS)
		{
			// Add a column to the right, step up number in numeric control
			InsertTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, -1, 1, 0);
			InsertTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, -1, 1, 0);
			SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, numCols+1);


			// Get coordinates of active cell
			GetActiveTableCell (panelHandle, controlID, &currentCell);
			ccol = currentCell.x;
			cpage = currentpage;

			// Set effective row number according to generating control
			switch (controlID)
			{
			case PANEL_TIMETABLE: crow = 0; break;
			case PANEL_ANALOGTABLE: crow = currentCell.y; break;
			case PANEL_DIGTABLE : crow = currentCell.y + NUMDIGITALSCANOFFSET; break;
			}

			// Write cell position to scan table
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,1), cpage);
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,2), ccol);
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,3), crow);
		}
		else
		{
			MessagePopup("Multi scan error","Cannot add parameter to scanlist. Maximum number\nof parameters reached. Need to increase NUMMAXSCANPARAMETERS.");
		}
	}
}

void CVICALLBACK MultiScan_AddAnalogCellTimeScaleToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{   // Adds a column to the multiscan list to scan the current cell's timescale

	int numCols, ccol, crow, cpage;
	Point currentCell = {0,0};

	if (parameterscanmode == 0)
	{
		GetNumTableColumns(panelHandle0, PANEL_MULTISCAN_VAL_TABLE, &numCols);
		if (numCols < NUMMAXSCANPARAMETERS)
		{
			// Add a column to the right, step up number in numeric control
			InsertTableColumns(panelHandle0, PANEL_MULTISCAN_POS_TABLE, -1, 1, 0);
			InsertTableColumns(panelHandle0, PANEL_MULTISCAN_VAL_TABLE, -1, 1, 0);
			SetCtrlVal(panelHandle0, PANEL_MULTISCAN_NUM_NUMERIC, numCols+1);

			// Get coordinates of active cell (pseudorow since timescale is not actually a row)
			GetActiveTableCell(panelHandle0, controlID, &currentCell);
			ccol = currentCell.x;
			crow = currentCell.y + NUMANALOGTIMESCALEOFFSET;
			cpage = currentpage;

			// Write cell position to scan table
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,1), cpage);
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,2), ccol);
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,3), crow);
		}
		else
		{
			MessagePopup("Multi scan error","Cannot add parameter to scanlist. Maximum number\nof parameters reached. Need to increase NUMMAXSCANPARAMETERS.");
		}
	}
}

// Callback so that right clicking on an EOR numeric entry element will add that EOR to the scan table.
int CVICALLBACK DDS_OFFSET_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int numCols, ccol, crow, cpage;

	switch (event)
	{
	case EVENT_RIGHT_CLICK:
		if( parameterscanmode == 0 ){
			GetNumTableColumns(panelHandle0, PANEL_MULTISCAN_VAL_TABLE, &numCols);
			if( numCols < NUMMAXSCANPARAMETERS )
			{
				// Add a column to the right, step up number in numeric control
				InsertTableColumns(panelHandle0, PANEL_MULTISCAN_POS_TABLE, -1, 1, 0);
				InsertTableColumns(panelHandle0, PANEL_MULTISCAN_VAL_TABLE, -1, 1, 0);
				SetCtrlVal(panelHandle0, PANEL_MULTISCAN_NUM_NUMERIC, numCols+1);

				// Get pseudo page,col,row for the DDS
				// Same row as DDS in analog channel table
				// Page doesn't make sense so set to 1 arbitrarily.
				// Column doesn't make sense so set to a large value which will be the "EOR" column from now on
				cpage = 1;
				ccol = DDS_EOR_SCAN_COLUMN_VALUE;
				switch( control )
				{
				case PANEL_NUM_DDS_OFFSET: crow = NUMBERANALOGCHANNELS+1; break;
				case PANEL_NUM_DDS2_OFFSET: crow = NUMBERANALOGCHANNELS+2; break;
				case PANEL_NUM_DDS3_OFFSET : crow = NUMBERANALOGCHANNELS+3; break;
				}

				// Write page, col, row to scan table
				SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,1), cpage);
				SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,2), ccol);
				SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,3), crow);
			}
			else
			{
				MessagePopup("Multi scan error","Cannot add parameter to scanlist. Maximum number\nof parameters reached. Need to increase NUMMAXSCANPARAMETERS.");
			}
		}
		break;
	}
	return 0;
}



//**************************************************************************************************************
void CVICALLBACK MultiScan_Table_SetVals(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
		Point cellPoint;

		GetActiveTableCell (panelHandle, PANEL_MULTISCAN_VAL_TABLE, &cellPoint);
		DisplayPanel (panelHandle8);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_TABLE, PANEL_MULTISCAN_VAL_TABLE);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_COLUMN, cellPoint.x);

}

//**************************************************************************************************************
void CVICALLBACK MultiScan_Table_DeleteColumn(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
		Point cellPoint = {0,0};
		int ppage, pcol, prow, numCols;
		int status;
		char message[500];

		// Any columns left?
		GetNumTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numCols);
		if (numCols > 1)
		{
			// Get current column
			GetActiveTableCell (panelHandle, controlID, &cellPoint);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(cellPoint.x,1), &ppage);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(cellPoint.x,2), &pcol);
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(cellPoint.x,3), &prow);

			// Ask for confirmation
			sprintf(message, "Remove parameter from scan (pg: %d, col: %d, row: %d)?", ppage, pcol, prow);
			status = ConfirmPopup("Remove parameter from scan",message);

			// Remove column
			if (status == 1)
			{
				DeleteTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, cellPoint.x, 1);
				DeleteTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, cellPoint.x, 1);
				SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, numCols-1);

			}
		}
		else
		{
			MessagePopup("Multi scan error","Cannot delete last column!");
		}
}

//**************************************************************************************************************
void CVICALLBACK MultiScan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	int numRows, numCols,numEntries,i,j,maxInd;
	int found;
	double *order,*tempSL;
	int *idx;
	double maxVal, value;

	found = 0; i = 0;
	GetNumTableRows (panelHandle,PANEL_MULTISCAN_VAL_TABLE,&numRows);
	GetNumTableColumns (panelHandle,PANEL_MULTISCAN_VAL_TABLE,&numCols);

	// search for first sentinel in scanlist
	while( (!found) && (i<=numRows) )
	{
		i++;
		for (j=1;j<=numCols;j++)
		{
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j, i), &value);
			if (value < -999.0)
			{
				found = 1;
				break;
			}
		}
	}
	numEntries = i;  // use 1 - numEntries-1

	// allocate memory for sorting arrays
	order = (double *)malloc(sizeof(double)*numEntries);
	idx = (int *)malloc(sizeof(int)*numEntries);
	tempSL = (double *)malloc(sizeof(double)*numRows);

	//randomize order values
	for(i=1;i<numEntries;i++)
	{
		order[i] = Random (0, 100);
	}

	//sort using order vals (a sucky selection sort)
	for(i=1;i<numEntries;i++)
	{
		maxVal=-1.0;
		for(j=1;j<numEntries;j++)
		{
			if(order[j]>maxVal)
			{
				maxVal=order[j];
				maxInd=j;
			}
		}
		idx[i] = maxInd;
		order[maxInd]=-1;
	}

	for (j=1;j<=numCols;j++)
	{
		for(i=1;i<numEntries;i++) // get this column
		{
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j, i), &tempSL[i]);
		}
		for(i=1;i<numEntries;i++) // scramble this column
		{
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE, MakePoint(j, i), tempSL[idx[i]]);
		}
	}

	free(order);
	free(tempSL);
	free(idx);
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
//Open Scan Table Loader Panel
void CVICALLBACK Scan_Table_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
		DisplayPanel (panelHandle8);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_TABLE, controlID);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_COLUMN, 1);
}

//**************************************************************************************************************
void CVICALLBACK Scan_Table_NumSet_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
		DisplayPanel (panelHandle9);
}

//**************************************************************************************************************
void CVICALLBACK Scan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
	int numRows,numEntries,i,j,maxInd;
	double *order,*tempSL;
	double maxVal;

	i=1;
	GetNumTableRows (panelHandle,controlID,&numRows);
	tempSL=(double *)malloc(sizeof(double)*numRows);

	GetTableCellVal(panelHandle, controlID, MakePoint(1, i),&tempSL[i]);
	while(tempSL[i]>-999.0&&i<=numRows)
	{
		i++;
		GetTableCellVal(panelHandle, controlID, MakePoint(1, i),&tempSL[i]);
	}

	numEntries=i;  // use 1 - numEntries-1
	order=(double *)malloc(sizeof(double)*numEntries);


	//randomize order values
	for(i=1;i<numEntries;i++)
		order[i]=Random (0, 100);

	//sort using order vals (a sucky selection sort)
	for(i=1;i<numEntries;i++)
	{
		maxVal=-1.0;
		for(j=1;j<numEntries;j++)
		{
			if(order[j]>maxVal)
			{
				maxVal=order[j];
				maxInd=j;
			}
		}
		SetTableCellVal(panelHandle, controlID, MakePoint(1, i),tempSL[maxInd]);
		order[maxInd]=-1;
	}

	free(order);
	free(tempSL);
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
		DisplayPanel (panelHandle13);
}
/**************************************************************************************/
int ToDigital(double analog)
{
	if (analog > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/**************************************************************************************/


