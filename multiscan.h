#ifndef MULTISCAN_H
#define MULTISCAN_H


#include <userint.h>// For CVICALLBACK



// MultiScan functions
int SetupScanFiles(int version, char *outputCmdsFileDir);
void UpdateMultiScanValues(int);
void GetNewMultiScanCommands(void);
void AutoExportMultiScanBuffer(void);
void ExportMultiScanBuffer(void);
void updateScannedCellsWithScanTableLine(int);
void updateScannedCellsWithOriginalValues(void);
void writeToScanInfoFile(void);
void EnableScanControls(void);
void ReshapeMultiScanTable( int top,int left,int height);

// MultiScan callbacks
void CVICALLBACK MultiScan_AddCellToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_AddAnalogCellTimeScaleToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_SetVals(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_DeleteColumn(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK MultiScan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData);

// Helper functions
int ToDigital(double);


// Scan12 functions and callbacks
void UpdateScanValue(int);
void UpdateScan1Value(int);
void UpdateScan2Value(int);
void ExportScanBuffer(void);
void ExportScan2Buffer(void);
void CVICALLBACK Scan_Table_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Scan_Table_NumSet_Load(int panelHandle, int controlID, int MenuItemID, void *callbackData);
void CVICALLBACK Scan_Table_Shuffle(int panelHandle, int controlID, int MenuItemID, void *callbackData);



#endif