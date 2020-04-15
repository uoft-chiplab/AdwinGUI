/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2011. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  ANRITPANEL                      1
#define  ANRITPANEL_ANRITSU_SET_DONE     2       /* callback function: ANRITSU_SET_DONE_CALLBACK */
#define  ANRITPANEL_ANRITSU_IPADDBOX     3
#define  ANRITPANEL_ANR_DIG_TRIG         4
#define  ANRITPANEL_ANR_DWELL_TIME       5
#define  ANRITPANEL_ANRITSU_COM_SWITCH   6
#define  ANRITPANEL_ANRITSU_OFFSET       7


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */ 

int  CVICALLBACK ANRITSU_SET_DONE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
