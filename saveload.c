// Saving and Loading functions for panels go here.
// Any binary dumps for debugging also go here (arrays sent to the adwin for example)

// For now, do not put (Multi)Scan export functions here. They will go in the multiscan.c file eventually.

// TODO:
// There is fn CONFIG_EXPORT_CALLBACK which does similar things to ExportPanel but is all in the callback fn.


#include "saveload.h"

#include <ansi_c.h>
#include <userint.h>

#include "vars.h"
#include "GUIDesign.h"// For menubar definitions
#include "GUIDesign2.h"// For DrawNewTable fn
#include "LaserSettings2.h"// For LaserSettingsInit fn
#include "AnritsuSettings2.h"//For LoadAnritsuSettings fn
#include "AnalogSettings2.h"//For SetAnalogChannels fn
#include "DigitalSettings2.h"//For SetDigitalChannels fn



/************************************************************************
Overall Save and Load that chooses correct version
*************************************************************************/

// From GUIDesign.c
//
void SaveSettings(int version)
/* Modified:
Feb 09, 2006   Clear the Debug box before saving. (was causing insanely large save files, and slowed down loading of old panels.)
*/
{
	// Save settings:  First look for file .ini  This will be a simple 1 line file staating the name of the last file
	//saved.  Load this up and use as the starting name in the file dialog.
	FILE *fpini;
	char fname[100]="",c,fsavename[500]="",buff[600];
	static char defaultdir[200]="C:\\UserDate\\Data";
	int i=0,j=0,k=0,inisize=0,status,loadonboot=0;
	//Check if .ini file exists.  Load it if it does.
	if(!(fpini=fopen("gui.ini","r"))==NULL)		// if "gui.ini" exists, then read it  Just contains a filename.
	{												//If not, do nothing
		while (fscanf(fpini,"%c",&c) != -1) fname[inisize++] = c;  //Read the contained name
	}
 	fclose(fpini);

	status=FileSelectPopup (defaultdir, "*.pan", "", "Save Settings", VAL_SAVE_BUTTON, 0, 0, 1, 1,fsavename);
	GetMenuBarAttribute (menuHandle, MENU_FILE_BOOTLOAD, ATTR_CHECKED, &loadonboot);
	if(!(status==0))
	{
	//	ClearListCtrl (panelHandle, PANEL_DEBUG);  // added Feb 09, 2006
		SavePanelState(PANEL, fsavename, 1);  // This one can be problematic when elements have been removed from the GUI!!!
		if(!(fpini=fopen("gui.ini","w"))==NULL)
		{
			fprintf(fpini,fsavename);
			fprintf(fpini,"\n%d",loadonboot);
		}
		fclose(fpini);

		// Stefan Trotzky - Oct 2012: added switch for file version
		switch (version)
		{
			case 15:
				SaveArraysV15(fsavename, strlen(fsavename));
				SaveLaserData(fsavename,strlen(fsavename));
				break;
			case 16: // V16 saves laser data together with arrays
				SaveArraysV16(fsavename, strlen(fsavename));
				break;
			default:
				SaveArraysV15(fsavename, strlen(fsavename));
				SaveLaserData(fsavename,strlen(fsavename));
				break;
		}


		i=499;
		while(i>=0&&fsavename[i]!='\\')
			i--;

		strcpy(buff,SEQUENCER_VERSION);
		strcat(buff,&fsavename[i+1]);
		SetPanelAttribute (panelHandle, ATTR_TITLE, buff);
	}
	else
	{
		MessagePopup ("File Error", "No file was selected");
	}
	strcpy(defaultdir,"");

}

// From GUIDesign.c
//
void LoadSettings(int version)
{
	int status;
	//If .ini exists, then open the file dialog.  Else just open the file dialog.  Add a method to load
	//last used settings on program startup.
	FILE *fpini;
	char fname[100]="",c,fsavename[500]="",buff[600];
	static char defaultdir[200]="C:\\UserDate\\Data";
	int i=0,j=0,k=0,inisize=0,success;

	//Check if .ini file exists.  Load it if it does.
	if(!(fpini=fopen("gui_V16.ini","r"))==NULL)
	{
		while (fscanf(fpini,"%c",&c) != -1) fname[inisize++] = c;  //Read the contained name
	}
	fclose(fpini);

	// prompt for a file, if selected then load the Panel and Arrays
	status=FileSelectPopup (defaultdir, "*.pan", "", "Load Settings", VAL_LOAD_BUTTON, 0, 0, 1, 1,fsavename );
	if(!(status==0))
	{
		RecallPanelState(PANEL, fsavename, 1); // This one can be problematic when elements have been removed from the GUI!!!

		// Stefan Trotzky - Oct 2012: added switch for file version
		switch (version)
		{
			case 13:
				LoadArraysV13(fsavename,strlen(fsavename));
				LoadLaserData(fsavename,strlen(fsavename));
				success = 1; // no success evaluation before V16
				break;
			case 15:
				LoadArraysV15(fsavename,strlen(fsavename));
				LoadLaserData(fsavename,strlen(fsavename));
				success = 1; // no success evaluation before V16
				break;
			case 16: // V16 saves laser data together with arrays
				success = LoadArraysV16(fsavename,strlen(fsavename));
				break;
		}

		if (success)
		{
			LaserSettingsInit();

			//edit panel title
			i=499;
			while(i>=0&&fsavename[i]!='\\')
				i--;

			strcpy(buff,SEQUENCER_VERSION);
			strcat(buff,&fsavename[i+1]);
			SetPanelAttribute (panelHandle, ATTR_TITLE, buff);
		}

	}
	else
	{
		MessagePopup ("File Error", "No file was selected");
	}

	DrawNewTable(0);
	strcpy(defaultdir,"");

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
	SaveArraysV16(fname, strlen(fname));

}

// From GUIDesign.c
//
void LoadLastSettings(int check)
{
	//If .ini exists, then open the file dialog.  Else just open the file dialog.  Add a method to load
	//last used settings on program startup.
	FILE *fpini;
	char fname[100]="",c,fsavename[500]="",loadname[100]="",buff[600];
	int i=0,j=0,k=0,inisize=0, success;
	//Check if .ini file exists.  Load it if it does.
	if(!(fpini=fopen("gui.ini","r"))==NULL)
	{
		while (fscanf(fpini,"%c",&c) != -1) fname[inisize++] = c;  //Read the contained name
	}
	fclose(fpini);

	c=fname[inisize-1];
	strncpy(loadname,fname,inisize-2);
	if((check==0)||(c==49))			  // 49 is the ascii code for "1"
	{
		RecallPanelState(PANEL, loadname, 1); // This one can be problematic when elements have been removed from the GUI!!!
		success = LoadArraysV16(fname,strlen( loadname));

		if (success)
		{
			LaserSettingsInit();

			i=500;
			while(i>=0&&fsavename[i]!='\\')
				i--;
			strcpy(buff,SEQUENCER_VERSION);
			strcat(buff,&fsavename[i+1]);
			SetPanelAttribute (panelHandle, ATTR_TITLE, buff);
		}

	}
	DrawNewTable(0);
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
	int i=0,j=0,k=0;
	int xval=NUMBEROFCOLUMNS,yval=NUMBERANALOGROWS,zval=NUMBEROFPAGES-1;
	int usupd5,usupd10,usupd15,usupd25,usupd50,usupd100,usupd1000,updatePer; //Update Period Check
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
	int i=0, j=0, k=0;
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
Even older ExportPanel functions
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





