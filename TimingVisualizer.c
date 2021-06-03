//==============================================================================
//
// Title:		TimingVisualizer.c
// Purpose:		A short description of the implementation.
//
// Created on:	2021-05-25 at 3:49:12 PM by colin dale.
// Copyright:	University of Toronto - St. George Campus. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "TimingVisualizer.h"
#include "userint.h"
#include "main.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
int FindIndexFromAChNum(int size, int value) {
	int index = 0;
	while (index < size && AChName[index].chnum != value) ++index;
	return (index == size ? -1 : index);
}

int FindIndexFromDChNum(int size, int value) {
	int index = 0;
	while (index < size && DChName[index].chnum != value) ++index;
	return (index == size ? -1 : index);
}
//==============================================================================
// Global variables

//==============================================================================
// Global functions

/// HIFN  What does your function do?
/// HIPAR x/What inputs does your function expect?
/// HIRET What does your function return?
/**************************************************************************************/
int CVICALLBACK CMD_PLOT_ANA_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	// known issues: "analog channels" are actually rows so it will be confusing to use with the GUI since channels and rows are jumbled. 
	int colors[]={0xFF0000L, 0x00FF00L, 0x0000FFL, 0x00FFFFL, 0xFF00FFL, 0xFFFF00L, 0x800000L, 0x000080L, 0x008000L, 0x008080L, 0x8000080L, 0x808000L,
				 0xC0C0C0L, 0x808080L, 0x000000L, 0xFFFFFFL, 0xA0A0A0L, 0xE0E0E0L};
	const int numofch = 4;
	BOOL DEBUG =0;
	char chnames[numofch][40];
	int col;
	int skipindex;
	int skipcol=0;
	int page = GetPage();
	// make a loop that skips dimmed columns (time<=0) and makes a meta array consisting of non-dimmed columns. 
	// Because "copy previous" cells don't have real values, need to loop backwards until it finds the last "good" value and propagate it forward.
	for (col=1; col<NUMBEROFCOLUMNS; col++)
	{
		if (TimeArray[col][page]<=0) {
			skipcol++;
		}
	}
	int METACOLS = NUMBEROFCOLUMNS-skipcol;
	int skiparr[skipcol];
	int i, j, k,l;
	switch (event)
	{
		case EVENT_COMMIT:
			// clear plot
			DeleteGraphPlot (panel, TV_PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);
			
			int ch1, ch2, ch3, ch4;
			GetCtrlVal(panelHandle14,TV_PANEL_NUMERIC, &ch1);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_2, &ch2);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_3, &ch3);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_4, &ch4);
			
			int chs[numofch] = {ch1, ch2, ch3, ch4};
			int rows[numofch];
			for (int n = 0; n<numofch; n++) {
					rows[n] = FindIndexFromAChNum(NUMBERANALOGCHANNELS, chs[n]);
					strcpy(chnames[n], AChName[rows[n]].chname);
			}
			// loop over given rows, get values, and plot them
			int row;
			int insertindex;
			for (int l=0;l<numofch; l++) {
				int xyHandle;
				double ana[METACOLS];
				double rawtime[METACOLS];
				int currcol;
				int currpage;
				row = rows[l];
				insertindex=0;
				
				// fill meta arrays with time and analog values from the table
				for (col=1;col <= NUMBEROFCOLUMNS; col++) {
					if (TimeArray[col][page] <= 0) {
						// printf("Skipping column %d \n", col);
						continue;	
					}
					
					rawtime[insertindex] = TimeArray[col][page];
					
					// setting analog value
					if (AnalogTable[col][row][page].fcn != 6) {
						ana[insertindex] = AnalogTable[col][row][page].fval;
					} else {
						currcol=col;
						currpage=page;
						while (AnalogTable[currcol][row][currpage].fcn == 6 || TimeArray[currcol][currpage] <= 0) { // loop backwards until we find a cell that isn't pink and also active.
							currcol--;
							if (currcol < 1) { // ran out of columns on current page so move current page back by 1 and reset columns to end
								currpage--;
								currcol=NUMBEROFCOLUMNS-1;
							}
						}
						ana[insertindex]=AnalogTable[currcol][row][currpage].fval; 
					}
					insertindex++;
				}
				
				// make time array hold consecutive increasing values for plotting purposes
				double time[METACOLS];
				for (int t = 0; t < METACOLS;t++) {
					if (t>0) {
						time[t] = rawtime[t] + time[t-1];
					} else {
						time[t] = rawtime[t];	
					}
				}
			
				// plot
				xyHandle = PlotXY(panel, TV_PANEL_GRAPH, time, ana, METACOLS, VAL_DOUBLE, VAL_DOUBLE, VAL_CONNECTED_POINTS, VAL_SOLID_CIRCLE, VAL_SOLID, 1, colors[l]);
				// change legend to match channel names
				SetPlotAttribute(panelHandle14, TV_PANEL_GRAPH, xyHandle, ATTR_PLOT_LG_TEXT, chnames[l]);
			}

			break;
	}
	return 0;
	
error:
	printf("Row number must be >0.");
	return 0;
}

int  CVICALLBACK CMD_PLOT_DIG_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
		
	int colors[]={0xFF0000L, 0x00FF00L, 0x0000FFL, 0x00FFFFL, 0xFF00FFL, 0xFFFF00L, 0x800000L, 0x000080L, 0x008000L, 0x008080L, 0x8000080L, 0x808000L,
				 0xC0C0C0L, 0x808080L, 0x000000L, 0xFFFFFFL, 0xA0A0A0L, 0xE0E0E0L};
	const int numofch = 4;
	BOOL DEBUG =0;
	char chnames[numofch][40];
	int col;
	int skipindex;
	int skipcol=0;
	int page = GetPage();
	// make a loop that skips dimmed columns (time<=0) and makes a meta array consisting of non-dimmed columns. 
	// Because "copy previous" cells don't have real values, need to loop backwards until it finds the last "good" value and propagate it forward.
	for (col=1; col<NUMBEROFCOLUMNS; col++)
	{
		if (TimeArray[col][page]<=0) {
			skipcol++;
		}
	}
	int METACOLS = NUMBEROFCOLUMNS-skipcol;
	int skiparr[skipcol];
	int i, j, k,l;
	switch (event)
	{
		case EVENT_COMMIT:
			DeleteGraphPlot (panel, TV_PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);

			// read out values of numeric controls to get the requested channels
			int ch1, ch2, ch3, ch4;
			GetCtrlVal(panelHandle14,TV_PANEL_NUMERIC_5, &ch1);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_6, &ch2);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_7, &ch3);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_8, &ch4);
			
			int chs[numofch] = {ch1, ch2, ch3, ch4};
			int rows[numofch];
			for (int n = 0; n<numofch; n++) {
				rows[n] = FindIndexFromDChNum(NUMBERDIGITALCHANNELS, chs[n]);
				strcpy(chnames[n], DChName[rows[n]].chname);
			}
		
			// loop over given rows, get values, and plot them
			int row;
			int insertindex;
			for (int l=0;l<numofch; l++) {
				int xyHandle;
				double dig[METACOLS];
				double rawtime[METACOLS];
				int currcol;
				int currpage;
				row = rows[l];
				insertindex=0;
				for (col=1;col <= NUMBEROFCOLUMNS; col++) {
					// skip unused columns
					if (TimeArray[col][page] <= 0) {
						continue;	
					}
					
					rawtime[insertindex] = TimeArray[col][page];
					dig[insertindex] = DigTableValues[col][row][page];
					insertindex++;
				}
				double time[METACOLS];
				for (int t = 0; t < METACOLS;t++) {
					if (t>0) {
						time[t] = rawtime[t] + time[t-1];
					} else {
						time[t] = rawtime[t];	
					}
				}

				// plot
				xyHandle = PlotXY(panel, TV_PANEL_GRAPH, time, dig, METACOLS, VAL_DOUBLE, VAL_DOUBLE, VAL_CONNECTED_POINTS, VAL_SOLID_CIRCLE, VAL_SOLID, 1, colors[l]);
				SetPlotAttribute(panelHandle14, TV_PANEL_GRAPH, xyHandle, ATTR_PLOT_LG_TEXT, chnames[l]);
			}
			break;
	}
	return 0;
	
error:
	printf("Row number must be >0.");
	return 0;
}

int  CVICALLBACK EXIT_BUTTON_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	// Cancel and don't make any changes.
	switch (event)
		{
		case EVENT_COMMIT:
			 HidePanel(panelHandle14);
			break;
		}
	return 0;
}
