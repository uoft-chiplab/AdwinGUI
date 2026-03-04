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
#define  PANEL_CHECKBOX_10                2       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_9                 3       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_8                 4       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_7                 5       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_6                 6       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_5                 7       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_4                 8       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_3                 9       /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX_2                 10      /* control type: radioButton, callback function: (none) */
#define  PANEL_CHECKBOX                   11      /* control type: radioButton, callback function: (none) */
#define  PANEL_LABEL_9                    12      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_8                    13      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_7                    14      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_6                    15      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_5                    16      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_4                    17      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_3                    18      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_10                   19      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_2                    20      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_1                    21      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_TIMETABLE                  22      /* control type: table, callback function: TIMETABLE_CALLBACK */
#define  PANEL_ANALOGTABLE                23      /* control type: table, callback function: ANALOGTABLE_CALLBACK */
#define  PANEL_TB_SHOWPHASE10             24      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE9              25      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_DIGTABLE                   26      /* control type: table, callback function: DIGTABLE_CALLBACK */
#define  PANEL_TB_SHOWPHASE8              27      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE7              28      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE6              29      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE5              30      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE4              31      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE3              32      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE2              33      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE1              34      /* control type: textButton, callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_CMD_SCAN                   35      /* control type: command, callback function: CMD_SCAN_CALLBACK */
#define  PANEL_CMD_RUN                    36      /* control type: command, callback function: CMD_RUN_CALLBACK */
#define  PANEL_TOGGLEREPEAT               37      /* control type: textButton, callback function: (none) */
#define  PANEL_VALS_CHANGED               38      /* control type: command, callback function: VALS_CHANGED_CALLBACK */
#define  PANEL_CMDSTOP                    39      /* control type: command, callback function: CMDSTOP_CALLBACK */
#define  PANEL_TGL_NUMERICTABLE           40      /* control type: textButton, callback function: TGLNUMERIC_CALLBACK */
#define  PANEL_DISPLAYDIAL                41      /* control type: slide, callback function: DISPLAYDIAL_CALLBACK */
#define  PANEL_TBL_ANALOGUNITS            42      /* control type: table, callback function: (none) */
#define  PANEL_MULTISCAN_NAMES_TABLE      43      /* control type: table, callback function: (none) */
#define  PANEL_TBL_DIGNAMES               44      /* control type: table, callback function: (none) */
#define  PANEL_TBL_ANAMES                 45      /* control type: table, callback function: (none) */
#define  PANEL_TIMER                      46      /* control type: timer, callback function: TIMER_CALLBACK */
#define  PANEL_NUM_INSERTIONCOL           47      /* control type: numeric, callback function: NUM_INSERTIONCOL_CALLBACK */
#define  PANEL_NUM_INSERTIONPAGE          48      /* control type: numeric, callback function: NUM_INSERTIONPAGE_CALLBACK */
#define  PANEL_NUM_DDS3_OFFSET            49      /* control type: numeric, callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_NUM_DDS2_OFFSET            50      /* control type: numeric, callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_NUM_DDS_OFFSET             51      /* control type: numeric, callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_SCAN_TABLE_2               52      /* control type: table, callback function: (none) */
#define  PANEL_SCAN_TABLE                 53      /* control type: table, callback function: (none) */
#define  PANEL_CANVAS_END                 54      /* control type: canvas, callback function: (none) */
#define  PANEL_CANVAS_START               55      /* control type: canvas, callback function: (none) */
#define  PANEL_DECORATION                 56      /* control type: deco, callback function: (none) */
#define  PANEL_NUM_LOOP_REPS              57      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_LOOPCOL2               58      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_LOOPPAGE2              59      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_LOOPCOL1               60      /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_LOOPPAGE1              61      /* control type: numeric, callback function: (none) */
#define  PANEL_SWITCH_LOOP                62      /* control type: binary, callback function: SWITCH_LOOP_CALLBACK */
#define  PANEL_DECORATION_2               63      /* control type: deco, callback function: (none) */
#define  PANEL_CANVAS_LOOPLINE            64      /* control type: canvas, callback function: (none) */
#define  PANEL_Looping                    65      /* control type: textMsg, callback function: (none) */
#define  PANEL_SRS_FREQ                   66      /* control type: numeric, callback function: (none) */
#define  PANEL_ANRITSU_OFFSET             67      /* control type: numeric, callback function: (none) */
#define  PANEL_MULTISCAN_POS_TABLE        68      /* control type: table, callback function: (none) */
#define  PANEL_MULTISCAN_VAL_TABLE        69      /* control type: table, callback function: (none) */
#define  PANEL_MULTISCAN_NUM_ROWS         70      /* control type: numeric, callback function: MULTISCAN_NUMROWS_CALLBACK */
#define  PANEL_MULTISCAN_NUM_NUMERIC      71      /* control type: numeric, callback function: MULTICSAN_NUM_CALLBACK */
#define  PANEL_MULTISCAN_ITS_NUMERIC      72      /* control type: numeric, callback function: (none) */
#define  PANEL_SCAN_KEEPRUNNING_CHK       73      /* control type: radioButton, callback function: (none) */
#define  PANEL_MULTISCAN_LED2             74      /* control type: LED, callback function: (none) */
#define  PANEL_MULTISCAN_LED1             75      /* control type: LED, callback function: (none) */
#define  PANEL_LED_GRE                    76      /* control type: LED, callback function: (none) */
#define  PANEL_LED_YEL                    77      /* control type: LED, callback function: (none) */
#define  PANEL_LED_RED                    78      /* control type: LED, callback function: (none) */
#define  PANEL_INFOTABLE                  79      /* control type: table, callback function: INFOTABLE_CALLBACK */
#define  PANEL_MULTISCAN_DECORATION       80      /* control type: deco, callback function: (none) */
#define  PANEL_LBL_DIGIOFFSET             81      /* control type: textMsg, callback function: (none) */
#define  PANEL_FORCE_BUILD_CHK            82      /* control type: radioButton, callback function: (none) */
#define  PANEL_TIMEVISUAL                 83      /* control type: pictButton, callback function: VisualizeTimingCallback */
#define  PANEL_LBL_GPIBOFFSET             84      /* control type: textMsg, callback function: (none) */

