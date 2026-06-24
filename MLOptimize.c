// MLOptimize.c
// ---------------------------------------------------------------------------
// Machine-learning optimization of experimental parameters, modelled on
// Section 4.7.5 of Beregi (2024). An external Python process (ml_optimizer.py)
// runs scikit-optimize's gp_minimize (Bayesian optimization with a Gaussian
// process). The GUI owns the loop: it applies the optimizer's suggested
// parameter vector to the device tables (reusing the MultiScan machinery),
// runs one cycle, waits, reads a cost/fitness value written by the separate
// atom-cloud fitting computer, and hands (params, cost) back to Python.
//
// All inter-process communication is via files in MLOpt.WorkDir, polled with
// FileExists. Parsing/formatting uses stdio (fopen/fgets/fprintf/sscanf), which
// is already used elsewhere in this project (multiscan.c, main.c).
// ---------------------------------------------------------------------------

#include "MLOptimize.h"

#include <ansi_c.h>
#include <userint.h>
#include <utility.h>
#include <formatio.h>
#include "toolbox.h"

#include "vars.h"
#include "GUIDesign.h"   // PANEL_*, panelHandle, menuHandle
#include "GUIDesign2.h"  // RunOnce, DrawNewTable
#include "multiscan.h"   // UpdateMultiScanValues, applyValueToScannedCell,
                         // updateScannedCellsWithOriginalValues, SetupScanFiles,
                         // EnableScanControls


// --- Runtime panel + control handles (file scope; only this module needs them) ---
static int mlPanel        = 0;
static int mlParamTable   = 0;
static int mlCostPath     = 0;
static int mlBrowseBtn    = 0;
static int mlCostFmt      = 0;
static int mlSettleDelay  = 0;
static int mlCostTimeout  = 0;
static int mlInitPoints   = 0;
static int mlTotalCalls   = 0;
static int mlDirection    = 0;
static int mlPythonExe    = 0;
static int mlScriptPath   = 0;
static int mlStatusCtrl   = 0;
static int mlBestCtrl     = 0;
static int mlStartBtn     = 0;
static int mlStopBtn      = 0;
static int mlExportBtn    = 0;
static int mlRefreshBtn   = 0;

static int mlPythonHandle = 0;   // handle returned by LaunchExecutableEx

// Columns of the parameter table (1-based, CVI convention).
#define MLCOL_NAME   1
#define MLCOL_PAGE   2
#define MLCOL_COL    3
#define MLCOL_ROW    4
#define MLCOL_LOWER  5
#define MLCOL_UPPER  6
#define MLNUMCOLS    6


// ===========================================================================
// Small helpers
// ===========================================================================

// Set the status line.
static void ml_status(const char *msg){
	if(mlStatusCtrl) SetCtrlVal(mlPanel, mlStatusCtrl, msg);
	printf("ML: %s\n", msg);
}

// Build a path "WorkDir/<name>".
static void ml_workpath(const char *name, char *out){
	MakePathname(MLOpt.WorkDir, (char*)name, out);
}

// Poll for a file until it exists or the timeout (ms) elapses. Returns 1 if the
// file appeared, 0 on timeout. Keeps the GUI responsive via ProcessSystemEvents.
static int ml_poll_file(const char *path, int timeoutMs){
	double start = Timer();
	while( FileExists((char*)path, 0) != 1 ){
		if( (Timer()-start)*1000.0 > (double)timeoutMs )
			return 0;
		ProcessSystemEvents();
		Delay(0.01);
	}
	return 1;
}


// ===========================================================================
// IPC: config / suggestion / result / log
// ===========================================================================

// Write ml_config.txt for the Python optimizer.
static void ml_write_config(void){
	char path[MAX_PATHNAME_LEN];
	FILE *f;
	int j;

	ml_workpath("ml_config.txt", path);
	f = fopen(path, "w");
	if(!f){ ml_status("ERROR: cannot write ml_config.txt"); return; }

	fprintf(f, "n_params %d\n",         MLOpt.NumPars);
	fprintf(f, "n_initial_points %d\n", MLOpt.InitPoints);
	fprintf(f, "n_calls %d\n",          MLOpt.TotalCalls);
	fprintf(f, "direction %d\n",        MLOpt.Direction); // 0 max, 1 min
	for(j=0; j<MLOpt.NumPars; j++)
		fprintf(f, "bound %.10g %.10g\n", MLOpt.Bounds[j].Lower, MLOpt.Bounds[j].Upper);

	fclose(f);
}

