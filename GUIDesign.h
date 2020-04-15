/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2016. All Rights Reserved.          */
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
#define  PANEL_CHECKBOX_10               2
#define  PANEL_CHECKBOX_9                3
#define  PANEL_CHECKBOX_8                4
#define  PANEL_CHECKBOX_7                5
#define  PANEL_CHECKBOX_6                6
#define  PANEL_CHECKBOX_5                7
#define  PANEL_CHECKBOX_4                8
#define  PANEL_CHECKBOX_3                9
#define  PANEL_CHECKBOX_2                10
#define  PANEL_CHECKBOX                  11
#define  PANEL_LABEL_9                   12      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_8                   13      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_7                   14      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_6                   15      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_5                   16      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_4                   17      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_3                   18      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_10                  19      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_2                   20      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_LABEL_1                   21      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_TIMETABLE                 22      /* callback function: TIMETABLE_CALLBACK */
#define  PANEL_ANALOGTABLE               23      /* callback function: ANALOGTABLE_CALLBACK */
#define  PANEL_TB_SHOWPHASE10            24      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE9             25      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_DIGTABLE                  26      /* callback function: DIGTABLE_CALLBACK */
#define  PANEL_TB_SHOWPHASE8             27      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE7             28      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE6             29      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE5             30      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE4             31      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE3             32      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE2             33      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_TB_SHOWPHASE1             34      /* callback function: TOGGLE_ALL_CALLBACK */
#define  PANEL_CMD_SCAN                  35      /* callback function: CMD_SCAN_CALLBACK */
#define  PANEL_CMD_RUN                   36      /* callback function: CMD_RUN_CALLBACK */
#define  PANEL_TOGGLEREPEAT              37
#define  PANEL_VALS_CHANGED              38      /* callback function: VALS_CHANGED_CALLBACK */
#define  PANEL_CMDSTOP                   39      /* callback function: CMDSTOP_CALLBACK */
#define  PANEL_TGL_NUMERICTABLE          40      /* callback function: TGLNUMERIC_CALLBACK */
#define  PANEL_DISPLAYDIAL               41      /* callback function: DISPLAYDIAL_CALLBACK */
#define  PANEL_TBL_ANALOGUNITS           42
#define  PANEL_MULTISCAN_NAMES_TABLE     43
#define  PANEL_TBL_DIGNAMES              44
#define  PANEL_TBL_ANAMES                45
#define  PANEL_TIMER                     46      /* callback function: TIMER_CALLBACK */
#define  PANEL_NUM_INSERTIONCOL          47      /* callback function: NUM_INSERTIONCOL_CALLBACK */
#define  PANEL_NUM_INSERTIONPAGE         48      /* callback function: NUM_INSERTIONPAGE_CALLBACK */
#define  PANEL_NUM_DDS3_OFFSET           49      /* callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_NUM_DDS2_OFFSET           50      /* callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_NUM_DDS_OFFSET            51      /* callback function: DDS_OFFSET_CALLBACK */
#define  PANEL_SCAN_TABLE_2              52
#define  PANEL_SCAN_TABLE                53
#define  PANEL_CANVAS_END                54
#define  PANEL_CANVAS_START              55
#define  PANEL_DECORATION                56
#define  PANEL_NUM_LOOP_REPS             57
#define  PANEL_NUM_LOOPCOL2              58
#define  PANEL_NUM_LOOPPAGE2             59
#define  PANEL_NUM_LOOPCOL1              60
#define  PANEL_NUM_LOOPPAGE1             61
#define  PANEL_SWITCH_LOOP               62      /* callback function: SWITCH_LOOP_CALLBACK */
#define  PANEL_DECORATION_2              63
#define  PANEL_CANVAS_LOOPLINE           64
#define  PANEL_Looping                   65
#define  PANEL_SRS_FREQ                  66
#define  PANEL_ANRITSU_OFFSET            67
#define  PANEL_MULTISCAN_POS_TABLE       68
#define  PANEL_MULTISCAN_VAL_TABLE       69
#define  PANEL_MULTISCAN_NUM_ROWS        70      /* callback function: MULTISCAN_NUMROWS_CALLBACK */
#define  PANEL_MULTISCAN_NUM_NUMERIC     71      /* callback function: MULTICSAN_NUM_CALLBACK */
#define  PANEL_MULTISCAN_ITS_NUMERIC     72
#define  PANEL_SCAN_KEEPRUNNING_CHK      73
#define  PANEL_MULTISCAN_LED2            74
#define  PANEL_MULTISCAN_LED1            75
#define  PANEL_LED_GRE                   76
#define  PANEL_LED_YEL                   77
#define  PANEL_LED_RED                   78
#define  PANEL_INFOTABLE                 79      /* callback function: INFOTABLE_CALLBACK */
#define  PANEL_MULTISCAN_DECORATION      80
#define  PANEL_LBL_DIGIOFFSET            81
#define  PANEL_FORCE_BUILD_CHK           82
#define  PANEL_LBL_GPIBOFFSET            83

#define  SUBPANEL1                       2
#define  SUBPANEL1_TEXTBOX               2

#define  SUBPANEL2                       3
#define  SUBPANEL2_NUM_SCANITER          2
#define  SUBPANEL2_NUM_SCANSTEP          3
#define  SUBPANEL2_NUM_SCANVAL           4


     /* Menu Bars, Menus, and Menu Items: */

#define  MENU                            1
#define  MENU_FILE                       2
#define  MENU_FILE_LOADSET_V16           3       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_LOADSET_V15           4       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_LOADSET_V13           5       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_SAVESET_V16           6       /* callback function: MENU_CALLBACK */
#define  MENU_FILE_SEPARATOR             7
#define  MENU_FILE_BOOTLOAD              8       /* callback function: BOOTLOAD_CALLBACK */
#define  MENU_FILE_MENU_EXPORT_CHANNEL   9       /* callback function: CONFIG_EXPORT_CALLBACK */
#define  MENU_FILE_MENU_EXPORT_PANEL     10      /* callback function: EXPORT_PANEL_CALLBACK */
#define  MENU_FILE_SEPARATOR_2           11
#define  MENU_FILE_QuitCallback          12      /* callback function: EXIT */
#define  MENU_SETTINGS                   13
#define  MENU_SETTINGS_ANRITSU           14      /* callback function: ANRITSUSET_CALLBACK */
#define  MENU_SETTINGS_ANALOG            15      /* callback function: ANALOGSET_CALLBACK */
#define  MENU_SETTINGS_DIGITAL           16      /* callback function: DIGITALSET_CALLBACK */
#define  MENU_SETTINGS_DDSSETUP          17      /* callback function: DDSSETUP_CALLBACK */
#define  MENU_SETTINGS_LASERSETUP        18      /* callback function: LASERSET_CALLBACK */
#define  MENU_SETTINGS_GPIB_SRS_SETUP    19      /* callback function: GPIB_SRS_CALLBACK */
#define  MENU_SETTINGS_SEPARATOR_3       20
#define  MENU_SETTINGS_CONTROLTEXT       21
#define  MENU_SETTINGS_CONTROLTEXT_SUBMEN 22
#define  MENU_SETTINGS_CONTROLTEXT_TITLE1 23     /* callback function: TITLE1_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE2 24     /* callback function: TITLE2_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE3 25     /* callback function: TITLE3_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE4 26     /* callback function: TITLE4_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE5 27     /* callback function: TITLE5_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE6 28     /* callback function: TITLE6_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE7 29     /* callback function: TITLE7_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE8 30     /* callback function: TITLE8_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLE9 31     /* callback function: TITLE9_CALLBACK */
#define  MENU_SETTINGS_CONTROLTEXT_TITLEX 32     /* callback function: TITLEX_CALLBACK */
#define  MENU_SETTINGS_DEBUG             33
#define  MENU_SETTINGS_BOOTADWIN         34      /* callback function: BOOTADWIN_CALLBACK */
#define  MENU_SETTINGS_PROCESSOR         35      /* callback function: PROCESSOR_CALLBACK */
#define  MENU_SETTINGS_CLEARPANEL        36      /* callback function: CLEARPANEL_CALLBACK */
#define  MENU_SETTINGS_RESETZERO         37      /* callback function: RESETZERO_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_SUBMENU 38
#define  MENU_SETTINGS_RESETZERO_SETLOW  39      /* callback function: MENU_ALLLOW_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_HOLD    40      /* callback function: MENU_HOLD_CALLBACK */
#define  MENU_SETTINGS_RESETZERO_BYCHNL  41      /* callback function: MENU_BYCHANNEL_CALLBACK */
#define  MENU_SETTINGS_NOTECHECK         42      /* callback function: NOTECHECK_CALLBACK */
#define  MENU_SETTINGS_SEPARATOR_4       43
#define  MENU_SETTINGS_SCANSETTING       44      /* callback function: SCANSETTING_CALLBACK */
#define  MENU_EDITMATRIX                 45
#define  MENU_EDITMATRIX_INSERTCOLUMN    46      /* callback function: INSERTCOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_DELETECOLUMN    47      /* callback function: DELETECOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_COPYCOLUMN      48      /* callback function: COPYCOLUMN_CALLBACK */
#define  MENU_EDITMATRIX_PASTECOLUMN     49      /* callback function: PASTECOLUMN_CALLBACK */
#define  MENU_UPDATEPERIOD               50
#define  MENU_UPDATEPERIOD_SETGD5        51      /* callback function: SETGD5_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD10       52      /* callback function: SETGD10_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD15       53      /* callback function: SETGD15_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD25       54      /* callback function: SETGD25_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD50       55      /* callback function: SETGD50_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD100      56      /* callback function: SETGD100_CALLBACK */
#define  MENU_UPDATEPERIOD_SETGD1000     57      /* callback function: SETGD1000_CALLBACK */
#define  MENU_PREFS                      58
#define  MENU_PREFS_COMPRESSION          59      /* callback function: COMPRESSION_CALLBACK */
#define  MENU_PREFS_SIMPLETIMING         60      /* callback function: SIMPLETIMING_CALLBACK */
#define  MENU_PREFS_SHOWARRAY            61      /* callback function: SHOWARRAY_CALLBACK */
#define  MENU_PREFS_DDS_OFF              62      /* callback function: DDS_OFF_CALLBACK */
#define  MENU_PREFS_STREAM_SETTINGS      63      /* callback function: STREAM_CALLBACK */


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


#ifdef __cplusplus
    }
#endif
