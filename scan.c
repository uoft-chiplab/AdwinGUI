#include <userint.h>
#include "Scan.h"
#include "Scan2.h"
#include "GuiDesign.h"
/*
Displays information on the SCAN panel, and reads the information.
This code doesn't act on the scan information, that is done in GUIDesign.c::UpdateScanValue
Scan has 4 modes of operation:
Analog, Time, DDS (single cell) and DDS(floor)

*/

void EnableScanControls(void);

int CVICALLBACK CALLBACK_SCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			ReadScanValues();
			PScan.ScanMode=0;// set to Analog scan
			break;
		}
	return 0;
}

//********************************************************************************************************
// last update:  May 11, 2005
// May12, 2005:  initialize for time and DDS scans

void InitializeScanPanel(void)
{
 	SetCtrlVal (panelHandle7,SCANPANEL_NUM_COLUMN,PScan.Column);
 	SetCtrlVal (panelHandle7,SCANPANEL_NUM_ROW,PScan.Row);
	SetCtrlVal (panelHandle7,SCANPANEL_NUM_PAGE,PScan.Page);

 	SetCtrlVal (panelHandle7,SCANPANEL_NUM_CHANNEL,AChName[PScan.Row].chnum);
 	SetCtrlVal (panelHandle7,SCANPANEL_NUM_DURATION,TimeArray[PScan.Column][PScan.Page]);

 	SetCtrlVal (panelHandle7, SCANPANEL_RING_MODE,  	PScan.Analog.Analog_Mode);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANSTART, 	PScan.Analog.Start_Of_Scan);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANEND, 	PScan.Analog.End_Of_Scan);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANSTEP, 	PScan.Analog.Scan_Step_Size);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_ITERATIONS,	PScan.Analog.Iterations_Per_Step);

 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMESTART, 	PScan.Time.Start_Of_Scan);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMEEND, 	PScan.Time.End_Of_Scan);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMESTEP, 	PScan.Time.Scan_Step_Size);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMEITER,  	PScan.Time.Iterations_Per_Step);

 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSBASEFREQ,PScan.DDS.Base_Freq);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSSTART,  	PScan.DDS.Start_Of_Scan);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSEND, 	PScan.DDS.End_Of_Scan);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSSTEP, 	PScan.DDS.Scan_Step_Size);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSITER,   	PScan.DDS.Iterations_Per_Step);
 	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSCURRENT,	PScan.DDS.Current);

	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORSTART, PScan.DDSFloor.Floor_Start);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOOREND,   PScan.DDSFloor.Floor_End);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORSTEP,  PScan.DDSFloor.Floor_Step);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORITER,  PScan.DDSFloor.Iterations_Per_Step);

	SetCtrlVal (panelHandle7, SCANPANEL_NUM_LASSTART, PScan.Laser.Start_Of_Scan);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_LASEND,   PScan.Laser.End_Of_Scan);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_LASSTEP,  PScan.Laser.Scan_Step_Size);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_LASITER,  PScan.Laser.Iterations_Per_Step);

	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSSTART_2, PScan.SRS.SRS_Start);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSEND_2,   PScan.SRS.SRS_End);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSSTEP_2,  PScan.SRS.SRS_Step);
	SetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSITER_2,  PScan.SRS.Iterations_Per_Step);

	SetCtrlVal (panelHandle7, SCANPANEL_CHECK_2PARAM, TwoParam);

	DisplayPanel(panelHandle7);
}


//********************************************************************************************************
// last update:
// May 12,2005:  read in values for time and dds, set mode (ANALOG, TIME or DDS)
//               set initial values
void ReadScanValues(void)
{
//	PScan.Row=currenty;
//	PScan.Column=currentx;
//	PScan.Page=currentpage;

	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANSTART, &PScan.Analog.Start_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANEND,   &PScan.Analog.End_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SCANSTEP,  &PScan.Analog.Scan_Step_Size);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_ITERATIONS,&PScan.Analog.Iterations_Per_Step);
	GetCtrlVal (panelHandle7, SCANPANEL_RING_MODE,     &PScan.Analog.Analog_Mode);

	GetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMESTART, &PScan.Time.Start_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMEEND,   &PScan.Time.End_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMESTEP,  &PScan.Time.Scan_Step_Size);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_TIMEITER,  &PScan.Time.Iterations_Per_Step);

 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSBASEFREQ, &PScan.DDS.Start_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSSTART,    &PScan.DDS.Start_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSEND,      &PScan.DDS.End_Of_Scan);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSSTEP,     &PScan.DDS.Scan_Step_Size);
 	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSITER,     &PScan.DDS.Iterations_Per_Step);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSBASEFREQ, &PScan.DDS.Base_Freq);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSCURRENT,  &PScan.DDS.Current);

	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORSTART, &PScan.DDSFloor.Floor_Start);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOOREND,   &PScan.DDSFloor.Floor_End);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORSTEP,  &PScan.DDSFloor.Floor_Step);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_DDSFLOORITER,  &PScan.DDSFloor.Iterations_Per_Step);

	GetCtrlVal (panelHandle7, SCANPANEL_NUM_LASSTART, &PScan.Laser.Start_Of_Scan);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_LASEND,   &PScan.Laser.End_Of_Scan);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_LASSTEP,  &PScan.Laser.Scan_Step_Size);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_LASITER,  &PScan.Laser.Iterations_Per_Step);

	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSSTART_2, &PScan.SRS.SRS_Start);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSEND_2,   &PScan.SRS.SRS_End);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSSTEP_2,  &PScan.SRS.SRS_Step);
	GetCtrlVal (panelHandle7, SCANPANEL_NUM_SRSITER_2,  &PScan.SRS.Iterations_Per_Step);

	GetCtrlVal (panelHandle7, SCANPANEL_CHECK_2PARAM,  &TwoParam);

	ScanVal.Current_Step=0;



}



//********************************************************************************************************
// last update:
int CVICALLBACK CALLBACK_SCAN_CANCEL (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			PScan.Scan_Active=FALSE;
			break;
		}
	return 0;
}



//********************************************************************************************************
// last update:
int CVICALLBACK CALLBACK_TIMESCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			ReadScanValues();
			PScan.ScanMode=1;// set to Time scan
			break;
		}
	return 0;
}


//********************************************************************************************************
// last update:
int CVICALLBACK CALLBACK_DDSSCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			ReadScanValues();
			PScan.ScanMode=2;// set to DDS scan
			break;
		}
	return 0;
}


//********************************************************************************************************
// Added Mar09, 2006
int CVICALLBACK CALLBACK_DDSFLOORSCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			ReadScanValues();
			PScan.ScanMode=3;// set to scan the DDS Floor
			break;
		}
	return 0;
}
//********************************************************************************************************
//Added Aug 09, 2006
int CVICALLBACK CALLBACK_LASSCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			ReadScanValues();
			PScan.ScanMode=4;//set to laser scan
			break;
		}
	return 0;
}

//********************************************************************************************************

