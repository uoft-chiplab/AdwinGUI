/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2013. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>
#include "GUIDesign.h"
#include "GUIDesign2.h"

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  SETUP                           1
#define  SETUP_GPIB_OK                   2       /* callback function: GPIB_OK_CALLBACK */
#define  SETUP_GPIB_LASTNO               3       /* callback function: DEVICENO_CALLBACK */
#define  SETUP_GPIB_DEVICENO             4       /* callback function: DEVICENO_CALLBACK */
#define  SETUP_GPIB_ADDRESS              5
#define  SETUP_SRS_ampl                  6
#define  SETUP_SRS_offst                 7
#define  SETUP_CHECKBOX                  8
#define  SETUP_GPIB_TXT_COMMAND          9
#define  SETUP_GPIB_TAB_VALS             10
#define  SETUP_GPIB_BUILD                11      /* callback function: GPIB_BUILD_CALLBACK */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK DEVICENO_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK GPIB_BUILD_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK GPIB_OK_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK MultiScan_AddGPIBCellToScan(int panelHandle, int controlID, int MenuItemID, void *callbackData);


#ifdef __cplusplus
    }
#endif