#define  SUBPANEL1                        2
#define  SUBPANEL1_TEXTBOX                2       /* control type: textBox, callback function: (none) */

#define  SUBPANEL2                        3
#define  SUBPANEL2_NUM_SCANITER           2       /* control type: numeric, callback function: (none) */
#define  SUBPANEL2_NUM_SCANSTEP           3       /* control type: numeric, callback function: (none) */
#define  SUBPANEL2_NUM_SCANVAL            4       /* control type: numeric, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

#define  MENU                             1
#define  MENU_FILE                        2
#define  MENU_FILE_LOADSET                3       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_SAVESET                4       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_LOADSET_V16            5       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_SAVESET_V16            6       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_LOADSET_V15            7       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_LOADSET_V13            8       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_SEPARATOR              9
#define  MENU_FILE_BOOTLOAD               10      /* callback function: BOOTLOAD_CALLBACK */
#define  MENU_FILE_MENU_EXPORT_CHANNEL    11      /* callback function: CONFIG_EXPORT_CALLBACK */
#define  MENU_FILE_MENU_EXPORT_PANEL      12      /* callback function: EXPORT_PANEL_CALLBACK */
#define  MENU_FILE_SEPARATOR_2            13
#define  MENU_FILE_QuitCallback           14      /* callback function: EXIT */
#define  MENU_SETTINGS                    15
#define  MENU_SETTINGS_ANRITSU            16      /* callback function: ANRITSUSET_CALLBACK */
#define  MENU_SETTINGS_ANALOG             17      /* callback function: ANALOGSET_CALLBACK */
#define  MENU_SETTINGS_DIGITAL            18      /* callback function: DIGITALSET_CALLBACK */
#define  MENU_SETTINGS_DDSSETUP           19      /* callback function: DDSSETUP_CALLBACK */
#define  MENU_SETTINGS_LASERSETUP         20      /* callback function: LASERSET_CALLBACK */
#define  MENU_SETTINGS_GPIB_SRS_SETUP     21      /* callback function: GPIB_SRS_CALLBACK */
#define  MENU_SETTINGS_SEPARATOR_3        22
#define  MENU_SETTINGS_CONTROLTEXT        23
#define  MENU_SETTINGS_CONTROLTEXT_SUBMENU 24
#define  MENU_SETTINGS_CONTROLTEXT_TITLE1 25      /* callback function: TITLE1_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE2 26      /* callback function: TITLE2_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE3 27      /* callback function: TITLE3_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE4 28      /* callback function: TITLE4_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE5 29      /* callback function: TITLE5_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE6 30      /* callback function: TITLE6_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE7 31      /* callback function: TITLE7_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE8 32      /* callback function: TITLE8_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE9 33      /* callback function: TITLE9_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLEX 34      /* callback function: TITLEX_CALLBACK */
#define  MENU_SETTINGS_DEBUG              35
#define  MENU_SETTINGS_BOOTADWIN          36      /* callback function: BOOTADWIN_CALLBACK */
#define  MENU_SETTINGS_PROCESSOR          37      /* callback function: PROCESSOR_CALLBACK */
#define  MENU_SETTINGS_CLEARPANEL         38      /* callback function: CLEARPANEL_CALLBACK */
#define  MENU_SETTINGS_RESETZERO          39      /* callback function: RESETZERO_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_SUBMENU  40
#define  MENU_SETTINGS_RESETZERO_SETLOW   41      /* callback function: MENU_ALLLOW_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_HOLD     42      /* callback function: MENU_HOLD_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_BYCHNL   43      /* callback function: MENU_BYCHANNEL_CALLBACK */
#define  MENU_SETTINGS_NOTECHECK          44      /* callback function: NOTECHECK_CALLBACK */
#define  MENU_SETTINGS_SEPARATOR_4        45
#define  MENU_SETTINGS_SCANSETTING        46      /* callback function: SCANSETTING_CALLBACK */
#define  MENU_EDITMATRIX                  47
#define  MENU_EDITMATRIX_INSERTCOLUMN     48      /* callback function: INSERTCOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_DELETECOLUMN     49      /* callback function: DELETECOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_COPYCOLUMN       50      /* callback function: COPYCOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_PASTECOLUMN      51      /* callback function: PASTECOLUMN_CALLBACK */
#define  MENU_UPDATEPERIOD                52
#define  MENU_UPDATEPERIOD_SETGD5         53      /* callback function: SETGD5_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD10        54      /* callback function: SETGD10_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD15        55      /* callback function: SETGD15_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD25        56      /* callback function: SETGD25_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD50        57      /* callback function: SETGD50_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD100       58      /* callback function: SETGD100_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD1000      59      /* callback function: SETGD1000_CALLBACK */
#define  MENU_PREFS                       60
#define  MENU_PREFS_COMPRESSION           61      /* callback function: COMPRESSION_CALLBACK */
#define  MENU_PREFS_SIMPLETIMING          62      /* callback function: SIMPLETIMING_CALLBACK */
#define  MENU_PREFS_SHOWARRAY             63      /* callback function: SHOWARRAY_CALLBACK */
#define  MENU_PREFS_DDS_OFF               64      /* callback function: DDS_OFF_CALLBACK */
#define  MENU_PREFS_STREAM_SETTINGS       65      /* callback function: STREAM_CALLBACK */


     /* Callback Prototypes: */

void CVICALLBACK ANALOGSET_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK ANALOGTABLE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK ANRITSUSET_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK BOOTADWIN_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK BOOTLOAD_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK CLEARPANEL_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK CMD_RUN_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_SCAN_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMDSTOP_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK COMPRESSION_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK CONFIG_EXPORT_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK COPYCOLUMN_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK DDS_OFF_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK DDS_OFFSET_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK DDSSETUP_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK DELETECOLUMN_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK DIGITALSET_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK DIGTABLE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK DISPLAYDIAL_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK EXIT(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK EXPORT_PANEL_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK GPIB_SRS_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK INFOTABLE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK INSERTCOLUMN_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK LASERSET_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MENU_ALLLOW_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MENU_BYCHANNEL_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MENU_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MENU_HOLD_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK MULTICSAN_NUM_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MULTISCAN_NUMROWS_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK NOTECHECK_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK NUM_INSERTIONCOL_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK NUM_INSERTIONPAGE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK PASTECOLUMN_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK PROCESSOR_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK RESETZERO_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SCANSETTING_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD1000_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD100_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD10_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD15_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD25_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD50_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SETGD5_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SHOWARRAY_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SIMPLETIMING_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK STREAM_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK SWITCH_LOOP_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TGLNUMERIC_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TIMER_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TIMETABLE_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK TITLE1_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE2_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE3_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE4_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE5_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE6_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE7_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE8_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLE9_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK TITLEX_CALLBACK(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK TOGGLE_ALL_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK VALS_CHANGED_CALLBACK(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK VisualizeTimingCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif