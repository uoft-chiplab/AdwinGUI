// Saving and Loading functions for panels go here.
// Any binary dumps for debugging also go here (arrays sent to the adwin for example)

// TODO:
// Write SaveArraysV17 where V17 save file
//		a) saves the existing variables that are saved to the .arr file with better metalanguage
//		b) saves the gpib settings in the same file
//		c) saves everything that is required to not need the .pan file (specifics of this tbd)
// Note that this code is still technically V16 until this work on saving is completed.
// The idea being at the moment that to load an old panel, we would start this program (say V16.9.9 or
// something) and then save the panel as V17 savefile version. After we get that working then we can
// write LoadV16intoV17 function if we want to.
// Update default directory to look for save files

// Generic format for saving one "thing" is:
// <tag_name>~elem_size~num_dim~dim1~dim2~...~BINARY_DATA</tag_name>

// possibly change to make temp variable pointer and pass it in then free memory allocated in these functions


#include "saveload.h"

#include <ansi_c.h>
#include <userint.h>
#include <utility.h>

#include "vars.h"
#include "GUIDesign.h"// For menubar definitions
#include "GUIDesign2.h"// For DrawNewTable fn
#include "LaserSettings2.h"// For LaserSettingsInit fn
#include "AnritsuSettings2.h"//For LoadAnritsuSettings fn
#include "AnalogSettings2.h"//For SetAnalogChannels fn
#include "DigitalSettings2.h"//For SetDigitalChannels fn
#include "main.h"//For reinitializing after failed load
#include "multiscan.h"// For putMultiScanPosTable fn

/************************************************************************
Overall Save and Load that chooses correct version
*************************************************************************/


void SaveSettings(int version)
{
	char buff[200]="";

	char imgsDirPath[MAX_PATHNAME_LEN];
	char runDirPath[MAX_PATHNAME_LEN];
	char panFilePath[MAX_PATHNAME_LEN];
	char panFileDir[MAX_PATHNAME_LEN];
	char panFileName[MAX_PATHNAME_LEN];
	int panStatus;
	int runDirStatus;
	int imgsDirStatus;
	int confirmStatus;

	// Get the filename
	switch( version ){
		case 15:
		case 16:
		default:
			panStatus = FileSelectPopup("", "*.pan", "", "Save Settings", VAL_SAVE_BUTTON, 0, 0, 1, 1, panFilePath);
			break;
		case 17:
			panStatus = FileSelectPopup("", "*.seq", "", "Save Settings", VAL_SAVE_BUTTON, 0, 0, 1, 1, panFilePath);
			break;
	}

	// Check for errors
	if( panStatus < 0 ){// an error
		printf("Error while choosing save file name. Error code: %d\n", panStatus);
		MessagePopup("Save filename Error", "An error occured while choosing the save filename.");
		return;
	}
	else if( panStatus == VAL_NO_FILE_SELECTED ){
		MessagePopup ("File Error", "No file was selected");
		return;
	}
	else if( panStatus != VAL_EXISTING_FILE_SELECTED && panStatus != VAL_NEW_FILE_SELECTED ){// something strange happened
		printf("Error while choosing save file name. Error code: %d\n", panStatus);
		MessagePopup("Save filename Error", "An error occured while choosing the save filename.");
		return;
	}
	// Now we have a valid filename (incl dir) in panFilePath

	// Save according to file version requested
	switch (version)
	{
		case 15:
			SavePanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			SaveArraysV15(panFilePath, strlen(panFilePath));
			SaveLaserData(panFilePath,strlen(panFilePath));
			break;
		case 16: // V16 saves laser data together with arrays
			SavePanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			SaveArraysV16(panFilePath, strlen(panFilePath));
			break;
		case 17:
			if( SaveSequenceV17(panFilePath, strlen(panFilePath)) ){// if there was an error
				MessagePopup("Saving Error","Error occured while saving. File not successfully saved.");
			}
			break;
		default:
			SavePanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			SaveArraysV15(panFilePath, strlen(panFilePath));
			SaveLaserData(panFilePath,strlen(panFilePath));
			break;
	}

	// Update the title of the sequencer
	SplitPath(panFilePath, NULL, panFileDir, panFileName);
	strcpy(buff, SEQUENCER_VERSION);
	strcat(buff, panFileName);
	SetPanelAttribute(panelHandle, ATTR_TITLE, buff);

	// Create the run folder
	strcpy(buff, panFileName);
	buff[strlen(buff)-4] = '\0';// Assume the last 4 characters of panFileName are the ".pan"
	MakePathname(panFileDir, buff, runDirPath);

	printf("runDirPath:\n>%s<\n", runDirPath);

	runDirStatus = MakeDir(runDirPath);

	if( runDirStatus == 0 ) // Successfully created directory
	{
		// Form the imgs subdirectory path and print it.
		strcpy(buff, "imgs");
		MakePathname(runDirPath, buff, imgsDirPath);
		printf("imgsDirPath:\n>%s<\n", imgsDirPath);

		// Attempt to create the subdirectories
		imgsDirStatus = MakeDir(imgsDirPath);

		// Check status of subdirectories

		if( imgsDirStatus < 0 ){ //Error
			printf("Error when creating images directory. Error code: %d\n", imgsDirStatus);
			MessagePopup ("Directory Error", "An error occured while creating the images directory.");
		}
	}
	else if( runDirStatus == -9 ){// The directory (or file) already exists

		// Ask for confimation to use existing directory.
		confirmStatus = ConfirmPopup ("Confirm use of existing directory",
						"Warning! Use existing scan directory?");
		if( confirmStatus == 0 ){// User selected no
		}
		else if( confirmStatus == 1 ){// User selected yes
			// Since the scan directory already exists the subdirectories probably also
			// already exist so we should accept those that exist as is.

			// Form the imgs subdirectory path and print it.
			strcpy(buff, "imgs");
			MakePathname(runDirPath, buff, imgsDirPath);
			printf("imgsDirPath:\n>%s<\n", imgsDirPath);

			// Attempt to create the subdirectories
			imgsDirStatus = MakeDir(imgsDirPath);

			// Check status of subdirectories

			if( imgsDirStatus < 0  && imgsDirStatus != -9){//Error but not error that imgs already exists
				printf("Error when creating images directory. Error code: %d\n", imgsDirStatus);
				MessagePopup ("Directory Error", "An error occured while creating the images directory.");
			}
		}
		else {
			MessagePopup ("Directory Error", "Unknown status code from MakeDir");
		}
	}
	else if( runDirStatus < 0 ){// Error
		printf("Error when creating run directory. Error code: %d\n", runDirStatus);
		MessagePopup ("Directory Error", "An error occured while creating the run directory.");
	}

	else {
			MessagePopup ("Directory Error", "Unknown status code from MakeDir");
	}

}


void LoadSettings(int version)
{
	char buff[600];
	int success=0;

	char panFilePath[MAX_PATHNAME_LEN];
	char panFileDir[MAX_PATHNAME_LEN];
	char panFileName[MAX_PATHNAME_LEN];
	int panStatus;

	// Get the filename
	switch( version ){
		case 13:
		case 15:
		case 16:
		default:
			panStatus = FileSelectPopup("", "*.pan", "", "Load Settings", VAL_LOAD_BUTTON, 0, 0, 1, 1, panFilePath);
			break;
		case 17:
			panStatus = FileSelectPopup("", "*.seq", "", "Load Settings", VAL_LOAD_BUTTON, 0, 0, 1, 1, panFilePath);
			break;
	}

	// Check for errors
	if( panStatus < 0 ){// an error
		printf("Error while choosing save file name. Error code: %d\n", panStatus);
		MessagePopup("Save filename Error", "An error occured while choosing the save filename.");
		return;
	}
	else if( panStatus == VAL_NO_FILE_SELECTED ){
		MessagePopup ("File Error", "No file was selected");
		return;
	}
	else if( panStatus != VAL_EXISTING_FILE_SELECTED && panStatus != VAL_NEW_FILE_SELECTED ){// something strange happened
		printf("Error while choosing save file name. Error code: %d\n", panStatus);
		MessagePopup("Save filename Error", "An error occured while choosing the save filename.");
		return;
	}
	// Now we have a valid filename (incl dir) in panFilePath

	// Load according to the file version
	switch (version)
	{
		case 13:
			RecallPanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			LoadArraysV13(panFilePath,strlen(panFilePath));
			LoadLaserData(panFilePath,strlen(panFilePath));
			success = 1; // no success evaluation before V16
			LaserSettingsInit();
			break;
		case 15:
			RecallPanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			LoadArraysV15(panFilePath,strlen(panFilePath));
			LoadLaserData(panFilePath,strlen(panFilePath));
			success = 1; // no success evaluation before V16
			LaserSettingsInit();
			break;
		case 16: // V16 saves laser data together with arrays
			RecallPanelState(PANEL, panFilePath, 1);// This one can be problematic when elements have been removed from the GUI!!!
			success = LoadArraysV16(panFilePath,strlen(panFilePath));
			if( success ){
				LaserSettingsInit();
			}
			break;
		case 17:
			if( LoadSequenceV17(panFilePath,strlen(panFilePath)) ){// negative on error, 0 on success
				// If we have failed to load properly, the initialize everything to the initial state as in main.c
				// NOTE: this does not reset everything (such as menu options) so we still warn the user to restart the sequencer
				initializeAnalogChProps();
				initializeAnalogTable();
				initializeDigArray();
				initializeInfoArray();
				initializeLaserArrays();
				initializeGpibDevArray();
				initializeDdsTables();
				initializeMultiScan();

				LaserSettingsInit();

				MessagePopup("Error loading sequence!",
							 "There was an error while reading from the file and setting global variables.\n!You are strongly advised to restart the sequencer!");

				success = 0;
			}
			else {
				success = 1;
				// Note that LaserSettingsInit() is called from the appropriate fn in LoadSequenceV17.
				//  And is one of the reasons why we must reinitialize things if the load fails.
			}
			break;
	}

	if (success)
	{
		// Update the title of the sequencer
		SplitPath(panFilePath, NULL, panFileDir, panFileName);
		strcpy(buff, SEQUENCER_VERSION);
		strcat(buff, panFileName);
		SetPanelAttribute(panelHandle, ATTR_TITLE, buff);
	}

	DrawNewTable(0);
}

/************************************************************************
Last used GUI Save and Load
*************************************************************************/

// From GUIDesign.c
//
void SaveLastGuiSettings(void)
{
	// Save settings:  First look for file .ini  This will be a simple 1 line file stating the name of the last file
	//saved.  Load this up and use as the starting name in the file dialog.
	char fname[100] = LASTGUISAVELOCATION;
	char buff[600];
	int i;

	SavePanelState(PANEL, fname, 1);
	SaveSequenceV17(fname, strlen(fname)); //I'm here hellooo
}


/************************************************************************
Version 17 Save and Load
*************************************************************************/

// The calling code of SaveV17 and LoadV17 should pop up a window saying that there was an error saving/loading.
// In the case of loading it should ideally run the initialization again to zero everything.

int SaveSequenceV17(char* save_name, int sn_length)
{
	// List all things that are saved:
	// Sequencer version (as vars.h string)
	// Savefile version
	// For EACH array, explicitly state the element size, dimension, size_dim1, size_dim2, et cetera
	// For EACH array, put ascii name before it as a demarkation and an ending marker after the array
	// For arrays of structs, consider not saving the binary data but saving the elements themselves with
	//	specific writing (and reading) functions.
	//
	// (Global) Arrays that are already saved in .arr file: -- Done
	// TimeArray
	// AnalogTable
	// DigTableValues
	// AChName
	// DChName
	// ddstable
	// dds2table
	// dds3table
	// LaserProperties
	// LaserTable
	// AnritsuSettingsValues
	// InfoArray (column labels)
	// Page enabled checkboxes statuses (ie. PANEL_TB_SHOWPHASE[i])
	// The update period of the ADwin (from the menu setting)
	// A few global DDS settings
	// (obsolete) srs specific gpib settings
	//
	// Other things that need to be saved in the new file format:
	// Titles of each page (button text) -- Done
	// The rest of the (global) DDS settings (ie. DDSFreq) -- Done
	// The gpib settings (merge .gpib and .arr files) -- Done
	// End of ramps for DDS -- Done
	//		Put and get the EORs separately from the other DDS things
	// End of ramps for Anritsu -- Done
	//		Explicitly save to AnritsuSettingValues[0].offset during the put fn
	//		And load to the PANEL_ANRITSU_OFFSET during the get fn
	// Build every time check mark -- Done
	// The Preferences menu, menu options -- Done
	//		Use compression -- Done
	//		Use simple timing -- Done
	//		The rest are either not a preference or implemented or meaningful to save
	// Reset to zero menu option? Does it do anything? -- No
	//		The per channel resets are stored in the respective channel properties
	//		So for now don't try and save the menu option
	// MultiScan -- Done
	//		It would be nice to always load the scan that was present when the sequence was saved
	// ... I think that's it.

	FILE *fbuffer;
	char file_buff[MAX_PATHNAME_LEN] = "";

	char cbuff[512] = "";

	int status;


	// Copy characters to buffer to prevent overflow of filename
	strncpy(file_buff, save_name, sn_length);
	if( (fbuffer = fopen(file_buff,"w")) == NULL ){
		MessagePopup("Save Error", "Failed to open file for saving");
		return -1;
	}

	// Write sequencer version
	sprintf(cbuff, "<SeqVer>~%d~%d~%d~", sizeof(char), 1, strlen(SEQUENCER_VERSION));
	strncat(cbuff, SEQUENCER_VERSION, strlen(SEQUENCER_VERSION));
	strcat(cbuff, "</SeqVer>");
	fwrite(cbuff, sizeof(char), strlen(cbuff), fbuffer);

	// Write save file version
	sprintf(cbuff, "<SaveVersion>~%d~%d~%d~", sizeof(char), 1, strlen("17.0"));
	strncat(cbuff, "17.0", strlen("17.0"));
	strcat(cbuff, "</SaveVersion>");
	fwrite(cbuff, sizeof(char), strlen(cbuff), fbuffer);

	// Write TimeArray
	status = putTimeArrayToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write AnalogTable
	status = putAnalogTableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DigitalTable
	status = putDigitalTableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Analog channel properties
	status = putAnalogChPropsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Digital channel properties
	status = putDigitalChPropsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DDS1 Table
	status = putDds1TableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DDS2 Table
	status = putDds2TableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DDS3 Table
	status = putDds3TableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DDS Globals
	status = putDdsGlobalsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write DDS EOR cells
	status = putDdsEorsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write LaserTable
	status = putLaserTableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write LaserProps
	status = putLaserPropsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Anritsu Table
	status = putAnritsuTableToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Anritsu Props
	status = putAnritsuPropsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write InfoArray (column labels)
	status = putInfoArrayToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Page Names (text on buttons)
	status = putPageNamesToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Page Checkboxes
	status = putPageCheckboxesToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Update Period
	status = putUpdatePeriodToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write GPIB Devices Settings
	status = putGpibDevsToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Force Build Checkbox
	status = putForceBuildChkToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Use Compression Preferences Menu option
	status = putUseCompressionToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write Use Simple Timing Preferences Menu option
	status = putUseSimpleTimingToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }

	// Write MultiScan object
	status = putMultiScanToFile(fbuffer);
	if( status < 0 ){ fclose(fbuffer); return status; }


	fflush(fbuffer);// make sure the file is completely written by the time we leave this function
	printf("Saved file successfully\n");
	return 0;// return success
}

int LoadSequenceV17(char* load_name, int ln_length)
{
	FILE *fbuffer;
	char file_buff[MAX_PATHNAME_LEN] = "";
	long fpos, fpos_file_end;

	int majorVer, minorVer;

	// Copy characters to buffer to prevent overflow of filename
	strncpy(file_buff, load_name, ln_length);
	if( (fbuffer = fopen(file_buff,"r")) == NULL ){
		MessagePopup("Load Error", "Failed to open file for loading");
		return -1;// Don't try to load anything from a NULL file pointer
	}
	// File is ready for reading

	// Get the fpos at the end of the file to make sure we don't read too much
	fseek(fbuffer, 0, SEEK_END);
	fpos_file_end = ftell(fbuffer);
	fseek(fbuffer, 0, SEEK_SET);

	// Read the first part of the file to print seq version info
	fpos = checkVersionFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the version of the save file
	fpos = getSaveVersionFromFile(fbuffer, fpos_file_end, &majorVer, &minorVer);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the TimeArray
	fpos = getTimeArrayFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the AnalogTable
	fpos = getAnalogTableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DigitalTable
	fpos = getDigitalTableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Analog channel properties
	fpos = getAnalogChPropsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Digital channel properties
	fpos = getDigitalChPropsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DDS1 Table
	fpos = getDds1TableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DDS2 Table
	fpos = getDds2TableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DDS3 Table
	fpos = getDds3TableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DDS Globals
	fpos = getDdsGlobalsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the DDS EOR cells
	fpos = getDdsEorsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the LaserTable
	fpos = getLaserTableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Laser properties
	fpos = getLaserPropsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Anritsu Table
	fpos = getAnritsuTableFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Anritsu Props
	fpos = getAnritsuPropsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the InfoArray (column labels)
	fpos = getInfoArrayFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Page Names (text on buttons)
	fpos = getPageNamesFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Page Checkboxes
	fpos = getPageCheckboxesFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Update Period
	fpos = getUpdatePeriodFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the GPIB Devices Settings
	fpos = getGpibDevsFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Force Build Checkbox
	fpos = getForceBuildChkFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Use Compression Preferences Menu option
	fpos = getUseCompressionFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the Use Simple Timing Preferences Menu option
	fpos = getUseSimpleTimingFromFile(fbuffer, fpos_file_end);
	if( fpos < 0 ){ fclose(fbuffer); return -1; }// pass though error

	// Get the MultiScan object
	fpos = getMultiScanFromFile(fbuffer, fpos_file_end);


	if( fpos < 0 && fpos != -2 ){// pass though error, but not if fpos == -2 then we have reached the end of the file as we expect.
		fclose(fbuffer);
		return fpos;
	}
	if( fpos > 0 ){
		printf("There is still more file to read but nothing to put it in!\n");
		fclose(fbuffer);
		return -1;
	}

	// These lines should go in the correct subfunction, but because SetAnalogChannels affects both analog&dds&anritsu rows,
	//  I put them here so it happens once after all of them have been loaded.
	LoadDDSSettings();// Enters the DDSsettings into the GUI Panel
	LoadAnritsuSettings();// Enters the Anritsu settings into the GUI Panel
	SetAnalogChannels();// Put the labels and units for analog (+dds&anritsu) rows
	SetDigitalChannels();// Put labels for dig channels. And checks that none of the digital lines to dds's are accidentally in use.

	// We have finally gotten to this point so close the file and return success
	fclose(fbuffer);
	printf("Loaded file successfully\n");
	return 0;
}


int checkVersionFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<SeqVer>";
	char etag[] = "</SeqVer>";
	int max_dims = 1;// SeqVer is 1D (chars)

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	char cbuff[512];
	int clen = 512;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size >= clen ){
		printf("Binary data in file for tag |%s| is too big for buffer\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(cbuff, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != elem_size*linear_size ){
		printf("Expected to read more bytes from file for tag |%s|\n", stag);
		return -1;
	}

	printf("SeqVer loaded is |%.*s|\n", elem_size*linear_size, cbuff);// print the SeqVer line

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header
	return fpos;
}

