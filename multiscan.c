// Functions for MultiScan go here.
// Also put the Scan and Scan2 functions here. They will be removed eventually.


// TODO:
// DDS_OFFSET_CALLBACK has multiscan code in it but it is a callback in the GUI
// Helper function ToDigital should go in a separate file eventually.

#include "multiscan.h"

#include <ansi_c.h>
#include <utility.h>
#include <formatio.h>
#include "toolbox.h"

#include "vars.h"
#include "GUIDesign.h"// For panel handles
#include "GUIDesign2.h"// For DrawNewTable fn
#include "saveload.h"// For saving function
#include "GPIB_SRS_SETUP.h"// For SETUP_GPIB_DEVICENO
#include "ScanTableLoader.h"// For SL_PANEL_NUM_* handles
#include "Scan.h"// For SCANPANEL_CHECK_USE_LIST handle



/************************************************************************
MultiScan functions
*************************************************************************/


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
    						"Save .mscan file to standard location?");
    if( manualSaveStatus == 1 ){// User selected yes
    	strcpy(mscanFileNameWithPath,MultiScan.ScanDirPath);
	}
	else if( manualSaveStatus == 0 ){// User selected no
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

// Put the values that are in MultiScan.Par[j].{Page,Column,Row} back into
//  the table PANEL_MULTISCAN_POS_TABLE.
// Written initially for use by saveload to put the loaded MultiScan into the sequencer
//  since 90% of the code uses the values in PANEL_MULTISCAN_POS_TABLE as opposed to the
//  MultiScan.Par[j]. (and MultiScan.Par[j] is set each time from PANEL_MULTISCAN_POS_TABLE)
void putMultiScanPosTable(void)
{
	int numCols, numColsWant;
	int i;

	// Get the current number of colunns
	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &numCols);

	// Get the number of columns we want
	numColsWant = MultiScan.NumPars;
	printf("numColsWAnt = %d\n",numColsWant);

	// Error checking: There is something wrong since can't have less than 1 col or more than the max
	if( numColsWant < 1 || numColsWant > NUMMAXSCANPARAMETERS){
		printf("Error, did not load multiscan cell positions due to incorrect number of columns\n");
		return;
	}

	// Resize the tables
	if( numCols > numColsWant ){// delete some cols
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, numColsWant+1, -1);
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, numColsWant+1, -1);
		SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, numColsWant);
	}
	else if( numCols < numColsWant ){// add some cols
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, -1, numColsWant-numCols, 0);
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, -1, numColsWant-numCols, 0);
		SetCtrlVal(panelHandle, PANEL_MULTISCAN_NUM_NUMERIC, numColsWant);
	}
	// else do nothing since we have the correct number of columns

	// Now just set the pages, rows, cols
	for( i = 0; i < numColsWant; i++ ){
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(i+1,1), MultiScan.Par[i].Page);
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(i+1,2), MultiScan.Par[i].Column);
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(i+1,3), MultiScan.Par[i].Row);
	}
}

// Print MultiScan for debugging
void printMultiScan(void)
{
	printf("-------\nMultiScan:\n");
	printf("CurrentStep: %d\n", MultiScan.CurrentStep);
	printf("CurrentIteration: %d\n", MultiScan.CurrentIteration);
	printf("Iterations: %d\n", MultiScan.Iterations);
	printf("NumPars: %d\n", MultiScan.NumPars);
	printf("Counter: %d\n", MultiScan.Counter);
	printf("NextCommandsFileNumber: %d\n", MultiScan.NextCommandsFileNumber);
	printf("CommandsFilePath: |%s|\n", MultiScan.CommandsFilePath);
	printf("ScanDirPath: |%s|\n", MultiScan.ScanDirPath);
	printf("HoldingPattern: %d\n", MultiScan.HoldingPattern);
	printf("SentinelFoundValue: %f\n", MultiScan.SentinelFoundValue);
	printf("Done: %d\n", MultiScan.Done);
	printf("Active: %d\n", MultiScan.Active);
	printf("Initialized: %d\n", MultiScan.Initialized);

	printf("Pars: <page> <col> <row>\n");
	for( int i=0; i < MultiScan.NumPars; ++i ){
		printf("%d:\t%d\t%d\t%d\n",i,MultiScan.Par[i].Page,MultiScan.Par[i].Column,MultiScan.Par[i].Row);
	}
}


/************************************************************************
MultiScan callbacks
*************************************************************************/


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

// Add a GPIB cell to the multi scan list
void CVICALLBACK MultiScan_AddGPIBCellToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{   // Adds a column to the multiscan list to scan the current cell's value

	int numCols, ccol, crow, cpage, devnum;
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

			// Get coordinates of active cell & device number
			GetActiveTableCell (panelHandle, controlID, &currentCell);
			GetCtrlVal (panelHandle13,SETUP_GPIB_DEVICENO, &devnum);
			ccol = currentCell.x;
			crow = devnum + NUMGPIBSCANOFFSET;
			cpage = 1; // no page needed for GPIB programming

			// Write cell position to scan table
			SetTableCellVal(panelHandle0, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,1), cpage);
			SetTableCellVal(panelHandle0, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,2), ccol);
			SetTableCellVal(panelHandle0, PANEL_MULTISCAN_POS_TABLE, MakePoint(numCols+1,3), crow);
		}
		else
		{
			MessagePopup("Multi scan error","Cannot add parameter to scanlist. Maximum number\nof parameters reached. Need to increase NUMMAXSCANPARAMETERS.");
		}
	}
}

void CVICALLBACK MultiScan_Table_SetVals(int panelHandle, int controlID, int MenuItemID, void *callbackData)
{
		Point cellPoint;

		GetActiveTableCell (panelHandle, PANEL_MULTISCAN_VAL_TABLE, &cellPoint);
		DisplayPanel (panelHandle8);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_TABLE, PANEL_MULTISCAN_VAL_TABLE);
		SetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_COLUMN, cellPoint.x);

}

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

// Callback so that right clicking on an EOR numeric entry element will add that EOR to the scan table.
void MultiScan_AddDdsEorToScan(int control)
{
	int numCols, ccol, crow, cpage;

	if( parameterscanmode == 0 ) // check global var for kind of scan
	{
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
				case PANEL_NUM_DDS_OFFSET:
					crow = NUMBERANALOGCHANNELS+1;
					break;
				case PANEL_NUM_DDS2_OFFSET:
					crow = NUMBERANALOGCHANNELS+2;
					break;
				case PANEL_NUM_DDS3_OFFSET :
					crow = NUMBERANALOGCHANNELS+3;
					break;
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

}







/************************************************************************
Helper functions
*************************************************************************/
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













/************************************************************************
Scan and Scan2 functions with callbacks at the end
*************************************************************************/


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