// Read suggest_%05d.txt. Fills MLOpt.Suggested[]. Returns:
//   1  = a valid parameter vector was read
//   0  = the optimizer signalled completion (DONE token)
//  -1  = file missing/timed out or malformed
static int ml_read_suggestion(int n){
	char path[MAX_PATHNAME_LEN];
	char name[64];
	char line[256];
	FILE *f;
	int j;
	double v;

	sprintf(name, "suggest_%05d.txt", n);
	ml_workpath(name, path);

	if( !ml_poll_file(path, MLOpt.CostTimeoutMs) ){
		ml_status("ERROR: timed out waiting for optimizer suggestion");
		return -1;
	}

	f = fopen(path, "r");
	if(!f){ ml_status("ERROR: cannot open suggestion file"); return -1; }

	// First line is a status token: "OK" or "DONE".
	if( !fgets(line, sizeof(line), f) ){ fclose(f); return -1; }
	if( strncmp(line, "DONE", 4) == 0 ){ fclose(f); return 0; }

	// Following lines: one parameter value per line, in parameter order.
	for(j=0; j<MLOpt.NumPars; j++){
		if( !fgets(line, sizeof(line), f) || sscanf(line, "%lf", &v) != 1 ){
			fclose(f);
			ml_status("ERROR: malformed suggestion file");
			return -1;
		}
		MLOpt.Suggested[j] = v;
	}
	fclose(f);
	return 1;
}

// Write result_%05d.txt with the measured (raw) cost for shot n. Python applies
// the maximize/minimize sign itself.
static void ml_write_result(int n, double cost){
	char path[MAX_PATHNAME_LEN];
	char name[64];
	FILE *f;

	sprintf(name, "result_%05d.txt", n);
	ml_workpath(name, path);
	f = fopen(path, "w");
	if(!f){ ml_status("ERROR: cannot write result file"); return; }
	fprintf(f, "%.10g\n", cost);
	fclose(f);
}

// Read the cost/fitness for shot n from the fitting computer's file. The path is
// built from the user-defined template (which should contain a printf integer
// field for the shot index, e.g. "Z:\\fit\\cost_%05d.txt"). The numeric value is
// parsed with the user-defined scan format (default "%lf"). Sets *ok to 1/0.
static double ml_read_cost(int n, int *ok){
	char path[MAX_PATHNAME_LEN];
	char line[512];
	FILE *f;
	double cost = 0.0;

	*ok = 0;

	// Substitute the shot index into the user template.
	sprintf(path, MLOpt.CostPathTemplate, n);

	if( !ml_poll_file(path, MLOpt.CostTimeoutMs) ){
		ml_status("ERROR: timed out waiting for cost file");
		return 0.0;
	}

	f = fopen(path, "r");
	if(!f){ ml_status("ERROR: cannot open cost file"); return 0.0; }
	if( fgets(line, sizeof(line), f) && sscanf(line, MLOpt.CostScanFmt, &cost) == 1 )
		*ok = 1;
	else
		ml_status("ERROR: could not parse cost value");
	fclose(f);
	return cost;
}

// Append one row to ml_log.csv (creating it with a header on first write).
static void ml_append_log(int n, double cost){
	char path[MAX_PATHNAME_LEN];
	FILE *f;
	int j, isNew;
	int hour, minute, second;

	ml_workpath("ml_log.csv", path);
	isNew = (FileExists(path, 0) != 1);

	f = fopen(path, "a");
	if(!f) return;

	if(isNew){
		fprintf(f, "iter,cost,best");
		for(j=0; j<MLOpt.NumPars; j++) fprintf(f, ",p%d", j+1);
		fprintf(f, ",time\n");
	}

	GetSystemTime(&hour, &minute, &second);
	fprintf(f, "%d,%.10g,%.10g", n, cost, MLOpt.BestCost);
	for(j=0; j<MLOpt.NumPars; j++) fprintf(f, ",%.10g", MLOpt.Suggested[j]);
	fprintf(f, ",%02d:%02d:%02d\n", hour, minute, second);

	fclose(f);
}


// ===========================================================================
// Applying suggestions / tracking the best result
// ===========================================================================