long getSaveVersionFromFile(FILE *fbuff, long fpos_eof, int *majorVer, int *minorVer)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<SaveVersion>";
	char etag[] = "</SaveVersion>";
	int max_dims = 1;// SaveVersion is 1D (chars)

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	char cbuff[512] = "";
	int clen = 512;
	char *cptr = cbuff;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size >= clen ){
		printf("Binary data in file for tag |%s| is too big for buffer\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(cbuff, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	// Check that the file version is valid (##.#)
	*(cptr+linear_size) = '\0';// explicitly set the char after the string to null cause fread doesn't care
	sscanf(cptr, "%d.%d", majorVer, minorVer);// get major and minor version numbers
	printf("Major.minor version: %d.%d\n", *majorVer, *minorVer);
	if( majorVer < 17 ){// if save file doesn't use tags (this shouldn't ever be true hehe)
		printf("Version of save file is invalid\n");
		return -1;
	}
	// at the moment we don't care about the minor version

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header
	return fpos;
}

int putTimeArrayToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// double TimeArray[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<TimeArray>";
	char etag[] = "</TimeArray>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(TimeArray[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&TimeArray, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getTimeArrayFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<TimeArray>";
	char etag[] = "</TimeArray>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(TimeArray) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&TimeArray, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putAnalogTableToFile(FILE *fbuff)
{
	int elems_writ;
	int i,j,k;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct AnalogTableValues AnalogTable[NUMBEROFCOLUMNS+1][NUMBERANALOGROWS+1][NUMBEROFPAGES];// vars.h line
	// But, we only want to write the analog channels to this tag ie. the first (NUMBERANALOGCHANNELS+1) rows.
	char stag[] = "<AnalogTable>";
	char etag[] = "</AnalogTable>";
	int num_dims = 3;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBERANALOGCHANNELS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(AnalogTable[0][0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass through error

	// Now for convenience when loading, we should write the array by pages first, then cols and rows
	for( i=0; i < dims[2]; ++i ){// iterate over pages
		for( j=0; j < dims[0]; ++j ){// iterate over cols
			for( k=0; k < dims[1]; ++k ){// iterate over rows
				elems_writ = fwrite(&AnalogTable[j][k][i], elem_size, 1, fbuff);// write binary data
				if( elems_writ != 1 ){
					fclose(fbuff);
					printf("Failed to write the correct number of elems for tag |%s|\n", stag);
					return -1;
				}
			}
		}
	}

	//elems_writ = fwrite(&AnalogTable, elem_size, linear_size, fbuff);// write binary data
	//if( elems_writ != linear_size ){
	//	fclose(fbuff);
	//	printf("Failed to write the correct number of elems for tag |%s|\n", stag);
	//	return -1;
	//}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getAnalogTableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<AnalogTable>";
	char etag[] = "</AnalogTable>";
	int max_dims = 3;// AnalogTable is 3D (cols x rows x pages)

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	int i,j,k;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size

	// Do error checking and sanity checking
	if( elem_size != sizeof(AnalogTable[0][0][0]) ){
		printf("Incorrect element size (%d), expected (%d), declared for tag |%s|\n",
					elem_size, sizeof(AnalogTable[0][0][0]), stag);
		return -1;
	}
	if( dims[0] != (NUMBEROFCOLUMNS+1) ){// incorrect number of columns
		printf("Incorrect number of columns (%d) declared for tag |%s|\n", dims[0], stag);
		return -1;
	}
	if( dims[2] != (NUMBEROFPAGES) ){// incorrect number of pages
		printf("Incorrect number of pages (%d) declared for tag |%s|\n", dims[2], stag);
		return -1;
	}
	if( dims[1] != (NUMBERANALOGCHANNELS+1) ){// a different number of rows is not necessarily an error
		if( dims[1] > (NUMBERANALOGCHANNELS+1) ){
			printf("Rows declared in file (%d) is larger than max (%d) declared for tag |%s|\n",
				   dims[1], (NUMBERANALOGCHANNELS+1), stag);
			return -1;
		}
		// now we know that the number of rows is smaller, let's also check if it's also a multiple of 8
		if( (dims[1]-1)%8 != 0 ){// dims[1]-1 should be the number of actual analog channels
			printf("Analog channels implied by file (%d) is not a multiple of 8 for tag |%s|\n",
				   dims[1]-1, stag);
			return -1;
		}
		// so now we have an acceptable number of rows in the file
		printf("Number of rows declared in file (%d) is acceptable for tag |%s|\n",
				dims[1], stag);

		// and since the number of rows is less than max, reinitialize them all.
		initializeAnalogTable();
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	// Parallelling the put fn, we must read each element separately
	for( i=0; i < dims[2]; ++i ){// iterate over pages
		for( j=0; j < dims[0]; ++j ){// iterate over cols
			for( k=0; k < dims[1]; ++k ){// iterate over rows
				elems_read = fread(&AnalogTable[j][k][i], elem_size, 1, fbuff);// read binary data
				if( elems_read != 1 ){
					printf("Expected to read more elements from file for tag |%s|\n", stag);
					return -1;
				}
			}
		}
	}

	//elems_read = fread(&AnalogTable, elem_size, linear_size, fbuff);// load the binary data directly into the array
	//if( elems_read != linear_size ){// didn't read expected number of elements
	//	printf("Expected to read more elements from file for tag |%s|\n", stag);
	//	return -1;
	//}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDigitalTableToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// int DigTableValues[NUMBEROFCOLUMNS+1][MAXDIGITAL][NUMBEROFPAGES];// vars.h line
	char stag[] = "<DigitalTable>";
	char etag[] = "</DigitalTable>";
	int num_dims = 3;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBERDIGITALCHANNELS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(DigTableValues[0][0][0]);
	int linear_size;

	int i,j,k;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Now for convenience when loading, we should write the array by pages first, then cols and rows
	for( i=0; i < dims[2]; ++i ){// iterate over pages
		for( j=0; j < dims[0]; ++j ){// iterate over cols
			for( k=0; k < dims[1]; ++k ){// iterate over rows
				elems_writ = fwrite(&DigTableValues[j][k][i], elem_size, 1, fbuff);// write binary data
				if( elems_writ != 1 ){
					fclose(fbuff);
					printf("Failed to write the correct number of elems for tag |%s|\n", stag);
					return -1;
				}
			}
		}
	}
	/*elems_writ = fwrite(&DigTableValues, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}*/

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDigitalTableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DigitalTable>";
	char etag[] = "</DigitalTable>";
	int max_dims = 3;// DigTableValues is 3D (cols x maxrows x pages)

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	int i,j,k;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size

	// Do error checking and sanity checking
	if( elem_size != sizeof(DigTableValues[0][0][0]) ){
		printf("Incorrect element size (%d), expected (%d), declared for tag |%s|\n",
					elem_size, sizeof(DigTableValues[0][0][0]), stag);
		return -1;
	}
	if( dims[0] != (NUMBEROFCOLUMNS+1) ){// incorrect number of columns
		printf("Incorrect number of columns (%d) declared for tag |%s|\n", dims[0], stag);
		return -1;
	}
	if( dims[2] != (NUMBEROFPAGES) ){// incorrect number of pages
		printf("Incorrect number of pages (%d) declared for tag |%s|\n", dims[2], stag);
		return -1;
	}
	if( dims[1] != (NUMBERDIGITALCHANNELS+1) ){// a different number of rows is not necessarily an error
		if( dims[1] > (NUMBERDIGITALCHANNELS+1) ){
			printf("Rows declared in file (%d) is larger than max (%d) declared for tag |%s|\n",
				   dims[1], (NUMBERDIGITALCHANNELS+1), stag);
			return -1;
		}
		// now we know that the number of rows is smaller, let's also check if it's also a multiple of 8
		if( (dims[1]-1)%8 != 0 ){// dims[1]-1 should be the number of actual digital channels
			printf("Digital channels implied by file (%d) is not a multiple of 8 for tag |%s|\n",
				   dims[1]-1, stag);
			return -1;
		}
		// so now we have an acceptable number of rows in the file
		printf("Number of rows declared in file (%d) is acceptable for tag |%s|\n",
				dims[1], stag);

		// and since the number of rows is less than max, reinitialize them all.
		initializeDigArray();
	}

	/*if( elem_size*linear_size != sizeof(DigTableValues) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}*/

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	// Parallelling the put fn, we must read each element separately
	for( i=0; i < dims[2]; ++i ){// iterate over pages
		for( j=0; j < dims[0]; ++j ){// iterate over cols
			for( k=0; k < dims[1]; ++k ){// iterate over rows
				elems_read = fread(&DigTableValues[j][k][i], elem_size, 1, fbuff);// read binary data
				if( elems_read != 1 ){
					printf("Expected to read more elements from file for tag |%s|\n", stag);
					return -1;
				}
			}
		}
	}

	/*elems_read = fread(&DigTableValues, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}*/

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putAnalogChPropsToFile(FILE *fbuff)
{
	int elems_writ;
	int i;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// AChName[MAXANALOG+NUMBERDDS];// vars.h line
	// AChName only stores the analog channel properties and the DDS names. The laser/anritsu are separate.
	// So we only want to write those rows ie. (NUMBERANALOGCHANNELS+1) for the analog channels
	//  and NUMBERDDS for the DDSs.
	char stag[] = "<AnalogChProps>";
	char etag[] = "</AnalogChProps>";
	int num_dims = 1;
	int dims[] = {(NUMBERANALOGCHANNELS+1)+NUMBERDDS};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(AChName[0]);
	int linear_size;

	printf("dims: %d\n", dims[0]);

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Only write the elements that are meaningful
	for( i=0; i < dims[0]; ++i ){
		elems_writ = fwrite(&AChName[i], elem_size, 1, fbuff);// write binary data
		if( elems_writ != 1 ){
			fclose(fbuff);
			printf("Failed to write the correct number of elems for tag |%s|\n", stag);
			return -1;
		}
	}

	//elems_writ = fwrite(&AChName, elem_size, linear_size, fbuff);// write binary data
	//if( elems_writ != linear_size ){
	//	fclose(fbuff);
	//	printf("Failed to write the correct number of elems for tag |%s|\n", stag);
	//	return -1;
	//}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getAnalogChPropsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<AnalogChProps>";
	char etag[] = "</AnalogChProps>";
	int max_dims = 1;
	int ddsRowOffset = (NUMBERANALOGCHANNELS+1)+1;// 1 based index of dds1 in AChNames

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	int i;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size

	// Do error checking and sanity checking
	if( elem_size != sizeof(AChName[0]) ){
		printf("Incorrect element size (%d), expected (%d), declared for tag |%s|\n",
					elem_size, sizeof(AChName[0]), stag);
		return -1;
	}
	if( dims[0] != (NUMBERANALOGCHANNELS+1)+NUMBERDDS ){// a different number of rows is not necessarily an error
		if( dims[0] > (NUMBERANALOGCHANNELS+1)+NUMBERDDS ){
			printf("Rows declared in file (%d) is larger than max (%d) declared for tag |%s|\n",
				   dims[0], (NUMBERANALOGCHANNELS+1)+NUMBERDDS, stag);
			return -1;
		}
		// now we know that the number of rows is smaller, let's also check if it's also a multiple of 8
		//  plus 3 for the dds's.
		if( (dims[0]-1-3)%8 != 0 ){// dims[1]-1 should be the number of actual analog channels, -3 for dds
			printf("Analog channels implied by file (%d) is not a multiple of 8 for tag |%s|\n",
				   dims[0]-1-3, stag);
			return -1;
		}
		// so now we have an acceptable number of rows in the file
		printf("Number of rows declared in file (%d) is acceptable for tag |%s|\n",
				dims[0], stag);

		// and since the number of rows is less than max, reinitialize them all.
		initializeAnalogChProps();
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	// Parallelling the put fn, we read only what was saved
	// First read the analog channels
	for( i=0; i < dims[0]-3; ++i ){// -3 to not read the 3 dds's yet
		elems_read = fread(&AChName[i], elem_size, 1, fbuff);// read binary data
		if( elems_read != 1 ){
			printf("Expected to read more elements from file for tag |%s|\n", stag);
			return -1;
		}
	}
	// Now read in the DDS's to the correct row
	for( i=0; i < 3; ++i ){
		elems_read = fread(&AChName[ddsRowOffset+i], elem_size, 1, fbuff);// read binary data
		if( elems_read != 1 ){
			printf("Expected to read more elements from file for tag |%s|\n", stag);
			return -1;
		}
	}

	//elems_read = fread(&AChName, elem_size, linear_size, fbuff);// load the binary data directly into the array
	//if( elems_read != linear_size ){// didn't read expected number of elements
	//	printf("Expected to read more elements from file for tag |%s|\n", stag);
	//	return -1;
	//}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDigitalChPropsToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// DChName[MAXDIGITAL];// vars.h line
	char stag[] = "<DigitalChProps>";
	char etag[] = "</DigitalChProps>";
	int num_dims = 1;
	int dims[] = {(NUMBERDIGITALCHANNELS+1)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(DChName[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Only write the elements that are meaningful
	for( int i=0; i < dims[0]; ++i ){
		elems_writ = fwrite(&DChName[i], elem_size, 1, fbuff);// write binary data
		if( elems_writ != 1 ){
			fclose(fbuff);
			printf("Failed to write the correct number of elems for tag |%s|\n", stag);
			return -1;
		}
	}

	/*elems_writ = fwrite(&DChName, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}*/

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDigitalChPropsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DigitalChProps>";
	char etag[] = "</DigitalChProps>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size

	// Do error checking and sanity checking
	if( elem_size != sizeof(DChName[0]) ){
		printf("Incorrect element size (%d), expected (%d), declared for tag |%s|\n",
					elem_size, sizeof(DChName[0]), stag);
		return -1;
	}
	if( dims[0] != (NUMBERDIGITALCHANNELS+1) ){// a different number of rows is not necessarily an error
		if( dims[0] > (NUMBERDIGITALCHANNELS+1) ){
			printf("Rows declared in file (%d) is larger than max (%d) declared for tag |%s|\n",
				   dims[0], (NUMBERDIGITALCHANNELS+1), stag);
			return -1;
		}
		// now we know that the number of rows is smaller, let's also check if it's also a multiple of 8
		if( (dims[0]-1)%8 != 0 ){// dims[0]-1 should be the number of actual digital channels
			printf("Digital channels implied by file (%d) is not a multiple of 8 for tag |%s|\n",
				   dims[0]-1, stag);
			return -1;
		}
		// so now we have an acceptable number of rows in the file
		printf("Number of rows declared in file (%d) is acceptable for tag |%s|\n",
				dims[0], stag);

		// and since the number of rows is less than max, reinitialize them all.
		initializeDigChProps();
	}

	/*if( elem_size*linear_size != sizeof(DChName) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}*/

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	// Parallelling the put fn, we read only what was saved
	for( i=0; i < dims[0]; ++i ){
		elems_read = fread(&DChName[i], elem_size, 1, fbuff);// read binary data
		if( elems_read != 1 ){
			printf("Expected to read more elements from file for tag |%s|\n", stag);
			return -1;
		}
	}
	/*elems_read = fread(&DChName, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}*/

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDds1TableToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// ddsoptions_struct ddstable[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<DDS1Table>";
	char etag[] = "</DDS1Table>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(ddstable[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&ddstable, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDds1TableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DDS1Table>";
	char etag[] = "</DDS1Table>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(ddstable) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&ddstable, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDds2TableToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// dds2options_struct dds2table[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<DDS2Table>";
	char etag[] = "</DDS2Table>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(dds2table[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&dds2table, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDds2TableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DDS2Table>";
	char etag[] = "</DDS2Table>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(dds2table) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&dds2table, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDds3TableToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// dds3options_struct dds3table[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<DDS3Table>";
	char etag[] = "</DDS3Table>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(dds3table[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&dds3table, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDds3TableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DDS3Table>";
	char etag[] = "</DDS3Table>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(dds3table) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&dds3table, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDdsGlobalsToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	//struct DDSClock{// vars.h line
	//		double	extclock;
	//		int 	PLLmult;
	//		double	clock;// clock is a derived quantity from extclock and PLLmult
	//}	DDSFreq;
	char stag[] = "<DdsGlobals>";
	char etag[] = "</DdsGlobals>";
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(DDSFreq);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&DDSFreq, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDdsGlobalsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DdsGlobals>";
	char etag[] = "</DdsGlobals>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(DDSFreq) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	// Read the data into DDSFreq and also set the DDSsettings panel
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&DDSFreq, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	LoadDDSSettings();// load DDSFreq global var into ddssettings panel

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putDdsEorsToFile(FILE *fbuff)
{// Lots of manual operations/customizations in this fn

	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// PANEL_NUM_DDS_OFFSET
	// PANEL_NUM_DDS2_OFFSET
	// PANEL_NUM_DDS3_OFFSET
	char stag[] = "<DdsEors>";
	char etag[] = "</DdsEors>";
	int num_dims = 1;
	int dims[] = {(3)};// ordering is: object[dims[0]][dims[1]]...
	double ddseors[dims[0]];
	int elem_size = sizeof(ddseors[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Get each EOR and write array to file

	GetCtrlVal( panelHandle, PANEL_NUM_DDS_OFFSET,  &(ddseors[0]) );
	GetCtrlVal( panelHandle, PANEL_NUM_DDS2_OFFSET, &(ddseors[1]) );
	GetCtrlVal( panelHandle, PANEL_NUM_DDS3_OFFSET, &(ddseors[2]) );

	elems_writ = fwrite(&ddseors, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getDdsEorsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<DdsEors>";
	char etag[] = "</DdsEors>";
	int max_dims = 1;
	double *ddseors;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(double) * 3 ){// HARDCODED
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	//printf("elem_size: %d, linear_size: %d, dims[0]:%d\n",elem_size,linear_size,dims[0]);// debug

	// Allocate the temporary required amount of memory
	ddseors = (double *) malloc( elem_size * linear_size );

	//printf("ddseors allocated\n");// debug

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(ddseors, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		free(ddseors);
		return -1;
	}
	// Set the dds eor cells
	SetCtrlVal( panelHandle, PANEL_NUM_DDS_OFFSET, ddseors[0] );
	SetCtrlVal( panelHandle, PANEL_NUM_DDS2_OFFSET, ddseors[1] );
	SetCtrlVal( panelHandle, PANEL_NUM_DDS3_OFFSET, ddseors[2] );

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		free(ddseors);
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	free(ddseors);
	return fpos;
}

int putLaserTableToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct LaserTableValues LaserTable[NUMBERLASERS][NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<LaserTable>";
	char etag[] = "</LaserTable>";
	int num_dims = 3;
	int dims[] = {(NUMBERLASERS), (NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(LaserTable[0][0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&LaserTable, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getLaserTableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<LaserTable>";
	char etag[] = "</LaserTable>";
	int max_dims = 3;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(LaserTable) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&LaserTable, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putLaserPropsToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct LaserProps{...}LaserProperties[NUMBERLASERS];// vars.h line
	char stag[] = "<LaserProperties>";
	char etag[] = "</LaserProperties>";
	int num_dims = 1;
	int dims[] = {(NUMBERLASERS)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(LaserProperties[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&LaserProperties, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getLaserPropsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<LaserProperties>";
	char etag[] = "</LaserProperties>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(LaserProperties) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&LaserProperties, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	SetLaserLabels();// update the row labels for the lasers

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putAnritsuTableToFile(FILE *fbuff)// this table isn't used anymore but it might in the future so why not save it too
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// anritsuoptions_struct anritsutable[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<AnritsuTable>";
	char etag[] = "</AnritsuTable>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(anritsutable[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&anritsutable, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getAnritsuTableFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<AnritsuTable>";
	char etag[] = "</AnritsuTable>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(anritsutable) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&anritsutable, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putAnritsuPropsToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct AnritsuSetting AnritsuSettingValues[NUMBEROFANRITSU];// vars.h line
	char stag[] = "<AnritsuProps>";
	char etag[] = "</AnritsuProps>";
	int num_dims = 1;
	int dims[] = {(NUMBEROFANRITSU)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(AnritsuSettingValues[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Update AnritsuSettingsValues and then write to file
	// The other settings are set in the Anritsu panel
	// But because of the way the offset (EOR) cell is separate, it must be done manually
	GetCtrlVal( panelHandle, PANEL_ANRITSU_OFFSET, &AnritsuSettingValues[0].offset );
	elems_writ = fwrite(&AnritsuSettingValues, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getAnritsuPropsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<AnritsuProps>";
	char etag[] = "</AnritsuProps>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(AnritsuSettingValues) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	// Not just load into AnritsuSettingValues but also update the gui
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&AnritsuSettingValues, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	LoadAnritsuSettings();// Load values into the Anritsu panel
	SetCtrlVal( panelHandle, PANEL_ANRITSU_OFFSET, AnritsuSettingValues[0].offset );// set offset (EOR)

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putInfoArrayToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct InfoArrayValues InfoArray[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<InfoArray>";
	char etag[] = "</InfoArray>";
	int num_dims = 2;
	int dims[] = {(NUMBEROFCOLUMNS+1), (NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(InfoArray[0][0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&InfoArray, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getInfoArrayFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<InfoArray>";
	char etag[] = "</InfoArray>";
	int max_dims = 2;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(InfoArray) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&InfoArray, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putPageNamesToFile(FILE *fbuff)
{
	int elems_writ;
	int i;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// For the page names, they are stored in the buttons themselves.
	// int PANEL_TB_SHOWPHASE[NUMBEROFPAGES]; is an array of the handles to each button.
	// And the content we are interested in can be found by
	// GetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_OFF_TEXT, char_buffer);
	// or
	// GetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_ON_TEXT, char_buffer);
	// since the on and off states have the same name.
	// We will assume (correctly) that the button text is not longer than 32 chars and write
	// 32 chars for each name to allow for the loading to know where the boundary between names is.
	// Note that PANEL_TB_SHOWPHASE[0] is unset and so is nonsense. I would save a dummy set of '\0'
	// but loading that in the loading fn would change the expectation that [0] is nonsense.
	// So set dims to be one less and start for loops from i=1 and compare with '<='.
	char stag[] = "<PageNames>";
	char etag[] = "</PageNames>";
	char text_buff[32] = "";
	int max_text_len = 32;
	int text_len;
	int num_dims = 1;
	int dims[] = {(NUMBEROFPAGES-1)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(text_buff);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Here we need to do a little more work than simply dumping the binary array to file
	for( i = 1; i <= linear_size; ++i )
	{
		// First clear the buffer
		nullCharBuff(text_buff, max_text_len);

		// Check that the text length is less than the buffer length to prevent overflows
		GetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_OFF_TEXT_LENGTH, &text_len);
		//printf("text_len: %d\n", text_len);// debug
		if( text_len > max_text_len ){
			fclose(fbuff);
			printf("The page name of page %d is too long for the save buffer\n", i);
			return -1;
		}

		// Get the (off state) text of the button
		GetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_OFF_TEXT, text_buff);
		//printf("text_buff |%s|\n", text_buff);// debug

		// And finally write it to the file
		elems_writ = fwrite(&text_buff, elem_size, 1, fbuff);

		//printf("elems_writ: %d, elem_size: %d, max_text_len: %d\n", elems_writ, elem_size, max_text_len);//debug
		if( elems_writ != 1 ){
			fclose(fbuff);
			printf("Failed to write the correct number of bytes for page name %d\n", i);
			return -1;
		}
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getPageNamesFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<PageNames>";
	char etag[] = "</PageNames>";
	int max_dims = 1;
	char text_buff[32] = "";
	int max_text_len = 32;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;
	int i;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size != max_text_len ){
		printf("Char blocks for page names are not the correct size\n");
		return -1;
	}
	if( linear_size != (NUMBEROFPAGES-1) ){
		printf("Number of page names in file is not correct\n");
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	for( i = 1; i <= linear_size; ++i ){
		elems_read = fread(&text_buff, elem_size, 1, fbuff);// load chars into buffer
		if( elems_read != 1 ){
			printf("Failed to read all of char block for page name %d\n", i);
			return -1;
		}
		//printf("text_buff |%s|\n", text_buff);// debug
		SetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_OFF_TEXT, text_buff);
		SetCtrlAttribute(panelHandle, PANEL_TB_SHOWPHASE[i], ATTR_ON_TEXT, text_buff);
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putPageCheckboxesToFile(FILE *fbuff)// These used to be saved in the .pan file in V16 and lower
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// There are no callbacks for the enable checkboxes for each page. The binary values are stored in global var "ischecked".
	// The handles for each checkbox are PANEL_CHECKBOX, PANEL_CHECKBOX_2, etc.
	// The handles are also saved in the array PANEL_CHKBOX[NUMBEROFPAGES].
	// However, cannot rely on values in ischecked being accurate so call checkActivePages() to set ischecked before saving.
	// Or just do it manually.
	// int ischecked[NUMBEROFPAGES];// vars.h line
	char stag[] = "<PageCheckboxes>";
	char etag[] = "</PageCheckboxes>";
	int num_dims = 1;
	int dims[] = {(NUMBEROFPAGES)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(ischecked[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	checkActivePages();// insure that global var ischecked is up to date
	elems_writ = fwrite(&ischecked, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getPageCheckboxesFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<PageCheckboxes>";
	char etag[] = "</PageCheckboxes>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(ischecked) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&ischecked, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	setActivePages();// set the checkboxes according to the values in global var ischecked

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putUpdatePeriodToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct InfoArrayValues InfoArray[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];// vars.h line
	char stag[] = "<UpdatePeriod>";
	char etag[] = "</UpdatePeriod>";
	int updatePeriod;
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(updatePeriod);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	updatePeriod = getUpdatePeriodFromMenu();
	elems_writ = fwrite(&updatePeriod, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getUpdatePeriodFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<UpdatePeriod>";
	char etag[] = "</UpdatePeriod>";
	int updatePeriod;
	double ev_period;
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(updatePeriod) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&updatePeriod, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	ev_period = setUpdatePeriodToMenu(updatePeriod);// set the menu options
	if( ev_period < 0.0 ){// if error
		printf("An error occured setting the update period menus\n");
		return -1;
	}
	EventPeriod = ev_period;// if things are good then set the global var EventPeriod

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putGpibDevsToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	//	struct GPIBDDeviceProperties{// vars.h line
	//	int		address;	// GPIB address (1..32), 0 means: not initialized
	//	char    devname[50]; // name of the device
	//	char	cmdmask[1024];
	//	char	command[1024];
	//	char	lastsent[1024];
	//	double	value[20];
	//	BOOL	active;
	//	} GPIBDev[NUMBERGPIBDEV];
	char stag[] = "<GpibDevs>";
	char etag[] = "</GpibDevs>";
	int num_dims = 1;
	int dims[] = {(NUMBERGPIBDEV)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(GPIBDev[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&GPIBDev, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getGpibDevsFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<GpibDevs>";
	char etag[] = "</GpibDevs>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(GPIBDev) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&GPIBDev, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putForceBuildChkToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// GetCtrlVal(panelHandle, PANEL_FORCE_BUILD_CHK, &forceBuild);
	char stag[] = "<ForceBuildCheckbox>";
	char etag[] = "</ForceBuildCheckbox>";
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int forceBuild;
	int elem_size = sizeof(forceBuild);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Get the status of the Force build checkbox and write to file
	GetCtrlVal(panelHandle, PANEL_FORCE_BUILD_CHK, &forceBuild);
	elems_writ = fwrite(&forceBuild, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getForceBuildChkFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<ForceBuildCheckbox>";
	char etag[] = "</ForceBuildCheckbox>";
	int max_dims = 1;
	int forceBuild;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(forceBuild) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&forceBuild, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	SetCtrlVal(panelHandle, PANEL_FORCE_BUILD_CHK, forceBuild);// set the checkbox

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putUseCompressionToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// #define  MENU_PREFS                      58
	// #define  MENU_PREFS_COMPRESSION          59	callback function: COMPRESSION_CALLBACK
	// #define  MENU_PREFS_SIMPLETIMING         60	callback function: SIMPLETIMING_CALLBACK
	// #define  MENU_PREFS_SHOWARRAY            61	callback function: SHOWARRAY_CALLBACK// BuildUpdateList pukes first 1000 when true
	// #define  MENU_PREFS_DDS_OFF              62	callback function: DDS_OFF_CALLBACK// This just sets ddstable[i][j].isStop = True
	// #define  MENU_PREFS_STREAM_SETTINGS      63	callback function: STREAM_CALLBACK// Stream (settings) was not implemented
	char stag[] = "<UseCompression>";
	char etag[] = "</UseCompression>";
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int useCompression;
	int elem_size = sizeof(useCompression);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Get from menu and write
	GetMenuBarAttribute( menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, &useCompression );
	elems_writ = fwrite(&useCompression, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getUseCompressionFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<UseCompression>";
	char etag[] = "</UseCompression>";
	int max_dims = 1;
	int useCompression;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(useCompression) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	// Read option from file and set menu accordingly
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&useCompression, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	SetMenuBarAttribute( menuHandle, MENU_PREFS_COMPRESSION, ATTR_CHECKED, useCompression );

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putUseSimpleTimingToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// See putUseCompressionToFile() comment for preferences menu details
	char stag[] = "<UseSimpleTiming>";
	char etag[] = "</UseSimpleTiming>";
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int useSimpleTiming;
	int elem_size = sizeof(useSimpleTiming);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	// Get from menu and write
	GetMenuBarAttribute( menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, &useSimpleTiming );
	elems_writ = fwrite(&useSimpleTiming, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getUseSimpleTimingFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<UseSimpleTiming>";
	char etag[] = "</UseSimpleTiming>";
	int max_dims = 1;
	int useSimpleTiming;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(useSimpleTiming) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	// Read option from file and set menu accordingly
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&useSimpleTiming, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	SetMenuBarAttribute( menuHandle, MENU_PREFS_SIMPLETIMING, ATTR_CHECKED, useSimpleTiming );

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}

int putMultiScanToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct MultiScanValues{...; struct MultiScanParameters Par[NUMMAXSCANPARAMETERS]; } MultiScan;// vars.h line
	// Note that if NUMMAXSCANPARAMETERS changes then loading a version with an old num will fail.
	// Could change the code to have the parameters struct be a pointer to another global variable.
	char stag[] = "<MultiScan>";
	char etag[] = "</MultiScan>";
	int num_dims = 1;
	int dims[] = {(1)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(MultiScan);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	getMultiScanGuiVals();// update the MultiScan variable so writing makes sense in all cases

	elems_writ = fwrite(&MultiScan, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getMultiScanFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<MultiScan>";
	char etag[] = "</MultiScan>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(MultiScan) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&MultiScan, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}
	setMultiScanPosTable();

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}


int putScanBufferToFile(FILE *fbuff)
{
	int elems_writ;

	// Particulars of the object to write (don't forget to change the actual data write line too)
	// struct ScanBuff{ // Needing a scan buffer to watch scan, since list can be updated during scan
	// int Iteration;
	// int Step;
	// double Value;
	// double MultiScanValue[NUMMAXSCANPARAMETERS]; // Added for MultiScans, not interferring with other scans
	// char Time[SCANBUFFER_TIMEBUFFER_LENGTH];
	// int BufferSize;
	//} ScanBuffer[SCANBUFFER_LENGTH];
	char stag[] = "<ScanBuffer>";
	char etag[] = "</ScanBuffer>";
	int num_dims = 1;
	int dims[] = {(SCANBUFFER_LENGTH)};// ordering is: object[dims[0]][dims[1]]...
	int elem_size = sizeof(ScanBuffer[0]);
	int linear_size;

	linear_size = writeHeader(fbuff, stag, elem_size, num_dims, dims);// write header
	if( linear_size < 0 ){ return linear_size; }// pass though error

	elems_writ = fwrite(&ScanBuffer, elem_size, linear_size, fbuff);// write binary data

	if( elems_writ != linear_size ){
		fclose(fbuff);
		printf("Failed to write the correct number of elems for tag |%s|\n", stag);
		return -1;
	}

	return writeFooter(fbuff, etag);// write footer and pass through any errors
}

long getScanBufferFromFile(FILE *fbuff, long fpos_eof)
{
	// Particulars of the object to load (don't forget to change the actual data write line too)
	char stag[] = "<ScanBuffer>";
	char etag[] = "</ScanBuffer>";
	int max_dims = 1;

	int elem_size;
	int num_dims;
	int dims[max_dims];
	int linear_size = 1;
	long fpos;
	int elems_read;

	fpos = readHeader(fbuff, stag, &elem_size, &num_dims, dims, max_dims, fpos_eof);
	// Do some simple checks
	if( fpos < 0 ){// if there was an err in readHeader (printf's in readHeader)
		return fpos;
	}
	for( int i=0; i<max_dims; ++i ){ linear_size *= dims[i]; }// calculate linear_size
	if( elem_size*linear_size != sizeof(ScanBuffer) ){
		printf("Binary data read from file for tag |%s| is not the correct size\n", stag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the binary data
	elems_read = fread(&ScanBuffer, elem_size, linear_size, fbuff);// load the binary data directly into the array
	if( elems_read != linear_size ){// didn't read expected number of elements
		printf("Expected to read more elements from file for tag |%s|\n", stag);
		return -1;
	}

	fpos = checkFooter(fbuff, etag, fpos_eof);
	if( fpos < 0 ){// pass though signal, either -1 for error or -2 for eof
		return fpos;
	}
	fseek(fbuff, fpos, SEEK_SET);// seek to the start of the next header

	return fpos;
}






void nullCharBuff(char *buff, int max_len)// simple fn to null all elements of a char buffer
{
	for( int i = 0; i < max_len; ++i ){
		buff[i] = '\0';
	}
	return;
}

int writeHeader(FILE *fbuff, char *stag, int elem_size, int num_dims, int *dims){
	// num_dims is the number of elements of array dims
	// return either linear_size (positive) or <0 for error
	// assumes tag is properly '\0' terminated

	char cbuff[512] = "";// header buffer
	char bf[256] = "";// assembly buffer
	int elems_writ;
	int linear_size = 1;

	// Make the header
	sprintf(cbuff, "%s~%d~%d~", stag, elem_size, num_dims);
	for( int i = 0; i < num_dims; ++i ){
		sprintf(bf, "%d~", dims[i]);
		strcat(cbuff, bf);
		linear_size = linear_size * dims[i];
	}

	// Write the header
	elems_writ = fwrite(cbuff, sizeof(char), strlen(cbuff), fbuff);

	// Some simple error checking
	if( elems_writ != strlen(cbuff) ){
		fclose(fbuff);
		printf("Failed to write correct number of chars in header for tag |%s|\n",stag);
		return -1;
	}

	return linear_size;
}

int writeFooter(FILE *fbuff, char *etag){
	// assumes tag is properly '\0' terminated

	char cbuff[512] = "";// header buffer
	int elems_writ;

	// Write the Footer
	sprintf(cbuff, etag);
	elems_writ = fwrite(cbuff, sizeof(char), strlen(cbuff), fbuff);

	// Some simple error checking
	if( elems_writ != strlen(cbuff) ){
		fclose(fbuff);
		printf("Failed to write correct number of chars in footer for tag |%s|\n",etag);
		return -1;
	}

	return 0;// return success
	// note that by convention this value is also the successful return value of the calling function
}

long readHeader(FILE *fbuff, char *tag, int *elem_size, int *num_dims, int *dims, const int max_dims, long fpos_eof){
	// fbuff's file pointer should be pointing at the start of the header ie. '<'
	// returns ftell at start of binary data that is to be read
	// assumes tag is properly '\0' terminated

	char cbuff[512] = "";
	int clen = 512;// max len of cbuff
	char *cptr = cbuff;
	int i;
	long fpos, fpos_binary;
	int elems_read;

	//printf("---Enter readHeader for tag:|%s|\n", tag);// debug

	fpos = ftell(fbuff);// First save position in file
	elems_read = fread(cbuff, sizeof(char), clen, fbuff);// read into char buffer
	if( elems_read != clen ){
		clen = elems_read;// prevent looking at bad data
		if( ftell(fbuff) >= fpos_eof ){
			printf("Will reach EOF in %d bytes\n", elems_read);
		}
		else {// there was some kind of error and we didn't read all the bytes we wanted to
			printf("Bytes read %d in readHeader less than buffer size.\n", elems_read);
			return -1;
		}
	}

	if( cbuff[0] == 'A' ){// only relevant for the first header
		printf("Savefile format seems to be pre V17\n");
		return -1;// return negative for error (not V17 and above)
	}

	//printf("cptr:|%.40s|\n", cptr);// debug
	//printf("etag:|%s|\n", tag);// debug
	if( strncmp(cptr,tag,strlen(tag)) == 0 ){// if strs match
		cptr += strlen(tag);// cptr now points to the first '~'
	}
	else {// we got a tag we didn't expect
		printf("Unexpected tag when expected |%s|\n", tag);
		return -1;
	}

	sscanf(cptr, "~%d~", elem_size);// Get the sizeof() of each element in the array
	if( *elem_size <= 0 ){// Simple error checking
		printf("Invalid element size ~%d~ for tag |%s|\n", *elem_size, tag);
		return -1;
	}
	++cptr; while( *cptr != '~' && cptr-cbuff < clen ){ ++cptr; }// Increment cptr to the tilde after the number

	sscanf(cptr, "~%d~", num_dims);// Get the number of dimensions
	if( *num_dims > max_dims ){
		printf("Invalid number of dims ~%d~ for tag |%s|\n", *num_dims, tag);
		return -1;
	}
	++cptr; while( *cptr != '~' && cptr-cbuff < clen ){ ++cptr; }// Increment cptr to the tilde after the number

	for( i = 0; i < *num_dims; ++i ){// Get the size of each dim
		sscanf(cptr, "~%d~", dims+i);// Get the length of the ith dim and put it in dims[i] by reference
		if( dims[i] <= 0 ){
			printf("Invalid dimension size ~%d~ for tag |%s|\n", dims[i], tag);
			return -1;
		}
		++cptr; while( *cptr != '~' && cptr-cbuff < clen ){ ++cptr; }
	}
	// Now we have parsed the entire header part

	fseek(fbuff, fpos, SEEK_SET);// seek back to the start of the header
	++cptr;// increment to point at the first byte of the binary data
	fpos_binary = fpos + (cptr-cbuff);// calculate the fpos for the binary data start
	//printf("diff %d\n", (cptr-cbuff));// debug
	//printf("fpbi %d\n", fpos_binary);// debug
	if( fpos_binary >= fpos_eof ){// if we have reached the end of the file
		printf("Binary data is past the end of the file\n");
		return -1;// contrary to checkFooter, having the location of the binary data be past the end of the file is an error
	}
	return fpos_binary;// return the offset from the start of the file of the binary data
}

long checkFooter(FILE *fbuff, char *endtag, long fpos_eof){
	// Check that the next part after the binary data is actually the end tag
	// returns fpos at the end of the end tag
	// assumes tag is properly '\0' terminated

	char cbuff[512] = "";
	int clen = 512;// max len of cbuff
	char *cptr = cbuff;
	long fpos, fpos_next;
	int elems_read;

	//printf("---Enter checkFooter for tag:|%s|\n", endtag);// debug

	fpos = ftell(fbuff);// First save position in file
	elems_read = fread(cbuff, sizeof(char), clen, fbuff);// read into char buffer
	if( elems_read != clen ){
		clen = elems_read;// prevent looking at bad data
		if( ftell(fbuff) >= fpos_eof ){
			printf("Will reach EOF in %d bytes\n", elems_read);
		}
		else {// there was some kind of error and we didn't read all the bytes we wanted to
			printf("Bytes read %d in checkFooter less than buffer size.\n", elems_read);
			return -1;
		}
	}

	//printf("cptr:|%.40s|\n", cptr);// debug
	//printf("etag:|%s|\n", endtag);// debug
	if( strncmp(cptr,endtag,strlen(endtag)) == 0 ){// if strs match
		cptr += strlen(endtag);// cptr now points to the char after the '>'
	}
	else {// we got an endtag we didn't expect
		printf("Unexpected endtag when expected tag |%s|\n", endtag);
		return -1;
	}

	fseek(fbuff, fpos, SEEK_SET);// seek back to the start of the footer
	fpos_next = fpos + (cptr-cbuff);// calc the position of the next header's '<'
	if( fpos_next >= fpos_eof ){// if we have reached the end of the file
		printf("Reached end of file in checkFooter for tag |%s|\n", endtag);
		return -2;
	}
	return fpos_next;
}




int LoadArraysV16intoV17(char savedname[500], int csize)
{
	// This is the function that I (Scott) implemented in the previous version of V17.
	// For now let's focus on writing a new save fn without having to work through the
	// details of reverse engineering the V16 file format.
	return 0;
}



/************************************************************************
Version 16 Save and Load
*************************************************************************/

void SaveArraysV16(char savedname[500],int csize)
{
	/*
		Stefan Trotzky -- V16.0.2 (2012-10-20)
		Added _all_ Toggle button names to the panel files.
	    Added the column labels to the panel files.
	    Added the sequencer version as first entry.
	    At the same time: reordered the arrays in the files to be more logical (e.g. grouped DDSs)

	    Stefan Trotzky -- V16.1.1 (2013-02-02)
	    Saving the newly added GPIB structure in a separate file for now. This should
	    be included in the existing file at a convenient time in the future. Also:
	    consider a more flexible file structure where adding variables does not spoil
	    downward compatibility.
	*/

	FILE *fdata;
	int i=0;
	int xval=NUMBEROFCOLUMNS,yval=NUMBERANALOGROWS,zval=NUMBEROFPAGES-1;
	int updatePer; //Update Period Check
	char buff[500]="",buff2[100]="", buff3[500]="";
	strncpy(buff,savedname,csize-4);
	strcat(buff,".arr");
	if((fdata=fopen(buff,"w"))==NULL)
	{
		strcpy(buff2,"Failed to save data arrays.");
		MessagePopup("Save Error",buff2);
	}
	else
	{
		strcpy(buff2,SEQUENCER_VERSION);
		fwrite(&buff2, sizeof buff2, 1,fdata);
		fwrite(&xval,sizeof xval, 1,fdata);
		fwrite(&yval,sizeof yval, 1,fdata);
		fwrite(&zval,sizeof zval, 1,fdata);
		// Times
		fwrite(&TimeArray,sizeof TimeArray,1,fdata);
		// Analog and Digital Channels
		fwrite(&AnalogTable,sizeof AnalogTable,1,fdata);
		fwrite(&DigTableValues,sizeof DigTableValues,1,fdata);
		fwrite(&AChName,sizeof AChName,1,fdata);
		fwrite(&DChName,sizeof DChName,1,fdata);
		// DDS settings
		fwrite(&ddstable,sizeof ddstable,1,fdata);
		fwrite(&dds2table,sizeof dds2table,1,fdata);
		fwrite(&dds3table,sizeof dds3table,1,fdata);
		// 'Laser" settings (used to be in .laser files)
		fwrite(&LaserProperties,sizeof LaserProperties, 1,fdata);
		fwrite(&LaserTable,sizeof LaserTable,1,fdata);
		// Anritsu settings
		fwrite(&AnritsuSettingValues,sizeof AnritsuSettingValues,1,fdata);
		// Column labels (new since V16.02)
		fwrite(&InfoArray, sizeof InfoArray,1,fdata);
		for (i=1;i<=zval;i++)
		{
			GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE[i],ATTR_OFF_TEXT, buff2);
			fwrite(&buff2,sizeof buff2,1,fdata);
		}

		// Update period
		updatePer = getUpdatePeriodFromMenu();
		fwrite(&updatePer,sizeof updatePer,1,fdata);

		// global DDS settings
		fwrite(&DDSFreq.extclock,sizeof DDSFreq.extclock,1,fdata);
		fwrite(&DDSFreq.PLLmult,sizeof DDSFreq.PLLmult,1,fdata);

		// Save SRS settings (obsolete with new GPIB feature -- 2013-01 -- to
		// be removed at a convenient point in time)
		fwrite(&GPIB_address,sizeof GPIB_address,1,fdata);
		fwrite(&SRS_amplitude,sizeof SRS_amplitude,1,fdata);
		fwrite(&SRS_offset,sizeof SRS_offset,1,fdata);
		fwrite(&SRS_frequency,sizeof SRS_frequency,1,fdata);
		fwrite(&GPIB_ON,sizeof GPIB_ON,1,fdata);

		fclose(fdata);

		strncat(buff3, savedname, csize-4);
		strcat(buff3,".gpib");
		if((fdata=fopen(buff3,"w"))==NULL)
		{
			printf("Failed to save GPIB settings.\n");
		}
		else
		{
			fwrite(&GPIBDev,(sizeof GPIBDev),1,fdata);
			fclose(fdata);
		}

	}
}


int LoadArraysV16(char savedname[500],int csize)
{
	/* Loads all Panel attributes and values which are not saved automatically by the NI function SavePanelState.
	   the values are stored in the .arr file by SaveArrays
	   x,y, and zval give the 3D table dimensions but are not critical to the saving/loading operation

	   Note that if the lengths of any of the data arrays are changed previous saves will not be able to be laoded.
	   If necessary See AdwinGUI Panel Converter V11-V12 (created June 01, 2006)
	*/

	FILE *fdata;
	int i=0;
	int xval=16, yval=16, zval=NUMBEROFPAGES-1, updatePer;
	int status;
	char buff[500]="", buff2[100]="", version[100]="", buff3[500]="";
	strncat(buff, savedname, csize-4);
	strcat(buff,".arr");
	if((fdata=fopen(buff,"r"))==NULL)
	{
		MessagePopup("Load Error","Failed to load data arrays (*.arr file)");
		return(0);
	}

	fread(&version,sizeof version, 1,fdata);
	// Check whether first line contains a sequencer version number (since V16.0.2)
	status = strncmp(version,SEQUENCER_VERSION,17);
	if (status == 0) // means that strings match
	{
		fread(&xval,sizeof xval, 1,fdata);
		fread(&yval,sizeof yval, 1,fdata);
		fread(&zval,sizeof zval, 1,fdata);
		// Time table
		fread(&TimeArray,(sizeof TimeArray),1,fdata);
		// Analog and digital table data
		fread(&AnalogTable,(sizeof AnalogTable),1,fdata);
		fread(&DigTableValues,(sizeof DigTableValues),1,fdata);
		fread(&AChName,(sizeof AChName),1,fdata);
		fread(&DChName,sizeof DChName,1,fdata);
		// DDS settings
		fread(&ddstable,(sizeof ddstable),1,fdata);
		fread(&dds2table,(sizeof dds2table),1,fdata);
		fread(&dds3table,(sizeof dds3table),1,fdata);
		// "Laser" settings
		fread(&LaserProperties,sizeof LaserProperties, 1,fdata);
		fread(&LaserTable,sizeof LaserTable,1,fdata);
		// Load Anritsu settings
		fread(&AnritsuSettingValues,sizeof AnritsuSettingValues,1,fdata);
		//Load Column labels (new since V16.02)
		fread(&InfoArray, sizeof InfoArray, 1, fdata);

		for (i=1;i<NUMBEROFPAGES;i++)
		{	// NOT IDEAL -- should get the number of pages from the panel files ...
			fread(&buff2,sizeof buff2,1,fdata);
			SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE[i],ATTR_OFF_TEXT, buff2);
			SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE[i],ATTR_ON_TEXT, buff2);
		}

		//Update Period Retrieved and Set
		fread(&updatePer,sizeof updatePer,1,fdata);
		EventPeriod = setUpdatePeriodToMenu(updatePer);// assume the value in the file is okay

		fread(&DDSFreq.extclock,sizeof DDSFreq.extclock,1,fdata);
		fread(&DDSFreq.PLLmult,sizeof DDSFreq.PLLmult,1,fdata);
		DDSFreq.clock = DDSFreq.extclock * DDSFreq.PLLmult;

		fread(&GPIB_address,sizeof GPIB_address,1,fdata);
		fread(&SRS_amplitude,sizeof SRS_amplitude,1,fdata);
		fread(&SRS_offset,sizeof SRS_offset,1,fdata);
		fread(&SRS_frequency,sizeof SRS_frequency,1,fdata);
		fread(&GPIB_ON,sizeof GPIB_ON,1,fdata);

		fclose(fdata);

		LoadDDSSettings();  //Enters the DDSsettings into the GUI Panel
		LoadAnritsuSettings(); //Enters the Anritsu settings into the GUI Panel
		SetAnalogChannels();
		SetDigitalChannels();

		// ST 2013-02-02 -- currently saving newly introduced GPIB settings in separate
		// file to maintain downward compatibility ... should be included in arr file at
		// some more convenient point (see comment in SaveArraysV16)
		strncat(buff3, savedname, csize-4);
		strcat(buff3,".gpib");
		if((fdata=fopen(buff3,"r"))==NULL)
		{
			printf("No GPIB settings found for loaded panel (*.gpib file).\n");
			printf("Keeping current GPIB settings.\n");
		}
		else
		{
			fread(&GPIBDev,(sizeof GPIBDev),1,fdata);
			fclose(fdata);
		}


		return(1);
	}
	else
	{
		fclose(fdata);
		MessagePopup("Loading Error","Could not find sequencer version in file.\nPossibly older version. Consider different loading option.");
		return(0);
	}
}





/************************************************************************
Version 15 Save and Load
*************************************************************************/

void SaveArraysV15(char savedname[500],int csize)
{
	// S. Trotzky -- 2013-01-30 -- removed this option from the menu. Keeping the function for now (never called)

	/* Saves all Panel attributes and values which are not saved automatically by the NI function SavePanelState
	   The values are stored in the .arr file
	   x,y, and zval give the 3D table dimensions but are not critical to the saving/loading operation

	   Note that if the lengths of any of the data arrays are changed previous saves will not be able to be laoded.
	   If necessary See AdwinGUI Panel Converter V11-V12 (created June 01, 2006)
	*/

	FILE *fdata;
	int i=0,j=0,k=0;
	int xval=NUMBEROFCOLUMNS,yval=NUMBERANALOGROWS,zval=NUMBEROFPAGES-1;
	int usupd5,usupd10,usupd15,usupd25,usupd50,usupd100,usupd1000,updatePer; //Update Period Check
	char buff[500]="",buff2[100];

	MessagePopup("Save Warning","Note that panels should only be saved in V15\nformat for testing. Use save option without\nversion number to save panels!");

	strncpy(buff,savedname,csize-4);
	strcat(buff,".arr");
	if((fdata=fopen(buff,"w"))==NULL)
	{
		strcpy(buff2,"Failed to save data arrays.");
		MessagePopup("Save Error",buff2);
	}

	fwrite(&xval,sizeof xval, 1,fdata);
	fwrite(&yval,sizeof yval, 1,fdata);
	fwrite(&zval,sizeof zval, 1,fdata);
	//now for the times.
	fwrite(&TimeArray,sizeof TimeArray,1,fdata);
	//and the analog data
	fwrite(&AnalogTable,sizeof AnalogTable,1,fdata);
	fwrite(&DigTableValues,sizeof DigTableValues,1,fdata);

	fwrite(&AChName,sizeof AChName,1,fdata);
	fwrite(&DChName,sizeof DChName,1,fdata);
	fwrite(&ddstable,sizeof ddstable,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	GetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_OFF_TEXT, buff2);
	fwrite(&buff2,sizeof buff2,1,fdata);
	fwrite(&dds2table,sizeof dds2table,1,fdata);
	fwrite(&dds3table,sizeof dds3table,1,fdata);

	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, &usupd5);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, &usupd10);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, &usupd15);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, &usupd25);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, &usupd50);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, &usupd100);
	GetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, &usupd1000);

	if (usupd5==1)
		updatePer=5;
	else if (usupd10==1)
		updatePer=10;
	else if (usupd15==1)
		updatePer=15;
	else if (usupd25==1)
		updatePer=25;
	else if (usupd50==1)
		updatePer=50;
	else if(usupd100==1)
		updatePer=100;
	else if(usupd1000==1)
		updatePer=1000;
	fwrite(&updatePer,sizeof updatePer,1,fdata);
	fwrite(&DDSFreq.extclock,sizeof DDSFreq.extclock,1,fdata);
	fwrite(&DDSFreq.PLLmult,sizeof DDSFreq.PLLmult,1,fdata);

	//Save SRS settings
	fwrite(&GPIB_address,sizeof GPIB_address,1,fdata);
	fwrite(&SRS_amplitude,sizeof SRS_amplitude,1,fdata);
	fwrite(&SRS_offset,sizeof SRS_offset,1,fdata);
	fwrite(&SRS_frequency,sizeof SRS_frequency,1,fdata);
	fwrite(&GPIB_ON,sizeof GPIB_ON,1,fdata);

	//Save Anritsu settings
	fwrite(&AnritsuSettingValues,sizeof AnritsuSettingValues,1,fdata);

	fclose(fdata);
}


void LoadArraysV15(char savedname[500],int csize)
{
	/* Loads all Panel attributes and values which are not saved automatically by the NI function SavePanelState.
	   the values are stored in the .arr file by SaveArrays
	   x,y, and zval give the 3D table dimensions but are not critical to the saving/loading operation

	   Note that if the lengths of any of the data arrays are changed previous saves will not be able to be laoded.
	   If necessary See AdwinGUI Panel Converter V11-V12 (created June 01, 2006)
	*/

	FILE *fdata;
	int i=0, j=0, k=0;
	int xval=16, yval=16, zval=10, updatePer;
	char buff[500]="", buff2[100]="";
	strncat(buff, savedname, csize-4);
	strcat(buff,".arr");
	if((fdata=fopen(buff,"r"))==NULL)
	{
		MessagePopup("Load Error","Failed to load data arrays (*.arr file)");
	//	exit(1);
	}

	fread(&xval,sizeof xval, 1,fdata);
	fread(&yval,sizeof yval, 1,fdata);
	fread(&zval,sizeof zval, 1,fdata);
	//now for the times.
	fread(&TimeArray,(sizeof TimeArray),1,fdata);
	//and the analog data
	fread(&AnalogTable,(sizeof AnalogTable),1,fdata);
	fread(&DigTableValues,(sizeof DigTableValues),1,fdata);
	fread(&AChName,(sizeof AChName),1,fdata);
	fread(&DChName,sizeof DChName,1,fdata);
	fread(&ddstable,(sizeof ddstable),1,fdata);
	fread(&buff2,sizeof buff2,1,fdata);

	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);

	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_ON_TEXT, buff2);
	fread(&dds2table,(sizeof dds2table),1,fdata);
	fread(&dds3table,(sizeof dds3table),1,fdata);

	//Update Period Retrieved and Set
	fread(&updatePer,sizeof updatePer,1,fdata);
	if (updatePer==5)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 1);
		EventPeriod=0.005;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	if (updatePer==10)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 1);
		EventPeriod=0.01;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	if (updatePer==15)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 1);
		EventPeriod=0.015;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	if (updatePer==25)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 1);
		EventPeriod=0.025;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	if (updatePer==50)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 1);
		EventPeriod=0.05;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	if (updatePer==100)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 1);
		EventPeriod=0.1;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	if (updatePer==1000)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 1);
		EventPeriod=1;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);

	fread(&DDSFreq.extclock,sizeof DDSFreq.extclock,1,fdata);
	fread(&DDSFreq.PLLmult,sizeof DDSFreq.PLLmult,1,fdata);
	DDSFreq.clock = DDSFreq.extclock * DDSFreq.PLLmult;

	fread(&GPIB_address,sizeof GPIB_address,1,fdata);
	fread(&SRS_amplitude,sizeof SRS_amplitude,1,fdata);
	fread(&SRS_offset,sizeof SRS_offset,1,fdata);
	fread(&SRS_frequency,sizeof SRS_frequency,1,fdata);
	fread(&GPIB_ON,sizeof GPIB_ON,1,fdata);

	//Load Anritsu settings
	fread(&AnritsuSettingValues,sizeof AnritsuSettingValues,1,fdata);

	// Reset InfoArray (not seved in these files) ... read from panel content below
	for(i=0;i<=NUMBEROFCOLUMNS;i++)// ramp over # of cells per page
	{
		for(k=0;k<NUMBEROFPAGES;k++)// ramp over pages
		{
			InfoArray[i][k].index = 0;
			InfoArray[i][k].value = 0.0;
			strcpy(InfoArray[i][k].text, "");
		}
	}

	fclose(fdata);

	LoadDDSSettings();  //Enters the DDSsettings into the GUI Panel
	LoadAnritsuSettings(); //Enters the Anritsu settings into the GUI Panel
	SetAnalogChannels();
	SetDigitalChannels();

	// The column labels are saved with the panel. Here, we read their values into the InforArray.
	// This way old panels are imported into the new structure (V16.0.2).
	for (k=1;k<NUMBEROFPAGES;k++)
	{
		for(i=1;i<=NUMBEROFCOLUMNS;i++)
		{
			GetTableCellVal(panelHandle, PANEL_OLD_LABEL[k], MakePoint(i,1), &buff);
			strcpy(InfoArray[i][k].text, buff);
		}
	}
}




/************************************************************************
Version 13 Load
*************************************************************************/

void LoadArraysV13(char savedname[500],int csize)
{
	/* Loads version 13 panel files (when the analog table had fewer rows)
	*/

	FILE *fdata;
	int i=0, j=0, k=0;
	int xval=16, yval=16, zval=10, updatePer;
	char buff[500]="", buff2[100]="";
	struct AnalogTableValues OldAnalogTable[NUMBEROFCOLUMNS+1][NUMBERANALOGCHANNELS+NUMBERDDS][NUMBEROFPAGES];
	strncat(buff, savedname, csize-4);
	strcat(buff,".arr");
	if((fdata=fopen(buff,"r"))==NULL)
	{
		MessagePopup("Load Error","Failed to load data arrays (*.arr file)");
	//	exit(1);
	}

	fread(&xval,sizeof xval, 1,fdata);
	fread(&yval,sizeof yval, 1,fdata);
	fread(&zval,sizeof zval, 1,fdata);
	//now for the times.
	fread(&TimeArray,(sizeof TimeArray),1,fdata);
	//and the analog data

	fread(&OldAnalogTable,(sizeof OldAnalogTable),1,fdata);

	for (i=0;i<NUMBEROFCOLUMNS+1;i++)
	{
		for (j=0;j<NUMBERANALOGCHANNELS+NUMBERDDS;j++)
		{
			for (k = 0; k<NUMBEROFPAGES;k++)
			{
				 AnalogTable[i][j][k] = OldAnalogTable[i][j][k];
			}
		}

	}

	fread(&DigTableValues,(sizeof DigTableValues),1,fdata);
	fread(&AChName,(sizeof AChName),1,fdata);
	fread(&DChName,sizeof DChName,1,fdata);
	fread(&ddstable,(sizeof ddstable),1,fdata);
	fread(&buff2,sizeof buff2,1,fdata);

	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE1,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);

	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE2,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE3,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE4,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE5,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE6,ATTR_ON_TEXT, buff2);
	fread(&buff2,sizeof buff2,1,fdata);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_OFF_TEXT, buff2);
	SetCtrlAttribute (panelHandle, PANEL_TB_SHOWPHASE7,ATTR_ON_TEXT, buff2);
	fread(&dds2table,(sizeof dds2table),1,fdata);
	fread(&dds3table,(sizeof dds3table),1,fdata);

	//Update Period Retrieved and Set
	fread(&updatePer,sizeof updatePer,1,fdata);
	if (updatePer==5)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 1);
		EventPeriod=0.005;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	if (updatePer==10)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 1);
		EventPeriod=0.01;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 0);
	if (updatePer==15)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 1);
		EventPeriod=0.015;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	if (updatePer==25)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 1);
		EventPeriod=0.025;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	if (updatePer==50)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 1);
		EventPeriod=0.05;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	if (updatePer==100)
		{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 1);
		EventPeriod=0.1;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	if (updatePer==1000)
	{
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 1);
		EventPeriod=1;
	}
	else
		SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);

	fread(&DDSFreq.extclock,sizeof DDSFreq.extclock,1,fdata);
	fread(&DDSFreq.PLLmult,sizeof DDSFreq.PLLmult,1,fdata);
	DDSFreq.clock = DDSFreq.extclock * DDSFreq.PLLmult;

	fread(&GPIB_address,sizeof GPIB_address,1,fdata);
	fread(&SRS_amplitude,sizeof SRS_amplitude,1,fdata);
	fread(&SRS_offset,sizeof SRS_offset,1,fdata);
	fread(&SRS_frequency,sizeof SRS_frequency,1,fdata);
	fread(&GPIB_ON,sizeof GPIB_ON,1,fdata);

	fclose(fdata);

	for(i=0;i<=NUMBEROFCOLUMNS;i++)// ramp over # of cells per page
	{
		for(k=0;k<NUMBEROFPAGES;k++)// ramp over pages
		{
			InfoArray[i][k].index = 0;
			InfoArray[i][k].value = 0.0;
			strcpy(InfoArray[i][k].text, "");
		}
	}

	LoadDDSSettings();  //Enters the DDSsettings into the GUI Panel
	SetAnalogChannels();
	SetDigitalChannels();

	// The column labels are saved with the panel. Here, we read their values into the InforArray.
	// This way old panels are imported into the new structure (V16.0.2).
	for (k=1;k<NUMBEROFPAGES;k++)
	{
		for(i=1;i<=NUMBEROFCOLUMNS;i++)
		{
			GetTableCellVal(panelHandle, PANEL_OLD_LABEL[k], MakePoint(i,1), &buff);
			strcpy(InfoArray[i][k].text, buff);
		}
	}
}











/************************************************************************
Laser Data Save and Load
*************************************************************************/

void SaveLaserData(char savedname[500],int nameSize)
/*  Saves all of the relevant laser settings from the GUI panel to a file under the same name with
	.laser file extension
	Parameters: savedname: the full name of the panel save file including the .pan extension
				nameSize: the number of characters in savedname */
{
	FILE *ldata;

	char buff[502]="",buff2[600];
	strncpy(buff,savedname,nameSize-4);
	strcat(buff,".laser");
	if((ldata=fopen(buff,"w"))==NULL)
	{
		strcpy(buff2,"Failed to save laser data arrays. \n Filename: \n");
		strcat(buff2,buff);
		MessagePopup("Save Error",buff2);

	}
	else
	{
		fwrite(LaserProperties,sizeof LaserProperties, 1,ldata);
		fwrite(LaserTable,sizeof LaserTable,1,ldata);
	}

	fclose(ldata);
}

void LoadLaserData(char savedname[500],int nameSize)
{
/*  Loads all Panel attributes and values relevant to the laser control which was saved in a
	.laser file by SaveLaserData

    Note that if the lengths of any of the data arrays are changed previous saves will not be able to be loaded.
    If necessary See AdwinGUI Panel Converter V11-V12 (created June 01, 2006) */


	FILE *ldata;
	char buff[502]="",buff2[600]="";
	char buff3[502];
	strncat(buff,savedname,nameSize-4);
	strcat(buff,".laser");
	if((ldata=fopen(buff,"r"))==NULL)
	{
		strcpy(buff2,"Failed to load laser data arrays (*.laser file). \n Filename: \n");
		strcat(buff2,buff);
		MessagePopup("Save Error",buff2);
	}
	else
	{
		fread(LaserProperties,sizeof LaserProperties, 1,ldata);
		fread(LaserTable,sizeof LaserTable,1,ldata);
	}

	fclose(ldata);
}





/************************************************************************
Helper functions
*************************************************************************/






/************************************************************************
Even older ExportPanel/Config functions
*************************************************************************/

// From GUIDesign.c
// Gets called by the callback EXPORT_PANEL_CALLBACK
void ExportPanel(char fexportname[200],int fnamesize)
{
	FILE *fexport;
	int i=0,j=0,k=0;
	int xval=16,yval=16,zval=10;
	char buff[500],buff2[100],buff3[31],bigbuff[2000];
	char fcnmode[6]=" LEJ";   // step, linear, exponential, const-jerk:  Step is assumed if blank
	double MetaTimeArray[500];
	int MetaDigitalArray[NUMBERDIGITALCHANNELS+1][500];
	struct AnVals MetaAnalogArray[NUMBERANALOGCHANNELS+1][500];
	double DDSfreq1[500], DDSfreq2[500],DDScurrent[500],DDSstop[500];
	int mindex,nozerofound,tsize;
	fcnmode[0]=' ';fcnmode[1]='L';fcnmode[2]='E';fcnmode[3]='J';
	isdimmed=1;

	if((fexport=fopen(fexportname,"w"))==NULL)
	{
		MessagePopup("Save Error","Failed to save configuration file");
	}

	//Lets build the times list first...so we know how long it will be.
	//check each page...find used columns and dim out unused....(with 0 or negative values)
	SetCtrlAttribute(panelHandle,PANEL_ANALOGTABLE,ATTR_TABLE_MODE,VAL_COLUMN);
	mindex=0;
	for(k=1;k<=NUMBEROFPAGES;k++)				//go through for each page
	{
		nozerofound=1;
		if(ischecked[k]==1) //if the page is selected
		{
		//go through for each time column
			for(i=1;i<NUMBEROFCOLUMNS;i++)
			{
				if((nozerofound==1)&&(TimeArray[i][k]>0))
				//ignore all columns after the first time=0
				{
					mindex++; //increase the number of columns counter
					MetaTimeArray[mindex]=TimeArray[i][k];

					//go through for each analog channel
					for(j=1;j<=NUMBERANALOGCHANNELS;j++)
					{
						MetaAnalogArray[j][mindex].fcn=AnalogTable[i][j][k].fcn;
						MetaAnalogArray[j][mindex].fval=AnalogTable[i][j][k].fval;
						MetaAnalogArray[j][mindex].tscale=AnalogTable[i][j][k].tscale;
						MetaDigitalArray[j][mindex]=DigTableValues[i][j][k];
						DDScurrent[mindex]=ddstable[i][k].amplitude;
						DDSfreq1[mindex]=ddstable[i][k].start_frequency;
						DDSfreq2[mindex]=ddstable[i][k].end_frequency;
						DDSstop[mindex]=ddstable[i][k].is_stop;
					}
				}
				else if (TimeArray[i][k]==0)
				{
					nozerofound=0;
				}
			}
		}
	}
	tsize=mindex; //tsize is the number of columns
	// now write to file
	// write header
	sprintf(bigbuff,"Time(ms)");
	for(i=1;i<=tsize;i++)
	{
		strcat(bigbuff,",");
		sprintf(buff,"%f",MetaTimeArray[i]);
		strcat(bigbuff,buff);
	}
	strcat(bigbuff,"\n");
	fprintf(fexport,bigbuff);
	//done header, now write analog lines
	for(j=1;j<=NUMBERANALOGCHANNELS;j++)
	{
		sprintf(bigbuff,AChName[j].chname);
		for(i=1;i<=tsize;i++)
		{
			strcat(bigbuff,",");
			strncat(bigbuff,fcnmode+MetaAnalogArray[j][i].fcn-1,1);
			sprintf(buff,"%3.2f",MetaAnalogArray[j][i].fval);
			strcat(bigbuff,buff);
		}
		strcat(bigbuff,"\n");
		fprintf(fexport,bigbuff);
	}
	//done analog lines, now write DDS
	sprintf(bigbuff,"DDS");
	for(i=1;i<=tsize;i++)
	{
		strcat(bigbuff,",");
		if(DDSstop[i]==TRUE)
		{
			strcat(bigbuff,"OFF");
		}
		else
		{
			strcat(bigbuff,"ma");
			sprintf(buff,"%4.2f",DDScurrent[i]);
			strcat(bigbuff,buff);
			strcat(bigbuff,"_FA");
			sprintf(buff,"%4.2f",DDSfreq1[i]);
			strcat(bigbuff,buff);
			strcat(bigbuff,"_FB");
			sprintf(buff,"%4.2f",DDSfreq2[i]);
			strcat(bigbuff,buff);
		}
	}
	fprintf(fexport,bigbuff);
	//done DDS, now do digital
	for(j=1;j<=NUMBERDIGITALCHANNELS;j++)
	{
		sprintf(bigbuff,DChName[j].chname);
		for(i=1;i<=tsize;i++)
		{
			strcat(bigbuff,",");
			sprintf(buff,"%d",MetaDigitalArray[j][i]);

			strcat(bigbuff,buff);
		}
		strcat(bigbuff,"\n");
		fprintf(fexport,bigbuff);
	}

	fclose(fexport);
}

// The code in this function used to be in CONFIG_EXPORT_CALLBACK but I
// modified that function to call this one to better divide the
// saving and loading code from GUIDesign.c. But not like this function
// will ever be used since I'm pretty sure the menu item has only been
// kept to not break backwards compatibility - Scott 2020-05-07
void ExportConfig(char fconfigname[290],int fnamesize)
{
	FILE *fconfig;
	int i=0,j=0,k=0;
	int xval=16,yval=16,zval=10;
	char buff[500],buff2[190],buff3[31];

	if((fconfig=fopen(fconfigname,"w"))==NULL)
	{
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



