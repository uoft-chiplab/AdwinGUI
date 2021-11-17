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

//==============================================================================
// Global variables

//==============================================================================
// Global functions

/// HIFN  What does your function do?
/// HIPAR x/What inputs does your function expect?
/// HIRET What does your function return?
/**************************************************************************************/
int CVICALLBACK CMD_PLOT_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	switch (event)
	{
		case EVENT_COMMIT:
			
		
			DeleteGraphPlot (panel, TV_PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW);

			// make a loop that skips dimmed columns and makes a meta array consisting of non-dimmed columns. 
			// Because "copy previous" cells don't have real values, loop backwards until it finds the last "good" value and propagate it forward.
			BOOL DEBUG =0;
			int col;
			int skipindex;
			int skipcol=0;
			int page = GetPage();
			for (col=1; col<NUMBEROFCOLUMNS; col++)
			{
				if (TimeArray[col][page]<=0) {
					skipcol++;
				}
			}
			
			// read out values of numeric controls to get the rows requested to be displayed
			int row1, row2, row3;
			GetCtrlVal(panelHandle14,TV_PANEL_NUMERIC, &row1);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_2, &row2);
			GetCtrlVal(panelHandle14, TV_PANEL_NUMERIC_3, &row3);
			printf("row1: %d"
			int rows[3] = {row1, row2, row3};
			for (int n=0; n<3; n++) {
				if (rows[n]<=0) {
					goto error;
				}
			}
			
			int METACOLS = NUMBEROFCOLUMNS-skipcol;
			int skiparr[skipcol];
			BOOL skip;
			int i, j, k,l, mindex;

			for (int r=0;r<3; r++) {
				int xyHandle;
				double testana[METACOLS];
				double testdig[METACOLS];
				double testtime[METACOLS];
				int currcol;
				int currpage;
				int row = rows[r];
				int insertindex=0;
				for (col=1;col <= NUMBEROFCOLUMNS; col++) {
					
					if (TimeArray[col][page] <= 0) {
						// printf("Skipping column %d \n", col);
						continue;	
					}
					
					testtime[insertindex] = TimeArray[col][page];
					testdig[insertindex] = DigTableValues[col][row][page];
					if (AnalogTable[col][row][page].fcn != 6) {
						testana[insertindex] = AnalogTable[col][row][page].fval;
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
						testana[insertindex]=AnalogTable[currcol][row][currpage].fval; 
					}
					insertindex++;
				}
				
				double time[METACOLS];
				for (int t = 0; t < METACOLS;t++) {
					if (t>0) {
						time[t] = testtime[t] + time[t-1];
					} else {
						time[t] = testtime[t];	
					}
				}
				/*int currcol;
				int currpage;
				
				for (int col=1; col<NUMBEROFCOLUMNS; col++) 
				{
					
					if (AnalogTable[col][row][page].fcn!=6) {
						testrow[col-1]=AnalogTable[col][row][page].fval;
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
						testrow[col-1]=AnalogTable[currcol][row][currpage].fval; 
					}
		
					digtestrow[col-1]=DigTableValues[col][row][page];
					
					// copy raw time values
					if (TimeArray[col][page] <= 0) {
						rawtesttime[col-1]=0;	
					} else {
						rawtesttime[col-1]=TimeArray[col][page];
					}
					
					// sum consecutive time cells to make an increasing array
					if (col==1) {
						testtime[col-1]=0;	
					} else {
						testtime[col-1]=rawtesttime[col-1]+testtime[col-2];	
					}
				}
				*/
				// DEBUG
				if (DEBUG) {
					printf("testana:"); // very strange, printed values don't match loaded values at all.... problem somewhat persists on fresh build. Something is weird with how the analog values are "locked in" after setting them,
					// especially in skipped columns. EDIT: it's because those cells weren't steps.... copy previous, etc. have a transformation between displayed value and internal value.
					for (i=0;i<METACOLS;i++){
						printf("%f ",testana[i]);
					}
					printf("\n testdig:");
					for (k=0;k<METACOLS;k++){
						printf("%f ",testdig[k]);
					}
				
					printf("\n testtime:");
					for (l=0;l<METACOLS;l++){
						printf("%f ",testtime[l]);
					}
					printf("time:\n");
					for (j=0;j<METACOLS;j++){
						printf("%f ",time[j]);
					}
				}
				
				// plot
				xyHandle = PlotXY(panel, TV_PANEL_GRAPH, time, testdig, METACOLS, VAL_DOUBLE, VAL_DOUBLE, VAL_CONNECTED_POINTS, VAL_SOLID_CIRCLE, VAL_SOLID, 1, VAL_RED);
				
			}
			break;
	}
	return 0;
	
error:
	printf("Row number must be >0.");
	return 0;
}