// Apply the current MLOpt.Suggested[] vector to the device tables, reusing the
// MultiScan cell-application logic.
static void ml_apply_suggestion(void){
	int j;
	for(j=0; j<MLOpt.NumPars; j++)
		applyValueToScannedCell(j, MLOpt.Suggested[j]);
}

// Update the best-so-far record (direction aware: maximize keeps the largest
// cost, minimize keeps the smallest).
static void ml_update_best(double cost){
	int j;
	int better;

	if(!MLOpt.HaveBest)
		better = 1;
	else if(MLOpt.Direction == MLOPT_OBJ_MAXIMIZE)
		better = (cost > MLOpt.BestCost);
	else
		better = (cost < MLOpt.BestCost);

	if(better){
		MLOpt.BestCost = cost;
		for(j=0; j<MLOpt.NumPars; j++) MLOpt.BestParams[j] = MLOpt.Suggested[j];
		MLOpt.HaveBest = 1;
	}
}

// Refresh the best-so-far display.
static void ml_show_best(void){
	char buf[512];
	char tmp[64];
	int j;

	if(!mlBestCtrl) return;
	if(!MLOpt.HaveBest){ SetCtrlVal(mlPanel, mlBestCtrl, "Best so far: (none yet)"); return; }

	sprintf(buf, "Best so far: cost=%.6g @", MLOpt.BestCost);
	for(j=0; j<MLOpt.NumPars; j++){
		sprintf(tmp, " p%d=%.4g", j+1, MLOpt.BestParams[j]);
		strcat(buf, tmp);
	}
	SetCtrlVal(mlPanel, mlBestCtrl, buf);
}


// ===========================================================================
// Run lifecycle: start / per-cycle step / finish
// ===========================================================================

// Per-cycle handler. Called from TIMER_CALLBACK after the shot for the current
// iteration has been executed on the ADwin.
void MLOpt_Step(void){
	int ok, sret;
	char buf[128];

	// 1) Let the fitting computer finish writing its result.
	if(MLOpt.SettleDelayMs > 0) Delay((double)MLOpt.SettleDelayMs / 1000.0);

	// 2) Read the cost for the shot that just ran.
	MLOpt.CurrentCost = ml_read_cost(MLOpt.CurrentIter, &ok);
	if(!ok){ MLOpt_Finish(); return; }

	// 3) Track the best, log, and report the cost back to the optimizer.
	ml_update_best(MLOpt.CurrentCost);
	ml_append_log(MLOpt.CurrentIter, MLOpt.CurrentCost);
	ml_write_result(MLOpt.CurrentIter, MLOpt.CurrentCost);
	ml_show_best();

	sprintf(buf, "Iteration %d / %d  cost=%.6g", MLOpt.CurrentIter+1, MLOpt.TotalCalls, MLOpt.CurrentCost);
	ml_status(buf);

	// 4) Fetch the next suggestion.
	sret = ml_read_suggestion(MLOpt.CurrentIter + 1);
	if(sret == 0){ ml_status("Optimization complete (optimizer signalled DONE)."); MLOpt_Finish(); return; }
	if(sret < 0){ MLOpt_Finish(); return; }

	// Safety net in case the optimizer never sends DONE.
	if(MLOpt.CurrentIter + 1 >= MLOpt.TotalCalls && MLOpt.TotalCalls > 0){
		ml_status("Optimization complete (reached total iterations).");
		MLOpt_Finish();
		return;
	}

	// 5) Apply and run the next shot. RunOnce() re-arms the timer because the
	//    repeat toggle is on, continuing the loop on the next tick.
	MLOpt.CurrentIter++;
	ml_apply_suggestion();
	DrawNewTable(TRUE);
	ChangedVals = TRUE;
	RunOnce();
}

// Cleanly end the optimization run.
void MLOpt_Finish(void){
	// Restore the original cell values (reuse MultiScan helper).
	updateScannedCellsWithOriginalValues();
	DrawNewTable(TRUE);

	// Stop the cycle.
	SetCtrlVal(panelHandle, PANEL_TOGGLEREPEAT, 0);
	SetCtrlAttribute(panelHandle, PANEL_TIMER, ATTR_ENABLED, 0);

	MLOpt.Active = FALSE;
	MLOpt.Done   = TRUE;

	EnableScanControls();
	if(mlStartBtn) SetCtrlAttribute(mlPanel, mlStartBtn, ATTR_DIMMED, 0);
	if(mlStopBtn)  SetCtrlAttribute(mlPanel, mlStopBtn,  ATTR_DIMMED, 1);
	ml_show_best();
	ml_status("Run finished.");
}


