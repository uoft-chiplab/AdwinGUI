#include <formatio.h>
//IDEAS:   
/*		
Add a killswitch...to interupt the ADwin hardware.  Dangerous because it will leave the outputs in 
			whatever state they had at the time the system was 'killed.  Could be fixed by adding a
			bit to the ADBasic code that recognizes the kill switch and sets channels low.

*/
/*
Caveats:
If making any changes to the program, be aware of several inconsistencies.
Adwin software is base 1 for arrays
C (LabWindows) is base 0 for arrays,
however some controls for LabWindows are base 1   : Tables
and some are base 0								  : Listboxes


V16.1.B; April 02, 2015:
		I am going to make the "Changed" button change to purple if there are changes that
			will be taken next cycle. When that cycle starts then the "Changed" button will 
			go back to being the default gray. Done.
		I also changed the label on the checkbox that forces the program to capture and
			rebuild the sequence every time to "Build Every Time". Hopefully it is more
			obvious that whatever the state of the panels, those values will be taken even
			if a set of changes being made by the user is not complete.

V16.1.A; ???
		I believe I added the "Changed" button in this release. Who knows what else I did.

V16.1.9; March 18, 2014:
		Going to attempt to send info to the Rabbits using a parallel thread.
		Again after investigating this is proving difficult. However the lasers are updated
			over tcp. The function BuildLaserUpdate used to wait a full second for the
			commands to finish sending using tcp. I reduced this to 0.3 seconds.
			Non-thourogh testing reveals this delay to not cause a fire. I have no idea
			what the lower limit is for this delay. It is likely just a safety feature
			that I have now cut into. TCP is a handshake protocol so there is likely a
			way to have the laser dds tell us when it has recieved all of the commands.
			!Note that I have reset the tcp send wait time to 1.0 seconds just to be safe.
		The rate limiting step is now the function DrawNewTable() in RunOnce. This function
			redraws everything on the current panel which is quite costly
			(1.0 to 1.3 seconds).

V16.1.8; March 17, 2014:
		Fixed a problem with the force rebuild checkbox. I needed to change an if statement
			in function BuildLaserUpdate to include the force rebuild flag. The problem was
			that there would be errors sending things to the Micromatic. I Didn't figure out
			what those errors were. I just tried changing the if statement that already involved
			the flag ChangedVals.

V16.1.7; March 17, 2014:
		Further modified functions RunOnce and BuildUpdateTable. Added a checkbox on the main
			panel labeled "Force Rebuild?" such that when it is enabled both RunOnce and
			BuildUpdateTable will recompute the tables for the ADwin regardless of whether
			or not any values have changed.

V16.1.6; March 17, 2014:
		Changed functions RunOnce and BuildUpdateTable so that they cache the results from
			the last update and if nothing has changed they send the already computed tables.
		The global variable ChangedVals acts as a flag to indicate whether or not the tables
			need to be updated. To prevent unnecessarily computing the tables the times where
			ChangedVals is set to TRUE should be minimized. I commented out
			(tagged with V16.1.6) several lines where ChangedVals was set to TRUE where it didn't
			need to be set (such as some callback functions like copy and paste). To complement
			this I added a button to the front panel labeled "Changed" which sets ChangedVals
			to TRUE when clicked. These changes only affect running with the repeat button
			depressed. In all other cases the tables are always built. The intended usage is
			for the user to be able to make multiple changes to the sequence while the
			experiment is clicking away and only when the "Changed" button is pressed will
			the tables be recomputed at the start of the next sequence.

V16.1.3; March 5, 2014:
		Set both of the two scan tables on the right hand side to be hidden initially (forever).
		Moved DDS offsets to the right of the analog units table.
			The units for the DDS and PhaseOMatic and Micromatic still don't appear.
		Moved the Cycle Timer thing to above the LED's. (Was nearly off the screen on top right.)
			It doesn't appear to do anything even when enabled. (Maybe it tries to access the ADwin)
		The PhaseOmatic and Micromatic analog labels are still not aligned with the correct table
			rows.

2012:
Oct 7	Added a multi-parameter scan feature, more displayed digital channels (as we were
		running out of them) and the ability to scan digital channels (in multi-parameter 
		scans). Seems that not much documentation was going on between 2012 and 2006 so 
		watch out for unexpected changes, in case you are a returning customer.
		Also started a new file standard for scanfiles.

2006:
Mar 9:  Added the ability to loop between 2 columns of the panel, spanning pages if necessary.  Could be used
        for repetitive outputs.. e.g. cycling between MOT and an optical probe. 
        (need to be careful with DDS programming)  
Mar 9: 	Force DDS clock frequency to be set at run-time.
	    Fixed bug that caused the DDS trap bottom to set to 0 after doing a Scan.
		
Feb 9   Noticed a bug.. probably been there for a VERY long time.  The 'Debug Window' has been slowly accumulating
        text, and not being cleared.  Change code to clear the debug text window before saving the panel


2005
July 29 Added option to output a history of the Scan values when running a scan.
		Adding option to stream the panel to text files;
JULY 20 commands for DDS2 are now generated, also sent to ADwin now (see new ADBasic code  TransferData_Jul20.BAS)	    
        Fixed ADBasic software bug.  Turns out that DIG_WRITELATCH1 and DIG_WRITELATCH2 (for lower and upper 16 bits respectively)
        are incompatible.  Use DIG_WRITELATCH32 instead.
July 18 Added 3rd DDS interface, simplified DDS control by reusing the existing DDS control panel
	    DDS 2 and 3 clock settings displayed in DDSSettings.uir  Not modifiable. Set using a #define in vars.h
	 	Save DDS 2,3 info to file.
July4   Adding 2nd DDS (interface only, creating dual dds command structure still needs to be done.)
June7   Finalized scan programming.  Now scans in amplitude, time or DDS frequency. (Only DDS1 so far)
April 20 Changed the way that the table cells are coloured.  Now all cells are coloured based on 
		the information in the cell.  No longer based on the history of that row.
		Sine wave output now relabels amplitude and frequency on analog control panel. Colours Cyan on table.
April 7	Fixed a bug where we didn't reach the final value on a ramp, but reached the value before.
		Cause: in calculating ramps, i.e determine slope=amplitude/number of steps
			 	but should be amplitude/number_steps-1
Mar 23	Added A Frequency OFfset to the DDS...so the same ramp can be continually used while changing the 
		trap bottom.
		Fixed duration of ramps etc at column duration.
Mar 10	Fixed a problem where the frequency ramps generated by the DDS finished in half the expected time
		Can't find a reason for this, except that the DDS manual might be wrong.
		Added a Sinewave option to the list of possible functions.  Only accepts amplitude and frequency..no bias
		  -  bias could be 'worked around' using bias setting under Analog Channel Setup
Jan 18	Add menu option to turn off the DDS for all cells.  
		Avoids a string of warnings created by the DDS command routines if a DDS command is written before the 
		previous DDS command was done;
Jan 5   Fix bug in code where the timing isn't always copied into the DDS commands
		Fixed the last panel mobile ability
		

//Update History
2004
Apr 15:  Run button flashes with ADwin is operating.
Apr 29:  Fixed Digital line 16.  Loop counted to 15, should have been 16
May 04:  Adding code to delete/insert columns.
May 06:  Added Copy/Paste fcns for column.  Doesn't work 100% yet.... test that channels 16 work for dig. and analog.
May 10:  Fixed a bug where the arrays weren't properly initialized, causing strange values to be written to the adwin
		 Added flashing led's to notify when the ADwin is doing something
May 13:  Fixed 16th line of the panel, now is actually output.  Bug was result of inconsistency with arrays.
		 i.e. Base 0 vs base 1.  
		 - fixed by increasing the internal array size to 17 and starting from 1. (dont read element 0)		 
May 18:  Improved performance.  Added a variable (ChangedVals) that is 0 when no changes have been made since
 		the last run.  If ChangedVals==0, then don't recreate/resend the arrays, shortens delay between runs.
June24:	Add support for more analog output cards.  Change the #define NUMBEROFANALOGCHANNELS xx to reflect the number of channels.
		NOTE:  You need to change this in every .c file. 
		Still need to update the code to use a different channel for digital.  (currently using 17, which will be overwritten
		if using more than 16 analog channels.
July26: Begin adding code to control the DDS (AD9854 from Analog Devices)
	    Use an extra line on the analog table (17 or 25) as the DDS control interface
AUg		Include DDS control
Nov		DDS control re-written by Ian Leroux
Dec7	Add a compression routine for the NumberUpdates variable, to speed up communication with ADwin.
		Added Menu option to turn compression on/off
Dec16	Made the last panel mobile, such that it can be inserted into other pages.
*/

