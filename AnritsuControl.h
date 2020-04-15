/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2010. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                           1
#define  PANEL_ANRITSU_LAST_POWR         2
#define  PANEL_ANRITSU_TARGET_POWR       3
#define  PANEL_ANRITSU_CONTROL_MOD_P     4
#define  PANEL_ANRITSU_LAST_FREQ         5
#define  PANEL_ANRITSU_TARGET_FREQ       6
#define  PANEL_ANRITSU_CONTROL_MODE      7
#define  PANEL_CANCEL_ANRITSU_CONT       8       /* callback function: CancelANRITSUConCALLBACK */
#define  PANEL_SET_ANRITSU_CON           9       /* callback function: SetANRITSUConCALLBACK */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */ 

int  CVICALLBACK CancelANRITSUConCALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetANRITSUConCALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