// Read all configuration from the panel controls into MLOpt. Returns 1 on
// success, 0 if the configuration is invalid.
static int ml_read_panel_config(void){
	int j, numRows;
	double lo, hi;

	GetCtrlVal(mlPanel, mlCostPath,    MLOpt.CostPathTemplate);
	if(MLOpt.CostPathTemplate[0] == '\0'){ ml_status("Set the cost file template first."); return 0; }
	GetCtrlVal(mlPanel, mlCostFmt,     MLOpt.CostScanFmt);
	if(MLOpt.CostScanFmt[0] == '\0') strcpy(MLOpt.CostScanFmt, "%lf");
	GetCtrlVal(mlPanel, mlPythonExe,   MLOpt.PythonExe);
	GetCtrlVal(mlPanel, mlScriptPath,  MLOpt.OptimizerScript);
	GetCtrlVal(mlPanel, mlSettleDelay, &MLOpt.SettleDelayMs);
	GetCtrlVal(mlPanel, mlCostTimeout, &MLOpt.CostTimeoutMs);
	GetCtrlVal(mlPanel, mlInitPoints,  &MLOpt.InitPoints);
	GetCtrlVal(mlPanel, mlTotalCalls,  &MLOpt.TotalCalls);
	GetCtrlVal(mlPanel, mlDirection,   &MLOpt.Direction);

	// Bounds come from the parameter table (one row per parameter).
	GetNumTableRows(mlPanel, mlParamTable, &numRows);
	if(numRows < 1){ ml_status("No parameters selected. Use right-click 'Scan cell value' then Refresh."); return 0; }
	if(numRows != MLOpt.NumPars){
		ml_status("Parameter mismatch: press Refresh after changing the selection.");
		return 0;
	}
	for(j=0; j<MLOpt.NumPars; j++){
		GetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_LOWER, j+1), &lo);
		GetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_UPPER, j+1), &hi);
		if(hi <= lo){ ml_status("Each upper bound must exceed its lower bound."); return 0; }
		MLOpt.Bounds[j].Lower = lo;
		MLOpt.Bounds[j].Upper = hi;
	}
	return 1;
}


// ===========================================================================
// Callbacks
// ===========================================================================

// Pull the currently selected parameters (from the MultiScan POS/NAMES tables)
// into the ML parameter table for bound entry.
int CVICALLBACK ML_Refresh_Callback(int panel, int control, int event, void *cbd, int e1, int e2){
	int numCols, j, page, col, row, oldRows;
	char name[64];
	char cellbuf[32];

	if(event != EVENT_COMMIT) return 0;

	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &numCols);
	if(numCols < 1){ ml_status("No parameters selected. Right-click a cell -> 'Scan cell value'."); return 0; }

	// Clear existing rows.
	GetNumTableRows(mlPanel, mlParamTable, &oldRows);
	if(oldRows > 0) DeleteTableRows(mlPanel, mlParamTable, 1, oldRows);

	MLOpt.NumPars = numCols;
	InsertTableRows(mlPanel, mlParamTable, 1, numCols, VAL_USE_MASTER_CELL_TYPE);

	for(j=0; j<numCols; j++){
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,1), &page);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,2), &col);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE, MakePoint(j+1,3), &row);
		sprintf(name, "p%d", j+1);

		// Page/Col/Row are string columns so they always render reliably.
		SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_NAME, j+1), name);
		sprintf(cellbuf, "%d", page); SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_PAGE, j+1), cellbuf);
		sprintf(cellbuf, "%d", col);  SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_COL,  j+1), cellbuf);
		sprintf(cellbuf, "%d", row);  SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_ROW,  j+1), cellbuf);
		// Bounds stay numeric so Start can read them back as doubles.
		SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_LOWER, j+1), 0.0);
		SetTableCellVal(mlPanel, mlParamTable, MakePoint(MLCOL_UPPER, j+1), 1.0);
	}

	ml_status("Parameters refreshed. Enter lower/upper bounds, then Start.");
	return 0;
}

