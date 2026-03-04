/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1
#define  PANEL_LASER_SET_DONE             2       /* control type: command, callback function: ExitLaserSettings */
#define  PANEL_LASER_RING                 3       /* control type: ring, callback function: LaserSelect */
#define  PANEL_LASER_SET_TABLE            4       /* control type: table, callback function: LaserSettingsTable */
#define  PANEL_LASER_TOGGLE               5       /* control type: radioButton, callback function: LASER_TOGGLE_CALLBACK */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK ExitLaserSettings(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK LASER_TOGGLE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK LaserSelect(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK LaserSettingsTable(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif