#ifndef MLOPTIMIZE_H
#define MLOPTIMIZE_H

#include <userint.h>// For CVICALLBACK


// ---------------------------------------------------------------------------
// Machine-learning optimization (scikit-optimize gp_minimize) for AdwinGUI.
//
// The GUI owns the experiment loop. An external Python process (ml_optimizer.py)
// runs the Bayesian optimizer and communicates with the GUI purely through files
// in MLOpt.WorkDir:
//   ml_config.txt        GUI -> Py  (once): bounds, n_initial_points, n_calls, dir
//   suggest_%05d.txt     Py  -> GUI: next parameter vector, or a DONE token
//   result_%05d.txt      GUI -> Py : measured cost for that shot
//   ml_log.csv           GUI append-only log of (iter, cost, best, params, time)
// The cost/fitness for each shot is read from a file written by the (separate)
// atom-cloud fitting computer at a user-defined path template (see MLOpt).
//
// Parameter selection reuses the MultiScan POS_TABLE / MultiScan.Par[] machinery
// (the user picks cells via the existing right-click "Scan cell value" menu).
// ---------------------------------------------------------------------------


// Build the runtime ML panel (called once during startup, after the main panel
// and MultiScan controls exist).
void BuildMLOptPanel(void);

// Initialize MLOpt defaults (called once during startup).
void InitMLOptDefaults(void);

// Display / hide the ML panel.
void ShowMLOptPanel(void);

// Per-cycle handler, called from TIMER_CALLBACK when MLOpt.Active is TRUE. Reads
// the cost for the shot that just ran, feeds it back to Python, fetches the next
// suggestion, applies it and triggers the next cycle (or finishes the run).
void MLOpt_Step(void);

// Cleanly end an optimization run (restore original cell values, stop the timer).
void MLOpt_Finish(void);


// Callbacks (installed on runtime-built controls).
int CVICALLBACK ML_Start_Callback   (int panel, int control, int event, void *cbd, int e1, int e2);
int CVICALLBACK ML_Stop_Callback    (int panel, int control, int event, void *cbd, int e1, int e2);
int CVICALLBACK ML_Browse_Callback  (int panel, int control, int event, void *cbd, int e1, int e2);
int CVICALLBACK ML_ExportLog_Callback(int panel, int control, int event, void *cbd, int e1, int e2);
int CVICALLBACK ML_Refresh_Callback (int panel, int control, int event, void *cbd, int e1, int e2);
int CVICALLBACK ML_Panel_Callback   (int panel, int event, void *cbd, int e1, int e2);

// Menu-bar launcher (added to the main menu so it never overlaps existing controls).
void CVICALLBACK ML_Menu_Callback(int menuBar, int menuItem, void *callbackData, int panel);


#endif