// Choose the cost-file template path.
int CVICALLBACK ML_Browse_Callback(int panel, int control, int event, void *cbd, int e1, int e2){
	char path[MAX_PATHNAME_LEN];
	if(event != EVENT_COMMIT) return 0;
	if( FileSelectPopup("", "*.*", "", "Select cost file (template)", VAL_SELECT_BUTTON, 0, 0, 1, 0, path) > 0 )
		SetCtrlVal(mlPanel, mlCostPath, path);
	return 0;
}

// Start an optimization run.
int CVICALLBACK ML_Start_Callback(int panel, int control, int event, void *cbd, int e1, int e2){
	int status, sret;

	if(event != EVENT_COMMIT) return 0;
	if(MLOpt.Active){ ml_status("A run is already active."); return 0; }

	// 1) Create the run directory / save the .seq file (reuse MultiScan setup).
	//    SetupScanFiles returns the commands subdirectory; we use it as WorkDir.
	status = SetupScanFiles(17, MLOpt.WorkDir);
	if(status != 1){ ml_status("Setup cancelled or failed."); return 0; }
	strcpy(MultiScan.CommandsFilePath, MLOpt.WorkDir); // so reused MultiScan funcs work

	// 2) Populate MultiScan.Par[] (types + original values) from the POS table.
	UpdateMultiScanValues(TRUE);
	MultiScan.Done   = FALSE;
	MultiScan.Active = FALSE; // ML drives the loop, not the MultiScan engine.
	MLOpt.NumPars    = MultiScan.NumPars; // keep in sync with the actual selection

	// 3) Read config + bounds from the panel. (ml_read_panel_config checks that the
	//    bound-table row count matches MLOpt.NumPars, i.e. that Refresh was pressed
	//    after the most recent change to the parameter selection.)
	if( !ml_read_panel_config() ) return 0;

	// 4) Hand the configuration to Python and launch the optimizer.
	ml_write_config();
	{
		char cmd[3*MAX_PATHNAME_LEN];
		sprintf(cmd, "\"%s\" \"%s\" \"%s\"", MLOpt.PythonExe, MLOpt.OptimizerScript, MLOpt.WorkDir);
		if( LaunchExecutableEx(cmd, LE_SHOWNORMAL, &mlPythonHandle) < 0 ){
			ml_status("ERROR: failed to launch Python optimizer.");
			return 0;
		}
	}

	// 5) Initialize run state and fetch the first suggestion.
	MLOpt.CurrentIter = 0;
	MLOpt.HaveBest    = 0;
	MLOpt.BestCost    = 0.0;
	MLOpt.Active      = TRUE;
	MLOpt.Done        = FALSE;

	sret = ml_read_suggestion(0);
	if(sret != 1){ ml_status("ERROR: no initial suggestion from optimizer."); MLOpt.Active = FALSE; return 0; }
	ml_apply_suggestion();

	// 6) Start cycling. RunOnce() arms the timer because the repeat toggle is on.
	SetCtrlAttribute(mlPanel, mlStartBtn, ATTR_DIMMED, 1);
	SetCtrlAttribute(mlPanel, mlStopBtn,  ATTR_DIMMED, 0);
	SetCtrlVal(panelHandle, PANEL_TOGGLEREPEAT, 1);
	SetCtrlAttribute(panelHandle, PANEL_TIMER, ATTR_ENABLED, 1);
	ChangedVals = TRUE;
	ml_status("Run started.");
	RunOnce();
	return 0;
}

// Stop the run early.
int CVICALLBACK ML_Stop_Callback(int panel, int control, int event, void *cbd, int e1, int e2){
	if(event != EVENT_COMMIT) return 0;
	if(!MLOpt.Active){ ml_status("No active run."); return 0; }
	ml_status("Stopping run...");
	MLOpt_Finish();
	return 0;
}

// Copy the log to a user-chosen location.
int CVICALLBACK ML_ExportLog_Callback(int panel, int control, int event, void *cbd, int e1, int e2){
	char src[MAX_PATHNAME_LEN], dst[MAX_PATHNAME_LEN];
	if(event != EVENT_COMMIT) return 0;
	ml_workpath("ml_log.csv", src);
	if( FileExists(src, 0) != 1 ){ ml_status("No log file to export yet."); return 0; }
	if( FileSelectPopup("", "*.csv", "", "Export ML log", VAL_SAVE_BUTTON, 0, 0, 1, 1, dst) > 0 ){
		if( CopyFile(src, dst) == 0 ) ml_status("Log exported.");
		else ml_status("ERROR: could not export log.");
	}
	return 0;
}

// Hide rather than discard on the window close box.
int CVICALLBACK ML_Panel_Callback(int panel, int event, void *cbd, int e1, int e2){
	if(event == EVENT_CLOSE) HidePanel(mlPanel);
	return 0;
}

// Menu-bar launcher.
void CVICALLBACK ML_Menu_Callback(int menuBar, int menuItem, void *callbackData, int panel){
	ShowMLOptPanel();
}


// ===========================================================================
// Panel construction + defaults
// ===========================================================================

void InitMLOptDefaults(void){
	memset(&MLOpt, 0, sizeof(MLOpt));
	MLOpt.InitPoints   = 16;
	MLOpt.TotalCalls   = 80;
	MLOpt.Direction    = MLOPT_OBJ_MAXIMIZE;
	MLOpt.SettleDelayMs = 500;
	MLOpt.CostTimeoutMs = 30000;
	strcpy(MLOpt.CostScanFmt, "%lf");
	strcpy(MLOpt.PythonExe,   "python");
	strcpy(MLOpt.OptimizerScript, "ml_optimizer.py");
	MLOpt.CostPathTemplate[0] = '\0';
}

// Helper: a labelled numeric control with integer data type.
static int ml_new_int(int top, int left, const char *label, int value){
	int c = NewCtrl(mlPanel, CTRL_NUMERIC, (char*)label, top, left);
	SetCtrlAttribute(mlPanel, c, ATTR_DATA_TYPE, VAL_INTEGER);
	SetCtrlAttribute(mlPanel, c, ATTR_CTRL_VAL, value);
	return c;
}

// Helper: a labelled string control sized for a path.
static int ml_new_str(int top, int left, const char *label, const char *value, int width){
	int c = NewCtrl(mlPanel, CTRL_STRING, (char*)label, top, left);
	SetCtrlAttribute(mlPanel, c, ATTR_MAX_ENTRY_LENGTH, MAX_PATHNAME_LEN-1);
	SetCtrlAttribute(mlPanel, c, ATTR_WIDTH, width);
	if(value) SetCtrlVal(mlPanel, c, (char*)value);
	return c;
}

