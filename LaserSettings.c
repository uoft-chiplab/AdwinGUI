/*  LaserSettings.c
	Author: David Burns, July 2006
	Jan 2011: Added NUMBEROFANRITSU to various table creation parameters

	*/




#include <ansi_c.h>
#include <cvirte.h>
#include <userint.h>
#include "LaserSettings.h"
#include "LaserSettings2.h"
#include "vars.h"
#include "GUIDesign.h"
#include "DigitalSettings2.h"


/*************************************************************************************************************************/
int CVICALLBACK LaserSelect (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
/*  Callback Function for when a given laser is selected from the pulldown menu.
	Loads Laser Settings into GUI table */
{
	int laserNum;

	switch (event)  {
		case EVENT_COMMIT:
			GetCtrlVal (panel, control, &laserNum);
			FillLaserTable(laserNum);
			break;
	}
	return 0;
}


/*************************************************************************************************************************/
int CVICALLBACK ExitLaserSettings (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel (panelHandle10);
			break;
		}
	return 0;
}


/*************************************************************************************************************************/
int CVICALLBACK LaserSettingsTable (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
/* Callback function for the Settings Table. Updates changes to the corresponding virtual parameters */
{
	int laserNum;

	switch (event)  {
		case EVENT_VAL_CHANGED:
			GetCtrlVal (panel,PANEL_LASER_RING, &laserNum);
			switch(eventData1)  {
				case 1:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,1),LaserProperties[laserNum].Name);
					SetLaserLabels();
					break;
				case 2:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,2),LaserProperties[laserNum].IP);
					break;
				case 3:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,3),&LaserProperties[laserNum].Port);
					break;
				case 4:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,4),&LaserProperties[laserNum].DigitalChannel);
					break;
				case 5:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,5),&LaserProperties[laserNum].DDS_Clock);
					break;
				case 6:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,6),&LaserProperties[laserNum].DDS_Div);
					if(LaserProperties[laserNum].DDS_Div<1)
					{
						LaserProperties[laserNum].DDS_Div=1;
						SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,6),LaserProperties[laserNum].DDS_Div);

					}
				break;
				case 7:
					GetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,7),&LaserProperties[laserNum].DDS_Type);
					break;
			}
			break;
	}
	return 0;
}


/*************************************************************************************************************************/
int CVICALLBACK LASER_TOGGLE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
/* Callback funtion for the Laser on/off button. */
{
	int laserNum;

	switch (event)  {
		case EVENT_COMMIT:
			GetCtrlVal (panel,PANEL_LASER_RING, &laserNum);
			GetCtrlVal (panel, control, &LaserProperties[laserNum].Active);
			SetDigitalChannels();
			SetLaserLabels();
			break;
	}

	return 0;
}


/*************************************************************************************************************************/
// Copies the LASER_NAMES into the appropriate label locations on the panel. Red bkgnd applied to active lasers, white
//	for inactive lasers.
// Also sets the name in the drop down box in LaserSettings.uir
void SetLaserLabels(void)
{
	int i;
	// Index offset into ACh names table
	// The cause of PhaseOMatic and Micromatic off by 2 was due to there being "+NUMBEROFANRITSU*2" in this calc.
	int laserRowOffset = NUMBERANALOGCHANNELS+NUMBERDDS;

	for( i=0; i < NUMBERLASERS; i++)
	{
		SetTableCellVal(panelHandle, PANEL_TBL_ANAMES, MakePoint(1,(i+1) + laserRowOffset), LaserProperties[i].Name);

		if (LaserProperties[i].Active ==1){
			SetTableCellAttribute(panelHandle, PANEL_TBL_ANAMES,MakePoint(1,(i+1) + laserRowOffset), ATTR_TEXT_BGCOLOR,0xFF3366L);
		} else {
			SetTableCellAttribute(panelHandle, PANEL_TBL_ANAMES,MakePoint(1,(i+1) + laserRowOffset), ATTR_TEXT_BGCOLOR,VAL_WHITE);
		}

		ReplaceListItem(panelHandle10,PANEL_LASER_RING,i,LaserProperties[i].Name,i);
	}
}


/*************************************************************************************************************************/
void FillLaserTable(int laserNum)
/*  Loads the laser table (for display) with the stored virtual parameters for the given laser */
{
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,1),LaserProperties[laserNum].Name);
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,2),LaserProperties[laserNum].IP);
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,3),LaserProperties[laserNum].Port);	///alan says to take this out!
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,4),LaserProperties[laserNum].DigitalChannel);
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,5),LaserProperties[laserNum].DDS_Clock);
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,6),LaserProperties[laserNum].DDS_Div);
	SetTableCellVal (panelHandle10, PANEL_LASER_SET_TABLE,MakePoint(2,7),LaserProperties[laserNum].DDS_Type);
	SetCtrlVal (panelHandle10, PANEL_LASER_TOGGLE, LaserProperties[laserNum].Active);
}


/*************************************************************************************************************************/
void LaserSettingsInit(void)
/*  Sets the LASER_CHANNEL array with values defined in LaserSettings2.h
	Fills table with laser0 data and sets ring menu to laser0
	Called at program startup */
{
	int i;
	// Index offset into ACh names table
	int laserRowOffset = NUMBERANALOGCHANNELS+NUMBERDDS;

	// Set the second cell to show the unts instead of the analog ch number
	for( i=0; i < NUMBERLASERS; i++ )
	{
		SetTableCellAttribute(panelHandle, PANEL_TBL_ANAMES, MakePoint(2,(i+1)+laserRowOffset), ATTR_CELL_TYPE, VAL_CELL_STRING);
		SetTableCellVal(panelHandle, PANEL_TBL_ANAMES, MakePoint(2,(i+1)+laserRowOffset),"MHz");
	}

	// I don't know what this line corresponds to for "dBm". Maybe was a cheap way to make Anritsu setting?
	//SetTableCellVal(panelHandle, PANEL_TBL_ANAMES, MakePoint(2,3 + NUMBERANALOGCHANNELS+NUMBERDDS + 2*NUMBEROFANRITSU +1),"dBm");

	SetLaserLabels();// set labels on the left side

	FillLaserTable(0);// set the gui subpanel LaserSettings.uir
	SetCtrlVal(panelHandle10, PANEL_LASER_RING, 0);// set the dropdown box to show first laser
}