int CVICALLBACK CMD_GETSCANVALS_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int cx=0,cy=0,cz=0,laserNum;
	double dtemp;

	switch (event)
		{
		case EVENT_COMMIT:

			PScan.Column=currentx;
			PScan.Row=currenty;
			PScan.Page=currentpage;
			cx=currentx;cy=currenty;cz=currentpage;
			laserNum=cy-NUMBERANALOGCHANNELS-NUMBERDDS-1;


			PScan.Analog.Analog_Mode		=AnalogTable[cx][cy][cz].fcn;
			PScan.Analog.Start_Of_Scan		=AnalogTable[cx][cy][cz].fval;
			PScan.Analog.End_Of_Scan		=AnalogTable[cx][cy][cz].fval;
			PScan.Analog.Scan_Step_Size		=0.1;
			PScan.Analog.Iterations_Per_Step=1;

			PScan.Time.Start_Of_Scan		=TimeArray[cx][cz];
			PScan.Time.End_Of_Scan			=TimeArray[cx][cz];
			PScan.Time.Scan_Step_Size		=0.1;
			PScan.Time.Iterations_Per_Step	=1;

			PScan.DDS.Base_Freq				=ddstable[cx][cz].start_frequency;
			PScan.DDS.Start_Of_Scan			=ddstable[cx][cz].end_frequency;
			PScan.DDS.End_Of_Scan			=ddstable[cx][cz].end_frequency;
			PScan.DDS.Iterations_Per_Step	=1;
			PScan.DDS.Current				=ddstable[cx][cz].amplitude;

			if (laserNum>=0&&laserNum<NUMBERLASERS)
			{
				PScan.Laser.Start_Of_Scan		=LaserTable[laserNum][cx][cz].fval;
				PScan.Laser.End_Of_Scan			=PScan.Laser.Start_Of_Scan+10.0;
				PScan.Laser.Scan_Step_Size		=10.0;
				PScan.Laser.Iterations_Per_Step =1;
				SetCtrlVal(panelHandle7,SCANPANEL_TXT_LASIDENT,LaserProperties[laserNum].Name);
			}

			GetCtrlVal (panelHandle, PANEL_NUM_DDS_OFFSET, &dtemp);
			PScan.DDSFloor.Floor_Start		=dtemp;
			PScan.DDSFloor.Floor_End		=dtemp;
			PScan.DDSFloor.Floor_Step		=0.1;
			PScan.DDSFloor.Iterations_Per_Step=1;

			GetCtrlVal (panelHandle, PANEL_SRS_FREQ, &SRS_frequency);
			PScan.SRS.SRS_Start			=SRS_frequency;
			PScan.SRS.SRS_End			=SRS_frequency;
			PScan.SRS.SRS_Step			=0.1;
			PScan.SRS.Iterations_Per_Step	=1;

			InitializeScanPanel();
			DisplayPanel(panelHandle7);
			break;
		}
	return 0;
}

//********************************************************************************************************
int CVICALLBACK CHECK_USE_LIST_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_SRSSCANOK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan.ScanMode=5;// set to scan the DDS Floor
			break;
		}
	return 0;
}


int CVICALLBACK CHECK_2PARAM_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

		   	GetCtrlVal (panelHandle7, SCANPANEL_CHECK_2PARAM, &TwoParam);
			SetCtrlVal (panelHandle7, SCANPANEL_CHECK_2PARAM, TwoParam);

			if ((TwoParam) && (parameterscanmode>0))
			{   parameterscanmode = 2;  } // two-parameter scan
			else
			{	parameterscanmode = 1;	} // single-parameter scan

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_SCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=0;// set to Analog scan

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_TIMESCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=1;// set to Time scan

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_DDSSCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=2;// set to DDS scan

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_LASSCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=4;//set to laser scan

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_DDSFLOORSCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=3;// set to scan the DDS Floor

			break;
		}
	return 0;
}

int CVICALLBACK CALLBACK_SRSSCANOK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.ScanMode=5;// set to scan the DDS Floor

			break;
		}
	return 0;
}

int CVICALLBACK CMD_GETSCANVALS_CALLBACK2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int cx=0,cy=0,cz=0,laserNum2;
	double dtemp;

	switch (event)
		{
		case EVENT_COMMIT:

			PScan2.Column=currentx;
			PScan2.Row=currenty;
			PScan2.Page=currentpage;
			cx=currentx;cy=currenty;cz=currentpage;
			laserNum2=cy-NUMBERANALOGCHANNELS-NUMBERDDS-1;


			PScan2.Analog.Analog_Mode		=AnalogTable[cx][cy][cz].fcn;
			PScan2.Analog.Start_Of_Scan		=AnalogTable[cx][cy][cz].fval;
			PScan2.Analog.End_Of_Scan		=AnalogTable[cx][cy][cz].fval;
			PScan2.Analog.Scan_Step_Size		=0.1;
			PScan2.Analog.Iterations_Per_Step=1;

			PScan2.Time.Start_Of_Scan		=TimeArray[cx][cz];
			PScan2.Time.End_Of_Scan			=TimeArray[cx][cz];
			PScan2.Time.Scan_Step_Size		=0.1;
			PScan2.Time.Iterations_Per_Step	=1;

			PScan2.DDS.Base_Freq				=ddstable[cx][cz].start_frequency;
			PScan2.DDS.Start_Of_Scan			=ddstable[cx][cz].end_frequency;
			PScan2.DDS.End_Of_Scan			=ddstable[cx][cz].end_frequency;
			PScan2.DDS.Iterations_Per_Step	=1;
			PScan2.DDS.Current				=ddstable[cx][cz].amplitude;

			if (laserNum2>=0&&laserNum2<NUMBERLASERS)
			{
				PScan2.Laser.Start_Of_Scan		=LaserTable[laserNum2][cx][cz].fval;
				PScan2.Laser.End_Of_Scan			=PScan.Laser.Start_Of_Scan+10.0;
				PScan2.Laser.Scan_Step_Size		=10.0;
				PScan2.Laser.Iterations_Per_Step =1;
				SetCtrlVal(panelHandle7,SCANPANEL_TXT_LASIDENT,LaserProperties[laserNum2].Name);
			}

			GetCtrlVal (panelHandle, PANEL_NUM_DDS_OFFSET, &dtemp);
			PScan2.DDSFloor.Floor_Start		=dtemp;
			PScan2.DDSFloor.Floor_End		=dtemp;
			PScan2.DDSFloor.Floor_Step		=0.1;
			PScan2.DDSFloor.Iterations_Per_Step=1;

			GetCtrlVal (panelHandle, PANEL_SRS_FREQ, &SRS_frequency);
			PScan2.SRS.SRS_Start			=SRS_frequency;
			PScan2.SRS.SRS_End			=SRS_frequency;
			PScan2.SRS.SRS_Step			=0.1;
			PScan2.SRS.Iterations_Per_Step	=1;

			InitializeScanPanel();

			SetCtrlVal (panelHandle7,SCANPANEL_NUM_COLUMN_2,PScan2.Column);
 			SetCtrlVal (panelHandle7,SCANPANEL_NUM_ROW_2,PScan2.Row);
			SetCtrlVal (panelHandle7,SCANPANEL_NUM_PAGE_2,PScan2.Page);
			DisplayPanel(panelHandle7);
			break;
		}
	return 0;
}

// 2012-10-07 Stefan Trotzky -- Added multi-scan toggle control
int CVICALLBACK MULTISCAN_CHK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int ischecked;

	switch (event)
		{
		case EVENT_COMMIT:

		   	GetCtrlVal (panelHandle7, SCANPANEL_MULTISCAN_CHK, &ischecked);
			SetCtrlVal (panelHandle7, SCANPANEL_MULTISCAN_CHK, ischecked);

			if (ischecked)
			{
				parameterscanmode = 0;
			}
			else
			{
				GetCtrlVal (panelHandle7, SCANPANEL_CHECK_2PARAM, &ischecked);
				if (ischecked)
				{   parameterscanmode = 2;  } // two-parameter scan
				else
				{	parameterscanmode = 1;	} // single-parameter scan
			}

			break;
		}
	return 0;
}



int CVICALLBACK CALLBACK_DONESCANSETUP (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int ischecked;

	switch (event)
		{
		case EVENT_COMMIT:

			// Override parameterscanmode in case box is checked
		   	GetCtrlVal (panelHandle7, SCANPANEL_MULTISCAN_CHK, &ischecked);
			if (ischecked)
			{
				parameterscanmode = 0;
			}
			EnableScanControls();

			HidePanel(panelHandle7);
			HidePanel(panelHandle4);
			break;
		}
	return 0;
}


