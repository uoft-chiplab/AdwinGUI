/*
The file that starts everything off.
*/


#define ALLOC_GLOBALS
#define VAR_DECLS 1

#include <formatio.h>

#include "main.h"
#include "Adwin.h"
#include <time.h>
#include "vars.h"
#include "LaserSettings2.h"
#include "Processor.h"
#include "GPIB_SRS_SETUP.h"
//#include <userint.h>
//#include <stdio.h>
//#include  <Windows.h>

#include "multiscan.h"

#include "saveload.h"// for testing saveload functionality only

int main (int argc, char *argv[])
{
	int i,j,k,status;
	int fileHandle;

	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "GUIDesign.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle_sub1=LoadPanel(0,"GUIDesign.uir",SUBPANEL1))<0)
		return -1;
	if ((panelHandle_sub2=LoadPanel(0,"GUIDesign.uir",SUBPANEL2))<0)
		return -1;
	if ((panelHandle2 = LoadPanel (0, "AnalogSettings.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle3 = LoadPanel (0, "DigitalSettings.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle4 = LoadPanel (0, "AnalogControl.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle5 = LoadPanel (0, "DDSSettings.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle6 = LoadPanel (0, "DDSControl.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle7 = LoadPanel (0, "Scan.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle8 = LoadPanel (0, "ScanTableLoader.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle9 = LoadPanel (0, "NumSet.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle10 = LoadPanel (0, "LaserSettings.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle11 = LoadPanel (0, "LaserControl.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle12 = LoadPanel (0, "Processor.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle13 = LoadPanel (0, "GPIB_SRS_SETUP.uir", SETUP)) < 0)
		return -1;
	if ((panelHandleANRITSUSETTINGS = LoadPanel (0, "AnritsuSettings.uir",PANEL)) < 0)
		return -1;
	if ((panelHandleANRITSU = LoadPanel (0, "AnritsuControl.uir",PANEL)) < 0)
		return -1;


	panelHandle0 = panelHandle; // since panelhandle is usually used as a parameter in functions

	fileHandle = OpenFile ("processordefault.txt",VAL_READ_ONLY,VAL_OPEN_AS_IS,VAL_BINARY );
	ReadFile (fileHandle,procbuff, 1);
	processorT1x = *procbuff-48;
	SetCtrlVal (panelHandle12, PANEL_PROCESSORSWITCH, processorT1x);
	GetCtrlVal (panelHandle12, PANEL_PROCESSORSWITCH, &processorT1x);
	CloseFile (fileHandle);


	// Initialize arrays (to avoid undefined elements causing -99 to be written)
	for (j=0;j<=NUMBERANALOGCHANNELS;j++)
	{		 //ramp over # of analog chanels
		AChName[j].tfcn=1;
		AChName[j].tbias=0;
		AChName[j].resettozero=1;
		for(i=0;i<=NUMBEROFCOLUMNS;i++)// ramp over # of cells per page
		{
			for(k=0;k<NUMBEROFPAGES;k++)// ramp over pages
			{
				AnalogTable[i][j][k].fcn=1;
				AnalogTable[i][j][k].fval=0.0;
				AnalogTable[i][j][k].tscale=1;

			}
		}
	}


	for (j=0;j<=NUMBERDIGITALCHANNELS;j++)
	{
		for(i=0;i<=NUMBEROFCOLUMNS;i++)// ramp over # of cells per page
		{
			for(k=0;k<NUMBEROFPAGES;k++)// ramp over pages
				DigTableValues[i][j][k]=0;
		}
	}

	for(i=0;i<=NUMBEROFCOLUMNS;i++)// ramp over # of cells per page
	{
		for(k=0;k<NUMBEROFPAGES;k++)// ramp over pages
		{
			InfoArray[i][k].index = 0;
			InfoArray[i][k].value = 0.0;
			strcpy(InfoArray[i][k].text, "");
		}
	}



	/* Initialize laser arrays - everything set to hold last value (default) 1st set to step */
	LaserTable[0][1][1].fcn=1;
	LaserTable[1][1][1].fcn=1;
	LaserTable[2][1][1].fcn=1;
	LaserTable[3][1][1].fcn=1;


	initializeGpibDevArray();

	initializeDdsTables();

	// done initializing

	EventPeriod=DefaultEventPeriod;

	//Sets the First Page as Active
	SetCtrlVal (panelHandle,PANEL_TB_SHOWPHASE[1],1);
	for (i=2;i<=(NUMBEROFPAGES-1);i++) // somebody set NUMBEROFPAGES to 11 -- to be quick and dirty -- no idea what that's supposed to be ... funny?
	{
		SetCtrlVal (panelHandle,PANEL_TB_SHOWPHASE[i],0);
	}

	Initialization();


	// Scott Testing V17 saving and loading
	SaveSequenceV17("C:\\Users\\coldatoms\\Documents\\Lab\\LocalCode\\ADwin_sequencers\\test.woot",
			 strlen("C:\\Users\\coldatoms\\Documents\\Lab\\LocalCode\\ADwin_sequencers\\test.woot"));

	LoadSequenceV17("C:\\Users\\coldatoms\\Documents\\Lab\\LocalCode\\ADwin_sequencers\\test.woot",
			 strlen("C:\\Users\\coldatoms\\Documents\\Lab\\LocalCode\\ADwin_sequencers\\test.woot"));



	DisplayPanel (panelHandle);

	RunUserInterface ();  // start the GUI

	DiscardPanel (panelHandle);  // returns here after the GUI shutsdown

	return 0;
}


void initializeDdsTables(void){

	int i,j;

	//initialize dds_tables, don't assume anything...
	for( i=0; i < NUMBEROFCOLUMNS; ++i ){
		for( j=0; j < NUMBEROFPAGES; ++j ){

			ddstable[i][j].start_frequency = 0.0;
			ddstable[i][j].end_frequency = 0.0;
			ddstable[i][j].amplitude = 0.0;
			ddstable[i][j].delta_time = 0.0;
			ddstable[i][j].is_stop = TRUE;

			dds2table[i][j].start_frequency = 0.0;
			dds2table[i][j].end_frequency = 0.0;
			dds2table[i][j].amplitude = 0.0;
			dds2table[i][j].delta_time = 0.0;
			dds2table[i][j].is_stop = TRUE;

			dds3table[i][j].start_frequency = 0.0;
			dds3table[i][j].end_frequency = 0.0;
			dds3table[i][j].amplitude = 0.0;
			dds3table[i][j].delta_time = 0.0;
			dds3table[i][j].is_stop = TRUE;
		}
	}
}


void initializeGpibDevArray(void){

	int i,j;

	// initialize GPIB device array
	for( i=0; i < NUMBERGPIBDEV; ++i){
		GPIBDev[i].address = 0;
		strcpy(GPIBDev[i].devname, "noname");
		strcpy(GPIBDev[i].cmdmask, "");
		strcpy(GPIBDev[i].command, "");
		strcpy(GPIBDev[i].lastsent, "");
		GPIBDev[i].active = FALSE;
		for( j=0; j < NUMGPIBPROGVALS; ++j ){
			GPIBDev[i].value[j] = 0.0;
		}
	}
}












//**********************************************************************************
void Initialization()
{
	//Changes:
	//Mar09, 2006:  Force DDS 1 frequency settings at loadtime.
	int i=0,cellheight=0,fontsize=0,aname_size,new_aname_size,ledleft,ledtop;
	int j=0,x0,dx;
	char str_list_val[5];
	char buff[200];

	PScan.Scan_Active=FALSE;
	PScan.Use_Scan_List=FALSE;
	MultiScan.Active = FALSE;
	TwoParam=FALSE;



	DDSFreq.extclock=15.36;
	DDSFreq.PLLmult=8;
	DDSFreq.clock=DDSFreq.extclock*(double)DDSFreq.PLLmult;

	//Add in any extra rows (if the number of channels increases)
	//July4, added another row for DDS2

	// Menu setting
	menuHandle=GetPanelMenuBar(panelHandle);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD5, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD10, ATTR_CHECKED, 1);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD15, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD25, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD50, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD100, ATTR_CHECKED, 0);
	SetMenuBarAttribute (menuHandle, MENU_UPDATEPERIOD_SETGD1000, ATTR_CHECKED, 0);
	currentpage=1;

	//Build display panels (text/channel num)
	SetTableColumnAttribute (panelHandle, PANEL_TBL_ANAMES,1,ATTR_CELL_TYPE, VAL_CELL_STRING);
	SetTableColumnAttribute (panelHandle, PANEL_TBL_ANAMES,2,ATTR_CELL_TYPE, VAL_CELL_NUMERIC);
	SetTableColumnAttribute (panelHandle, PANEL_TBL_DIGNAMES,1,ATTR_CELL_TYPE, VAL_CELL_STRING);
	SetTableColumnAttribute (panelHandle, PANEL_TBL_DIGNAMES,2,ATTR_CELL_TYPE, VAL_CELL_NUMERIC);
	SetCtrlAttribute (panelHandle, PANEL_TBL_ANAMES, ATTR_TABLE_MODE,VAL_COLUMN);
	SetCtrlAttribute (panelHandle, PANEL_TBL_DIGNAMES, ATTR_TABLE_MODE,VAL_COLUMN);

	// Build Digital Table
	NewCtrlMenuItem (panelHandle,PANEL_DIGTABLE,"Copy",-1,Dig_Cell_Copy,0); //Adds Popup Menu Item "Copy"
	NewCtrlMenuItem (panelHandle,PANEL_DIGTABLE,"Paste",-1,Dig_Cell_Paste,0);
	MNU_DIGTABLE_SCANCELL = NewCtrlMenuItem (panelHandle,PANEL_DIGTABLE,"Scan cell value",-1,MultiScan_AddCellToScan,0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_DIGTABLE,VAL_SORT); //Hides Sort Command

	InsertTableRows(panelHandle,PANEL_DIGTABLE,-1,NUMBERDIGITALCHANNELS-1,VAL_CELL_PICTURE);
	InsertTableRows(panelHandle,PANEL_TBL_DIGNAMES,-1,NUMBERDIGITALCHANNELS-1,VAL_CELL_NUMERIC);

	for (i=1;i<=NUMBERDIGITALCHANNELS;i++)
	{
		SetTableCellAttribute (panelHandle, PANEL_TBL_DIGNAMES, MakePoint(1,i),ATTR_CELL_TYPE, VAL_CELL_STRING);
		SetTableCellAttribute (panelHandle, PANEL_TBL_DIGNAMES, MakePoint(2,i),ATTR_DATA_TYPE, VAL_UNSIGNED_INTEGER);
		SetTableCellVal (panelHandle, PANEL_TBL_DIGNAMES, MakePoint(2,i), i);
	}

	//disable interactive row and column sizing on analog table,digtable and timetable
	SetCtrlAttribute(panelHandle,PANEL_ANALOGTABLE,ATTR_ENABLE_ROW_SIZING,0);
	SetCtrlAttribute(panelHandle,PANEL_ANALOGTABLE,ATTR_ENABLE_COLUMN_SIZING,0);
	SetCtrlAttribute(panelHandle,PANEL_DIGTABLE,ATTR_ENABLE_ROW_SIZING,0);
	SetCtrlAttribute(panelHandle,PANEL_DIGTABLE,ATTR_ENABLE_COLUMN_SIZING,0);
	SetCtrlAttribute(panelHandle,PANEL_TIMETABLE,ATTR_ENABLE_ROW_SIZING,0);
	SetCtrlAttribute(panelHandle,PANEL_TIMETABLE,ATTR_ENABLE_COLUMN_SIZING,0);

	// Build Analog Table //
	//Add menu items for analog channel panel
	NewCtrlMenuItem (panelHandle,PANEL_ANALOGTABLE,"Copy",-1,Analog_Cell_Copy,0);
	NewCtrlMenuItem (panelHandle,PANEL_ANALOGTABLE,"Paste",-1,Analog_Cell_Paste,0);
	MNU_ANALOGTABLE_SCANCELL = NewCtrlMenuItem (panelHandle,PANEL_ANALOGTABLE,"Scan cell value",-1,MultiScan_AddCellToScan,0);
	MNU_ANALOGTABLE_SCANCELL_TIMESCALE = NewCtrlMenuItem( panelHandle, PANEL_ANALOGTABLE,
					"Scan cell timescale", -1, MultiScan_AddAnalogCellTimeScaleToScan, 0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_ANALOGTABLE,VAL_SORT); //Hides Sort Command

	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_TABLE_MODE,VAL_GRID);
	SetTableColumnAttribute (panelHandle, PANEL_ANALOGTABLE, -1,ATTR_PRECISION, 3);

	InsertTableRows(panelHandle,PANEL_ANALOGTABLE,-1,NUMBERANALOGROWS-1,VAL_CELL_NUMERIC);
	InsertTableRows(panelHandle,PANEL_TBL_ANAMES,-1,NUMBERANALOGROWS-1,VAL_USE_MASTER_CELL_TYPE);
	InsertTableRows(panelHandle,PANEL_TBL_ANALOGUNITS,-1,NUMBERANALOGCHANNELS-1,-1);
	SetAnalogChannels();

	//Set default analog channel names
	for (i=1;i<=NUMBERANALOGCHANNELS;i++)
	{
		SetTableCellAttribute (panelHandle, PANEL_TBL_ANAMES, MakePoint(2,i),ATTR_DATA_TYPE, VAL_UNSIGNED_INTEGER);
		SetTableCellVal (panelHandle, PANEL_TBL_ANAMES, MakePoint(2,i), i);
	}

	//Add menu items for time table
	MNU_TIMETABLE_SCANCELL = NewCtrlMenuItem (panelHandle,PANEL_TIMETABLE,"Scan cell value",-1,MultiScan_AddCellToScan,0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_TIMETABLE,VAL_SORT); //Hides Sort Command

	//Add menu items for GPIB setup values table
	MNU_TIMETABLE_SCANCELL = NewCtrlMenuItem (panelHandle13,SETUP_GPIB_TAB_VALS,"Scan cell value",-1,MultiScan_AddGPIBCellToScan,0);
	HideBuiltInCtrlMenuItem (panelHandle13,SETUP_GPIB_TAB_VALS,VAL_SORT); //Hides Sort Command

	//Scan Table Right Click Menu Additions
	NewCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE,"Load Values",-1,Scan_Table_Load,0);
	NewCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE,"Set Number of Cells",-1,Scan_Table_NumSet_Load,0);
	NewCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE,"Shuffle Entries",-1,Scan_Table_Shuffle,0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE,VAL_SORT);

	//Scan Table 2 Right Click Menu Additions
	NewCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE_2,"Load Values",-1,Scan_Table_Load,0);
	NewCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE_2,"Shuffle Entries",-1,Scan_Table_Shuffle,0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_SCAN_TABLE_2,VAL_SORT);

	//MultiScan Table Right Click Menu Additions
	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_POS_TABLE,"Set Values",-1,MultiScan_Table_SetVals,0);
	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_POS_TABLE,"Remove Column",-1,MultiScan_Table_DeleteColumn,0);
	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_POS_TABLE,"Shuffle Scanlist",-1,MultiScan_Table_Shuffle,0);
	HideBuiltInCtrlMenuItem (panelHandle,PANEL_MULTISCAN_POS_TABLE,VAL_SORT);

	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_VAL_TABLE,"Set Values",-1,MultiScan_Table_SetVals,0);
	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_VAL_TABLE,"Remove Column",-1,MultiScan_Table_DeleteColumn,0);
	NewCtrlMenuItem (panelHandle,PANEL_MULTISCAN_VAL_TABLE,"Shuffle Scanlist",-1,MultiScan_Table_Shuffle,0);

	SetTableRowAttribute (panelHandle, PANEL_ANALOGTABLE, -1,ATTR_PRECISION, 3);


	// Change Analog Settings window sets maximums on channel number and channel line
	SetCtrlAttribute (panelHandle2, ANLGPANEL_NUM_ACH_LINE,ATTR_MAX_VALUE, NUMBERANALOGCHANNELS);
	SetCtrlAttribute (panelHandle2, ANLGPANEL_NUM_ACHANNEL,ATTR_MAX_VALUE, NUMBERANALOGCHANNELS);

	// change GUI
	SetCtrlAttribute (panelHandle, PANEL_ANALOGTABLE, ATTR_NUM_VISIBLE_ROWS, NUMBERANALOGROWS);
	SetCtrlAttribute (panelHandle, PANEL_DIGTABLE, ATTR_NUM_VISIBLE_ROWS, NUMBERDIGITALCHANNELS);



	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_LEFT, 165);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_TOP, 88);
	SetCtrlAttribute (panelHandle, PANEL_INFOTABLE, ATTR_VISIBLE, 1);


	// Being ugly once helps at later points!
	// TO BE DONE: generate toggle buttons & checkboxes at this place and remove the peranent ones from the
	// GUI panel -- needs updating whenever called.
	PANEL_TB_SHOWPHASE[1] = PANEL_TB_SHOWPHASE1; // index 0 not assigned
	PANEL_TB_SHOWPHASE[2] = PANEL_TB_SHOWPHASE2;
	PANEL_TB_SHOWPHASE[3] = PANEL_TB_SHOWPHASE3;
	PANEL_TB_SHOWPHASE[4] = PANEL_TB_SHOWPHASE4;
	PANEL_TB_SHOWPHASE[5] = PANEL_TB_SHOWPHASE5;
	PANEL_TB_SHOWPHASE[6] = PANEL_TB_SHOWPHASE6;
	PANEL_TB_SHOWPHASE[7] = PANEL_TB_SHOWPHASE7;
	PANEL_TB_SHOWPHASE[8] = PANEL_TB_SHOWPHASE8;
	PANEL_TB_SHOWPHASE[9] = PANEL_TB_SHOWPHASE9;
	PANEL_TB_SHOWPHASE[10] = PANEL_TB_SHOWPHASE10;

	PANEL_CHKBOX[1] = PANEL_CHECKBOX;
	PANEL_CHKBOX[2] = PANEL_CHECKBOX_2;
	PANEL_CHKBOX[3] = PANEL_CHECKBOX_3;
	PANEL_CHKBOX[4] = PANEL_CHECKBOX_4;
	PANEL_CHKBOX[5] = PANEL_CHECKBOX_5;
	PANEL_CHKBOX[6] = PANEL_CHECKBOX_6;
	PANEL_CHKBOX[7] = PANEL_CHECKBOX_7;
	PANEL_CHKBOX[8] = PANEL_CHECKBOX_8;
	PANEL_CHKBOX[9] = PANEL_CHECKBOX_9;
	PANEL_CHKBOX[10] = PANEL_CHECKBOX_10;

	PANEL_OLD_LABEL[10] = PANEL_LABEL_1;
	PANEL_OLD_LABEL[1] = PANEL_LABEL_2;
	PANEL_OLD_LABEL[3] = PANEL_LABEL_3;
	PANEL_OLD_LABEL[4] = PANEL_LABEL_4;
	PANEL_OLD_LABEL[5] = PANEL_LABEL_5;
	PANEL_OLD_LABEL[9] = PANEL_LABEL_6;
	PANEL_OLD_LABEL[8] = PANEL_LABEL_7;
	PANEL_OLD_LABEL[7] = PANEL_LABEL_8;
	PANEL_OLD_LABEL[6] = PANEL_LABEL_9;
	PANEL_OLD_LABEL[2] = PANEL_LABEL_10;
	// Enough ugly!

	// Reposition the page boxes and checkboxes
	x0=100; dx=65;
	for (i=1;i<=(NUMBEROFPAGES-1);i++) // somebody set NUMBEROFPAGES to 11 -- to be quick and dirty -- no idea what that's supposed to be ... funny?
	{
		SetCtrlAttribute (panelHandle,PANEL_TB_SHOWPHASE[i],ATTR_TOP,30);
		SetCtrlAttribute (panelHandle,PANEL_TB_SHOWPHASE[i],ATTR_LEFT,x0 + i*dx);
		SetCtrlAttribute (panelHandle,PANEL_CHKBOX[i],ATTR_TOP,60);
		SetCtrlAttribute (panelHandle,PANEL_CHKBOX[i],ATTR_LEFT,x0 + i*dx);
	}


	// Force number of GPIB devices to obey NUMBERGPIBDEV (the number of devices in vars.h)
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_DEVICENO, ATTR_MIN_VALUE, 1);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_DEVICENO, ATTR_MAX_VALUE, NUMBERGPIBDEV);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_LASTNO, ATTR_MIN_VALUE, 1);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_LASTNO, ATTR_MAX_VALUE, NUMBERGPIBDEV);
	// Allow for other VISA devices besides GPIB by allowing GPIB ADDRESS to be larger than 30.
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_ADDRESS, ATTR_MIN_VALUE, 0);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_ADDRESS, ATTR_MAX_VALUE, MAXVISAADDR);   //we are missing panel handle


	// A traffic light to indicate certain things for debugging
	GetCtrlAttribute (panelHandle,PANEL_LED_RED,ATTR_LEFT,&ledleft);
	GetCtrlAttribute (panelHandle,PANEL_LED_RED,ATTR_TOP,&ledtop);
	SetCtrlAttribute (panelHandle,PANEL_LED_YEL,ATTR_LEFT,ledleft);
	SetCtrlAttribute (panelHandle,PANEL_LED_YEL,ATTR_TOP,ledtop+25);
	SetCtrlAttribute (panelHandle,PANEL_LED_GRE,ATTR_LEFT,ledleft);
	SetCtrlAttribute (panelHandle,PANEL_LED_GRE,ATTR_TOP,ledtop+50);


	// Scan display
	SetCtrlAttribute (panelHandle_sub2,SUBPANEL2,ATTR_VISIBLE,0);

	SetTableColumnAttribute (panelHandle, PANEL_TBL_ANAMES, 2,ATTR_DATA_TYPE, VAL_UNSIGNED_INTEGER);
	SetTableColumnAttribute (panelHandle, PANEL_TBL_DIGNAMES, 2,ATTR_DATA_TYPE, VAL_UNSIGNED_INTEGER);

	parameterscanmode = 0;
	PScan.Analog.Scan_Step_Size=1.0;
	PScan.Analog.Iterations_Per_Step=1;
	PScan.Scan_Active=FALSE;

	// set to display both analog and digital channels also changes a bunch of their shape/position properties
	// Note the Scan table is not fixed in the source code, to change its position move it in the GUIDesign.uir file
	SetChannelDisplayed(1);
//
	//set to graphical display
//	SetDisplayType(VAL_CELL_NUMERIC);
//	DrawNewTable(0);


	//Initialize Settings Tables (with zeroes) and sets Laser Digital comm channels
	LaserSettingsInit();

	//Display Sequencer Version Number on Main Panel Title
	strcpy (buff,SEQUENCER_VERSION);
	strcat (buff,"untitled panel");
	SetPanelAttribute (panelHandle, ATTR_TITLE, buff);
	CycleTime=-1.0;


	// Initialize debug tracker for number of GPIB writes since program start.
	DEBUG_NUMBER_IBWRT_CALLS = 0;


	return;

}




