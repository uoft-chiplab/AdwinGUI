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

#define  TV_PANEL                         1
#define  TV_PANEL_GRAPH                   2       /* control type: graph, callback function: (none) */
#define  TV_PANEL_CMD_PLOT_DIG            3       /* control type: command, callback function: CMD_PLOT_DIG_CALLBACK */
#define  TV_PANEL_CMD_PLOT_ANA            4       /* control type: command, callback function: CMD_PLOT_ANA_CALLBACK */
#define  TV_PANEL_NUMERIC_4               5       /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_3               6       /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_2               7       /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_8               8       /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_7               9       /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_6               10      /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC_5               11      /* control type: numeric, callback function: (none) */
#define  TV_PANEL_NUMERIC                 12      /* control type: numeric, callback function: (none) */
#define  TV_PANEL_TEXTMSG                 13      /* control type: textMsg, callback function: (none) */
#define  TV_PANEL_EXIT_BUTTON             14      /* control type: command, callback function: EXIT_BUTTON_CALLBACK */
#define  TV_PANEL_TEXTMSG_2               15      /* control type: textMsg, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK CMD_PLOT_ANA_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_PLOT_DIG_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK EXIT_BUTTON_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif