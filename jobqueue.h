#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <userint.h>

// Initialize the queue panel (call from initializeGUI)
void initializeQueuePanel(void);

// Core queue operations
void Queue_CaptureCurrentConfig(void);         // Snapshot main panel tables → new job
void Queue_LoadJobToPanel(int jobIdx);          // Write job's tables back to main panel
void Queue_AdvanceToNextJob(void);             // Called at scan completion when QueueActive
void Queue_Start(void);                         // Begin executing queue from first Pending job
void Queue_Stop(void);                          // Halt queue execution
int  Queue_LoadCSV(int jobIdx, const char *csvpath);  // 0=ok, -1=dim mismatch
void Queue_SaveToFile(const char *path);        // Write .queue file
int  Queue_LoadFromFile(const char *path);      // Read .queue file, returns 0=ok
void Queue_RefreshPanelList(void);              // Repopulate queue list control

// Button callbacks (registered with InstallCtrlCallback)
int CVICALLBACK Queue_CB_AddJob     (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_UpdateJob  (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_RemoveJob  (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_EditJob    (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_MoveUp     (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_MoveDown   (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_LoadCSV    (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_SaveFile   (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_LoadFile   (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_StartQueue (int panel, int control, int event, void *cd, int e1, int e2);
int CVICALLBACK Queue_CB_StopQueue  (int panel, int control, int event, void *cd, int e1, int e2);

#endif
