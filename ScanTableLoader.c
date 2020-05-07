#include <ansi_c.h>
#include <cvirte.h>
#include <userint.h>
#include "ScanTableLoader.h"
#include "ScanTableLoader2.h"
#include "vars.h"
#include "GUIDesign.h"




int CVICALLBACK SCAN_LOAD_CANCEL (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel(panelHandle8);

			break;
		}
	return 0;
}

int CVICALLBACK SCAN_TYPE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int mode, dimmed;

	switch (event)
		{
		case EVENT_VAL_CHANGED:
			// read the selected scan type and dimm or un-dimm multiscan controls accordingly
			GetCtrlVal(panelHandle8, SL_PANEL_SCAN_TYPE, &mode);
			if (mode < 3)
				dimmed = 1;
			else
			{
				dimmed = 0;
			}
			SetCtrlAttribute(panelHandle8, SL_PANEL_MSCAN_MASTERCOL, ATTR_DIMMED, dimmed);
			SetCtrlAttribute(panelHandle8, SL_PANEL_MSCAN_FIXSUMORDIFF, ATTR_DIMMED, dimmed);
			SetCtrlAttribute(panelHandle8, SL_PANEL_STEP_NUM, ATTR_DIMMED, 1 - dimmed);
			SetCtrlAttribute(panelHandle8, SL_PANEL_SCAN_INIT, ATTR_DIMMED, 1 - dimmed);
			SetCtrlAttribute(panelHandle8, SL_PANEL_SCAN_FIN, ATTR_DIMMED, 1 - dimmed);
			SetCtrlAttribute(panelHandle8, SL_PANEL_SCAN_ITER, ATTR_DIMMED, 1 - dimmed);
			break;
		}
	return 0;
}

int CVICALLBACK SCAN_LOAD_OK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	//Loads Scan Tables Based on Selections made in the ScanTableLoader Panel

	int steps=0,mode,iters,STCELLNUMS;
	int targetTable, targetColumn, masterColumn;
	double first,last,fixedVal;
	char buff[50]="";
	char buff2[200]="";


	switch (event)
		{
		case EVENT_COMMIT:

			GetNumTableRows (panelHandle,PANEL_SCAN_TABLE,&STCELLNUMS);
			GetCtrlVal (panelHandle8,SL_PANEL_SCAN_TYPE, &mode);
			GetCtrlVal (panelHandle8,SL_PANEL_STEP_NUM, &steps);
			GetCtrlVal (panelHandle8,SL_PANEL_SCAN_INIT, &first);
			GetCtrlVal (panelHandle8,SL_PANEL_SCAN_FIN, &last);
			GetCtrlVal (panelHandle8,SL_PANEL_SCAN_ITER, &iters);
			GetCtrlVal (panelHandle8,SL_PANEL_MSCAN_MASTERCOL, &masterColumn);
			GetCtrlVal (panelHandle8,SL_PANEL_MSCAN_FIXSUMORDIFF, &fixedVal);
			GetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_TABLE, &targetTable);
			GetCtrlVal (panelHandle8,SL_PANEL_NUM_TARGET_COLUMN, &targetColumn);

			if ((steps+1)*iters+1>STCELLNUMS||steps<1)
			{
				sprintf(buff,"%d",STCELLNUMS-1);
				strcat(buff2,"# of (Steps+1)*Iters/Step must be between 1-");
				strcat(buff2,buff);

				ConfirmPopup("USER ERROR",buff2);
				HidePanel(panelHandle8);
			}
			else if(iters<1)
			{
				ConfirmPopup("USER ERROR","Number of iterations must be greater than 1");
				HidePanel(panelHandle8);
			}
			else
			{

				switch(mode)
				{
					case 1:
						LoadLinearRamp(targetTable,targetColumn,steps,first,last,iters,STCELLNUMS);
						break;
					case 2:
						LoadExpRamp(targetTable,targetColumn,steps,first,last,iters,STCELLNUMS);
						break;
					case 3: // Constant sum
						LoadFixedToMaster(targetTable,targetColumn,masterColumn,1,fixedVal); break;
					case 4: // Constant difference
						LoadFixedToMaster(targetTable,targetColumn,masterColumn,2,fixedVal); break;
					case 5: // Comparator
						LoadFixedToMaster(targetTable,targetColumn,masterColumn,3,fixedVal); break;
					case 6: // Comparator (inv)
						LoadFixedToMaster(targetTable,targetColumn,masterColumn,4,fixedVal); break;

				}
				HidePanel(panelHandle8);
			}

				break;
			}
	return 0;
}

 void LoadLinearRamp(int tableHandle, int tableColumn,int steps,double first,double last,int iter,int STCELLNUMS)
 {
 	//Loads a linear ramp into the scan table with first and last values in steps

 	int i,j;
 	double slope;

 	slope = (double)((last-first)/steps);

 	for(i=0;i<=steps;i++)
 	{

 		for(j=0;j<iter;j++)
 		{
 			SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i*iter+j+1), slope*i+first);
 			//printf("Cell: %d\tNum: %f\n",i*iter+j+1,slope*i+first); testing
 		}

 	}

 	SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, (steps+1)*iter+1), -1000.0);

 	for (i=(steps+1)*iter+2;i<=STCELLNUMS;i++)
 		SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i),0.0);

 }

 void LoadExpRamp(int tableHandle, int tableColumn, int steps,double first,double last,int iter,int STCELLNUMS)
 {
    //Loads an exponential ramp into the scan table with first and last values in steps
    //Creates Values Following Form f(x)=last-amplitude*exp(bx)
    //+/- 1% Accuracy

    int i,j;
    double amplitude,b;

    amplitude = last-first;

    b=log(0.01)/steps;

    for (i=0;i<steps;i++)
    {
    	for(j=0;j<iter;j++)
    		SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i*iter+j+1), last-amplitude*exp(b*(double)i));
    }

    for(j=1;j<=iter;j++)
    	SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, steps*iter+j), last);

    SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, (steps+1)*iter+1), -1000.0);

    for (i=(steps+1)*iter+2;i<=STCELLNUMS;i++)
 		SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i),0.0);

 }


 void LoadFixedToMaster(int tableHandle, int tableColumn, int masterColumn, int mode, double fixedVal)
 {
 	int i, numRows, numCols;
 	double thisVal;

 	// Get number of rows and columns in table
 	GetNumTableRows(panelHandle, tableHandle, &numRows);
 	GetNumTableColumns(panelHandle, tableHandle, &numCols);

 	if ( (masterColumn > 0) && (masterColumn <= numCols) )
 	{
 		// Step through master column values until sentinel is found or end of list is reached.
 		// Note, that it works to fill a column dependent on its existing values (tableColumn = masterColumn),
 		// so this option is also available for the two single-column scanlists.
 		for (i=1;i<=numRows;i++)
 		{
 			// read next value in master column
 			GetTableCellVal(panelHandle, tableHandle, MakePoint(masterColumn, i), &thisVal);

 			// check for sentinel
 			if (thisVal < -999.0)
 			{
 				// copy sentinel
 				SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i), thisVal);
 				break;
 			}

 			// calculate dependent value according to mode
 			switch (mode) // note that mode = 0 or mode > 5 just copies the master column
 			{
 			case 1: thisVal = fixedVal - thisVal; break; // keep the sum of both columns fixed
 			case 2: thisVal = fixedVal + thisVal; break; // keep the difference between both columns fixed
 			case 3: // Comparator: TRUE when higher
 				if (thisVal >= fixedVal)
 					thisVal = 1.0;
 				else
 					thisVal = 0.0;
 				break;
 			case 4: // Comparator: TRUE when lower
 				if (thisVal <= fixedVal)
 					thisVal = 1.0;
 				else
 					thisVal = 0.0;
 				break;
 			}

 			// write value to target column
 			SetTableCellVal(panelHandle, tableHandle, MakePoint(tableColumn, i), thisVal);
 		}
 	}
 	else
 	{
 		MessagePopup("Scan table load error", "Selected master column does not exist!");
 	}

 }