#define ALLOC_GLOBALS  
#include "main.h"
#include "Adwin.h"
#include <time.h>
#define VAR_DECLS 1
#include "vars.h"
#include "LaserSettings2.h"
#include "Processor.h"
#include "GPIB_SRS_SETUP.h"  
//#include <userint.h>
//#include <stdio.h>
//#include  <Windows.h>
 
int main (int argc, char *argv[])
{
	int i,j,k,status;
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
	
	file = OpenFile ("processordefault.txt",VAL_READ_ONLY,VAL_OPEN_AS_IS,VAL_BINARY );
	ReadFile (file,procbuff, 1);
	processorT1x = *procbuff-48;
	SetCtrlVal (panelHandle12, PANEL_PROCESSORSWITCH, processorT1x);
	GetCtrlVal (panelHandle12, PANEL_PROCESSORSWITCH, &processorT1x); 	
	CloseFile (file); 	
	
		
//	SetCtrlAttribute (panelHandle, PANEL_DEBUG, ATTR_VISIBLE, 0);
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
	
	// initialize GPIB device array
	for (i=0;i<NUMBERGPIBDEV;i++)
	{
		GPIBDev[i].address = 0;
		strcpy(GPIBDev[i].devname, "noname");
		strcpy(GPIBDev[i].cmdmask, "");
		strcpy(GPIBDev[i].command, "");
		strcpy(GPIBDev[i].lastsent, "");
		GPIBDev[i].active = FALSE;
		for (j=0;j<NUMGPIBPROGVALS;j++)
		{
			GPIBDev[i].value[j] = 0.0;
		}
		
	}
	

	//initialize dds_tables, don't assume anything...
	for (i=0;i<NUMBEROFCOLUMNS;i++)
	{
		for (j=0;j<NUMBEROFPAGES;j++)
		{
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
	// done initializing
	
	EventPeriod=DefaultEventPeriod;
	//ClearListCtrl(panelHandle,PANEL_DEBUG);

	//LoadLastSettings(1); //This feature is not fully implemented
	
	//Sets the First Page as Active
	SetCtrlVal (panelHandle,PANEL_TB_SHOWPHASE[1],1);
	for (i=2;i<=(NUMBEROFPAGES-1);i++) // somebody set NUMBEROFPAGES to 11 -- to be quick and dirty -- no idea what that's supposed to be ... funny?
	{
		SetCtrlVal (panelHandle,PANEL_TB_SHOWPHASE[i],0);
	}
	
	Initialization();  
	
	DisplayPanel (panelHandle);
	
	RunUserInterface ();  // start the GUI
	
	DiscardPanel (panelHandle);  // returns here after the GUI shutsdown
	
	return 0;
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
	


//new in this version 1/21/2019

	// Force number of GPIB devices to obey NUMBERGPIBDEV (the number of devices in vars.h)
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_DEVICENO, ATTR_MIN_VALUE, 1);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_DEVICENO, ATTR_MAX_VALUE, NUMBERGPIBDEV);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_LASTNO, ATTR_MIN_VALUE, 1);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_LASTNO, ATTR_MAX_VALUE, NUMBERGPIBDEV);
	// Allow for other VISA devices besides GPIB by allowing GPIB ADDRESS to be larger than 30. 
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_ADDRESS, ATTR_MIN_VALUE, 0);
	SetCtrlAttribute(panelHandle13, SETUP_GPIB_ADDRESS, ATTR_MAX_VALUE, MAXVISAADDR);   //we are missing panel handle
	
 //end of new stuff

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
	//DrawCanvasArrows();
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
//***************************************************************************************************
void ConvertIntToStr(int int_val, char *int_str)
{

	int i,j;
	
	for (i=j=floor(log10(int_val));i>=0;i--)
	{
		int_str[i] = (char) (((int) '0') + floor(((int) floor((int_val/pow(10,(j-i))))%10)));
	}
	
	int_str[j+1] = '\0';
	
	return;
}

