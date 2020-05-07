/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2013. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  SCANPANEL                       1
#define  SCANPANEL_NUM_ITERATIONS        2
#define  SCANPANEL_NUM_SCANSTEP          3
#define  SCANPANEL_NUM_SCANEND           4
#define  SCANPANEL_NUM_SCANSTART         5
#define  SCANPANEL_NUM_ROW_2             6
#define  SCANPANEL_NUM_ROW               7
#define  SCANPANEL_NUM_DURATION          8
#define  SCANPANEL_NUM_PAGE_2            9
#define  SCANPANEL_NUM_COLUMN_2          10
#define  SCANPANEL_NUM_CHANNEL           11
#define  SCANPANEL_NUM_PAGE              12
#define  SCANPANEL_NUM_COLUMN            13
#define  SCANPANEL_CMD_SCAN_CANCEL       14      /* callback function: CALLBACK_SCAN_CANCEL */
#define  SCANPANEL_CMD_SRSSCANOK_3       15      /* callback function: CALLBACK_SRSSCANOK2 */
#define  SCANPANEL_CMD_SRSSCANOK_2       16      /* callback function: CALLBACK_SRSSCANOK */
#define  SCANPANEL_CMD_DDSFLOORSCANOK_2  17      /* callback function: CALLBACK_DDSFLOORSCANOK2 */
#define  SCANPANEL_CMD_DDSFLOORSCANOK    18      /* callback function: CALLBACK_DDSFLOORSCANOK */
#define  SCANPANEL_CMD_LASSCANOK_2       19      /* callback function: CALLBACK_LASSCANOK2 */
#define  SCANPANEL_CMD_LASSCANOK         20      /* callback function: CALLBACK_LASSCANOK */
#define  SCANPANEL_CMD_DDSSCANOK_2       21      /* callback function: CALLBACK_DDSSCANOK2 */
#define  SCANPANEL_CMD_DDSSCANOK         22      /* callback function: CALLBACK_DDSSCANOK */
#define  SCANPANEL_CMD_TIMESCANOK_2      23      /* callback function: CALLBACK_TIMESCANOK2 */
#define  SCANPANEL_CMD_TIMESCANOK        24      /* callback function: CALLBACK_TIMESCANOK */
#define  SCANPANEL_CMD_SCANOK_2          25      /* callback function: CALLBACK_SCANOK2 */
#define  SCANPANEL_CMD_SCANOK            26      /* callback function: CALLBACK_SCANOK */
#define  SCANPANEL_RING_MODE             27
#define  SCANPANEL_DECORATION            28
#define  SCANPANEL_DECORATION_2          29
#define  SCANPANEL_NUM_TIMESTART         30
#define  SCANPANEL_NUM_TIMEEND           31
#define  SCANPANEL_NUM_TIMESTEP          32
#define  SCANPANEL_NUM_SRSEND_2          33
#define  SCANPANEL_NUM_TIMEITER          34
#define  SCANPANEL_NUM_SRSSTART_2        35
#define  SCANPANEL_NUM_DDSFLOOREND       36
#define  SCANPANEL_NUM_SRSITER_2         37
#define  SCANPANEL_NUM_SRSSTEP_2         38
#define  SCANPANEL_NUM_DDSSTART          39
#define  SCANPANEL_NUM_DDSFLOORSTART     40
#define  SCANPANEL_NUM_DDSEND            41
#define  SCANPANEL_NUM_DDSFLOORITER      42
#define  SCANPANEL_NUM_DDSFLOORSTEP      43
#define  SCANPANEL_NUM_DDSBASEFREQ       44
#define  SCANPANEL_NUM_DDSCURRENT        45
#define  SCANPANEL_NUM_LASITER           46
#define  SCANPANEL_NUM_LASSTEP           47
#define  SCANPANEL_NUM_DDSITER           48
#define  SCANPANEL_NUM_DDSSTEP           49
#define  SCANPANEL_CMD_GETSCANVALS_2     50      /* callback function: CMD_GETSCANVALS_CALLBACK2 */
#define  SCANPANEL_CMD_GETSCANVALS       51      /* callback function: CMD_GETSCANVALS_CALLBACK */
#define  SCANPANEL_CHECK_2PARAM          52      /* callback function: CHECK_2PARAM_CALLBACK */
#define  SCANPANEL_CHECK_USE_LIST        53      /* callback function: CHECK_USE_LIST_CALLBACK */
#define  SCANPANEL_DECORATION_3          54
#define  SCANPANEL_DECORATION_4          55
#define  SCANPANEL_NUM_LASEND            56
#define  SCANPANEL_NUM_LASSTART          57
#define  SCANPANEL_DECORATION_5          58
#define  SCANPANEL_TXT_LASIDENT          59
#define  SCANPANEL_DECORATION_7          60
#define  SCANPANEL_DECORATION_8          61
#define  SCANPANEL_DECORATION_9          62
#define  SCANPANEL_COMMANDBUTTON         63      /* callback function: CALLBACK_DONESCANSETUP */
#define  SCANPANEL_MULTISCAN_CHK         64      /* callback function: MULTISCAN_CHK */
#define  SCANPANEL_DECORATION_10         65
#define  SCANPANEL_DECORATION_6          66
#define  SCANPANEL_TEXTMSG               67


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK CALLBACK_DDSFLOORSCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_DDSFLOORSCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_DDSSCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_DDSSCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_DONESCANSETUP(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_LASSCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_LASSCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_SCAN_CANCEL(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_SCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_SCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_SRSSCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_SRSSCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_TIMESCANOK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CALLBACK_TIMESCANOK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CHECK_2PARAM_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CHECK_USE_LIST_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_GETSCANVALS_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_GETSCANVALS_CALLBACK2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MULTISCAN_CHK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