void BuildMLOptPanel(void){
	int top;
	int mlMenu;

	mlPanel = NewPanel(0, "ML Optimization", 60, 60, 470, 560);
	SetPanelAttribute(mlPanel, ATTR_CLOSE_ITEM_VISIBLE, 1);
	InstallPanelCallback(mlPanel, ML_Panel_Callback, 0);

	// --- Parameter table ---
	mlParamTable = NewCtrl(mlPanel, CTRL_TABLE, "Parameters (bounds)", 30, 20);
	SetCtrlAttribute(mlPanel, mlParamTable, ATTR_WIDTH, 520);
	SetCtrlAttribute(mlPanel, mlParamTable, ATTR_HEIGHT, 150);
	InsertTableColumns(mlPanel, mlParamTable, 1, MLNUMCOLS, VAL_CELL_NUMERIC);
	// Param/Page/Col/Row are display-only string columns (always render); the two
	// bound columns stay numeric so Start reads them back as doubles.
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_NAME, ATTR_CELL_TYPE, VAL_CELL_STRING);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_PAGE, ATTR_CELL_TYPE, VAL_CELL_STRING);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_COL,  ATTR_CELL_TYPE, VAL_CELL_STRING);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_ROW,  ATTR_CELL_TYPE, VAL_CELL_STRING);

	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_NAME,  ATTR_LABEL_TEXT, "Parameter");
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_PAGE,  ATTR_LABEL_TEXT, "Page");
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_COL,   ATTR_LABEL_TEXT, "Column");
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_ROW,   ATTR_LABEL_TEXT, "Row");
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_LOWER, ATTR_LABEL_TEXT, "Lower bound");
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_UPPER, ATTR_LABEL_TEXT, "Upper bound");

	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_NAME,  ATTR_COLUMN_WIDTH, 80);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_PAGE,  ATTR_COLUMN_WIDTH, 55);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_COL,   ATTR_COLUMN_WIDTH, 60);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_ROW,   ATTR_COLUMN_WIDTH, 55);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_LOWER, ATTR_COLUMN_WIDTH, 100);
	SetTableColumnAttribute(mlPanel, mlParamTable, MLCOL_UPPER, ATTR_COLUMN_WIDTH, 100);

	// --- Cost file + parse format ---
	top = 195;
	mlCostPath   = ml_new_str(top, 20, "Cost file template (use %05d for shot #)", MLOpt.CostPathTemplate, 380);
	mlBrowseBtn  = NewCtrl(mlPanel, CTRL_SQUARE_COMMAND_BUTTON, "Browse", top, 470);
	InstallCtrlCallback(mlPanel, mlBrowseBtn, ML_Browse_Callback, 0);

	top += 35;
	mlCostFmt    = ml_new_str(top, 20, "Cost parse format", MLOpt.CostScanFmt, 120);

	// --- Numeric settings ---
	top += 40;
	mlSettleDelay = ml_new_int(top, 20,  "Settle delay (ms)",  MLOpt.SettleDelayMs);
	mlCostTimeout = ml_new_int(top, 200, "Cost timeout (ms)",  MLOpt.CostTimeoutMs);

	top += 40;
	mlInitPoints  = ml_new_int(top, 20,  "Init points (Sobol)", MLOpt.InitPoints);
	mlTotalCalls  = ml_new_int(top, 200, "Total iterations",     MLOpt.TotalCalls);

	top += 40;
	mlDirection = NewCtrl(mlPanel, CTRL_RING, "Objective", top, 20);
	InsertListItem(mlPanel, mlDirection, 0, "Maximize", MLOPT_OBJ_MAXIMIZE);
	InsertListItem(mlPanel, mlDirection, 1, "Minimize", MLOPT_OBJ_MINIMIZE);
	SetCtrlVal(mlPanel, mlDirection, MLOpt.Direction);

	// --- Python config ---
	top += 40;
	mlPythonExe  = ml_new_str(top, 20, "Python exe", MLOpt.PythonExe, 200);
	top += 35;
	mlScriptPath = ml_new_str(top, 20, "Optimizer script", MLOpt.OptimizerScript, 380);

	// --- Status / best ---
	top += 40;
	mlStatusCtrl = NewCtrl(mlPanel, CTRL_TEXT_MSG, "Status", top, 20);
	SetCtrlAttribute(mlPanel, mlStatusCtrl, ATTR_WIDTH, 520);
	SetCtrlVal(mlPanel, mlStatusCtrl, "Idle.");
	top += 25;
	mlBestCtrl   = NewCtrl(mlPanel, CTRL_TEXT_MSG, "Best", top, 20);
	SetCtrlAttribute(mlPanel, mlBestCtrl, ATTR_WIDTH, 520);
	SetCtrlVal(mlPanel, mlBestCtrl, "Best so far: (none yet)");

	// --- Buttons ---
	top += 35;
	mlRefreshBtn = NewCtrl(mlPanel, CTRL_SQUARE_COMMAND_BUTTON, "Refresh Params", top, 20);
	InstallCtrlCallback(mlPanel, mlRefreshBtn, ML_Refresh_Callback, 0);
	mlStartBtn   = NewCtrl(mlPanel, CTRL_SQUARE_COMMAND_BUTTON, "Start", top, 170);
	InstallCtrlCallback(mlPanel, mlStartBtn, ML_Start_Callback, 0);
	mlStopBtn    = NewCtrl(mlPanel, CTRL_SQUARE_COMMAND_BUTTON, "Stop", top, 270);
	InstallCtrlCallback(mlPanel, mlStopBtn, ML_Stop_Callback, 0);
	SetCtrlAttribute(mlPanel, mlStopBtn, ATTR_DIMMED, 1);
	mlExportBtn  = NewCtrl(mlPanel, CTRL_SQUARE_COMMAND_BUTTON, "Export Log", top, 370);
	InstallCtrlCallback(mlPanel, mlExportBtn, ML_ExportLog_Callback, 0);

	// --- Menu-bar launcher on the main menu ---
	mlMenu = NewMenu(menuHandle, "MLOpt", -1);
	NewMenuItem(menuHandle, mlMenu, "Open ML Optimizer", -1, 0, ML_Menu_Callback, 0);
}

void ShowMLOptPanel(void){
	if(mlPanel) DisplayPanel(mlPanel);
}