//***************************************************************************************************
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

//***************************************************************************************************
void DrawCanvasArrows(void)
{
 int loopx=0,loopx0=0;
// draws an arrow in each of 2 canvas's on the GUI.  These canvas's are moved around to indicate the location
// of a loop in the Adwin output.
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_WIDTH,1);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_COLOR,VAL_DK_GREEN);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PICT_BGCOLOR,VAL_TRANSPARENT);

CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(0,0), MakePoint(15,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(15,0), MakePoint(8,12));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,12), MakePoint(0,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,11), MakePoint(1,1));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,10), MakePoint(2,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(0,0), MakePoint(8,4));
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_WIDTH,4);
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(4,2), MakePoint(11,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(11,2), MakePoint(8,8));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,8), MakePoint(4,2));

SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_WIDTH,1);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_COLOR,VAL_DK_RED);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PICT_BGCOLOR,VAL_TRANSPARENT);
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(0,0), MakePoint(8,4));

CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(0,0), MakePoint(15,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(15,0), MakePoint(8,12));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,12), MakePoint(0,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,11), MakePoint(1,1));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,10), MakePoint(2,2));
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_WIDTH,4);
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(4,2), MakePoint(11,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(11,2), MakePoint(8,8));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,8), MakePoint(4,2));

loopx0=170;		//horizontal offset
loopx=8;		//x position...in horizontal table units
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_LEFT,loopx0+loopx*40);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_TOP,141);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_LEFT,loopx0+loopx*40+25);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_TOP,141);

SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_LEFT,168);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_TOP,140);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_WIDTH,685);
}
