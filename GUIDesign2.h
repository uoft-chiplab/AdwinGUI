#ifndef GUIDESIGN2_H
#define GUIDESIGN2_H

#include <ansi_c.h>
#include <userint.h>
#include "Adwin.h"
#include "vars.h"
#include "AnalogControl2.h"
#include "DDSControl2.h"
#include "ddstranslator.h"
#include "DDSSettings2.h"
#include "Scan2.h"
#include "GPIB_SRS_SETUP2.h"
#define PRINT_TO_DEBUG //if defined, outputs the array to the debug window

struct AnVals{
	int		fcn;		//fcn is an integer refering to a function to use.
						// 0-step, 1-linear, 2- exp, 3- 'S' curve
	double 	fval;		//the final value
	double	tscale;		//the timescale to approach final value
	} ;

void LoadSettings(int);
void SaveSettings(int);
void LoadLastSettings(int check);
void ShiftColumn3(int col, int page,int dir);
void RunOnce(void);
int  GetPage(void);
void DrawNewTable(int dimmed);
void CheckActivePages(void);
void SaveArraysV16(char*,int);
void SaveArraysV15(char*,int);
void LoadLaserData(char *,int);
void SaveLaserData(char *,int);
int LoadArraysV16(char*,int);
void LoadArraysV15(char*,int);
void LoadArraysV13(char*,int);
void ExportPanel(char*,int);
void BuildUpdateList(double TMatrix[],struct AnVals AMat[NUMBERANALOGCHANNELS+1][500],int DMat[NUMBERDIGITALCHANNELS+1][500],ddsoptions_struct DDSArray[500],ddsoptions_struct DDS2Array[500],dds3options_struct DDS3Array[500],unsigned int LaserTriggerArray[NUMBERLASERS][500],int numtimes,int forceBuild);
void SeqError(char * msg);
int int_power(int base, int power);
double CalcFcnValue(int fcn,double Vinit,double Vfinal, double timescale,double telapsed,double celltime);

void ReshapeAnalogTable(int,int,int);
void ReshapeDigitalTable(int,int,int);
void SetChannelDisplayed(int display_setting); //analog, digital of both
void SetDisplayType(int display_setting); //toggle graphic and numeric
double CheckIfWithinLimits(double OutputVoltage, int linenumber);
void SaveLastGuiSettings(void);
void OptimizeTimeLoop(int *,int,int*);
void UpdateScanValue(int);
void UpdateScan1Value(int);
void UpdateScan2Value(int);
void ScanSetUp(void);
void ExportScanBuffer(void);
void ExportScan2Buffer(void);
double findLastVal(int row, int column, int page);

void EnableScanControls(void);
void AutoExportMultiScanBuffer(void);
void ExportMultiScanBuffer(void);
void UpdateMultiScanValues(int);
int ToDigital(double);
void GetNewMultiScanCommands(void);
void updateScannedCellsWithScanTableLine(int);
void updateScannedCellsWithOriginalValues(void);
void writeToScanInfoFile(void);

int SetupScanFiles(int version, char *outputCmdsFileDir);


void MoveCanvasStart(int,int); // start arrow indicator, (x pos,on/off (i.e. True/False));
void MoveCanvasEnd(int,int); // end arrow indicator, (x pos,on/off (i.e. True/False));
void DrawLoopIndicators(void);// draw lines to indicate looping region


// Right-click menu callback prototypes
void CVICALLBACK Dig_Cell_Copy(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Dig_Cell_Paste(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Analog_Cell_Copy(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Analog_Cell_Paste(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Scan_Table_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Scan_Table_NumSet_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Scan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_AddCellToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_AddAnalogCellTimeScaleToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_SetVals(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_DeleteColumn(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData);



int scancount;
#endif
