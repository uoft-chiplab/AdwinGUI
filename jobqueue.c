// jobqueue.c — Job queue for sequential multi-parameter scans.
// Each QueueJob stores a complete snapshot of the multiscan position and value tables
// plus an output filename. Jobs execute automatically one after another.

#include "jobqueue.h"

#include <ansi_c.h>
#include <userint.h>
#include <utility.h>
#include <formatio.h>

#include "vars.h"
#include "GUIDesign.h"
#include "GUIDesign2.h"
#include "multiscan.h"
#include "saveload.h"


/***********************************************************************
 * Static control handles for the queue panel
 ***********************************************************************/

static int queueListCtrl;     // List box showing jobs
static int btnAddJob;
static int btnRemoveJob;
static int btnEditJob;
static int btnMoveUp;
static int btnMoveDown;
static int btnLoadCSV;
static int btnSaveFile;
static int btnLoadFile;
static int btnStartQueue;
static int btnStopQueue;
static int lblProgress;

// Track which job is being edited (-1 = none)
static int editingJobIndex = -1;


/***********************************************************************
 * Helper: get the selected list index (0-based), returns -1 if none
 ***********************************************************************/
static int GetSelectedJobIndex(void)
{
	int idx = -1;
	GetCtrlIndex(queuePanelHandle, queueListCtrl, &idx);
	return idx;
}


/***********************************************************************
 * Queue_RefreshPanelList
 * Repopulate the list box from JobQueue[].
 ***********************************************************************/
void Queue_RefreshPanelList(void)
{
	int i;
	char label[MAX_PATHNAME_LEN + 80];
	char basename[MAX_PATHNAME_LEN];
	char dir[MAX_PATHNAME_LEN];
	char fname[MAX_PATHNAME_LEN];
	const char *statusStr;

	ClearListCtrl(queuePanelHandle, queueListCtrl);

	for (i = 0; i < QueueLength; i++) {
		SplitPath(JobQueue[i].filename, NULL, dir, fname);
		// Remove .seq extension for display if present
		strncpy(basename, fname, MAX_PATHNAME_LEN - 1);
		basename[MAX_PATHNAME_LEN - 1] = '\0';
		if (strlen(basename) > 4 &&
		    strcmp(basename + strlen(basename) - 4, ".seq") == 0)
			basename[strlen(basename) - 4] = '\0';

		switch (JobQueue[i].status) {
			case QUEUE_STATUS_PENDING: statusStr = "Pending"; break;
			case QUEUE_STATUS_RUNNING: statusStr = "Running"; break;
			case QUEUE_STATUS_DONE:    statusStr = "Done";    break;
			case QUEUE_STATUS_ERROR:   statusStr = "Error";   break;
			default:                   statusStr = "?";       break;
		}

		sprintf(label, "%d  %-30s  %2d pars  %4d steps  %s",
		        i + 1,
		        basename,
		        JobQueue[i].numPars,
		        JobQueue[i].numSteps,
		        statusStr);

		InsertListItem(queuePanelHandle, queueListCtrl, -1, label, i);
	}

	// Update progress label
	if (QueueActive && CurrentJobIndex >= 0) {
		char progStr[64];
		sprintf(progStr, "Job %d of %d", CurrentJobIndex + 1, QueueLength);
		SetCtrlVal(queuePanelHandle, lblProgress, progStr);
	} else {
		SetCtrlVal(queuePanelHandle, lblProgress, "Queue idle");
	}
}


/***********************************************************************
 * Queue_CaptureCurrentConfig
 * Snapshot the main panel's POS_TABLE and VAL_TABLE into a new job.
 ***********************************************************************/
void Queue_CaptureCurrentConfig(void)
{
	int numPars, numRows, numCols;
	int i, j;
	double cellVal;
	char seqPath[MAX_PATHNAME_LEN];
	int fileStatus;
	struct QueueJob *job;

	if (QueueLength >= MAX_QUEUE_JOBS) {
		MessagePopup("Queue Full",
		             "Maximum number of jobs reached. Remove a job before adding.");
		return;
	}

	// Check that at least one scan parameter is configured
	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &numPars);
	if (numPars < 1) {
		MessagePopup("No Scan Parameters",
		             "No scan parameters configured. Right-click a cell and choose "
		             "'Scan cell value' to add parameters.");
		return;
	}

	// Ask for the output .seq filename
	fileStatus = FileSelectPopup("", "*.seq", "",
	                             "Select output .seq file for this job",
	                             VAL_SAVE_BUTTON, 0, 0, 1, 1, seqPath);
	if (fileStatus <= 0)
		return; // Cancelled or error

	job = &JobQueue[QueueLength];

	// Store path
	strncpy(job->filename, seqPath, MAX_PATHNAME_LEN - 1);
	job->filename[MAX_PATHNAME_LEN - 1] = '\0';
	job->csvpath[0] = '\0';
	job->numPars = numPars;
	job->status  = QUEUE_STATUS_PENDING;

	// Capture POS_TABLE (rows 1=page, 2=col, 3=row; columns 1..numPars)
	for (j = 0; j < numPars; j++) {
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 1), &job->posPage[j]);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 2), &job->posCol[j]);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 3), &job->posRow[j]);
	}

	// Capture VAL_TABLE — scan rows until sentinel or end of table
	GetNumTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numRows);

	job->numSteps = 0;
	for (i = 0; i < numRows && job->numSteps < SCANBUFFER_LENGTH; i++) {
		// Check first column for sentinel
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                MakePoint(1, i + 1), &cellVal);
		if (cellVal <= -999.0)
			break;

		for (j = 0; j < numPars; j++) {
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
			                MakePoint(j + 1, i + 1), &cellVal);
			job->values[job->numSteps][j] = cellVal;
		}
		job->numSteps++;
	}

	if (job->numSteps == 0) {
		MessagePopup("No Scan Steps",
		             "The value table has no scan steps (or starts with a sentinel). "
		             "Add values to the table before capturing.");
		return;
	}

	QueueLength++;
	Queue_RefreshPanelList();

	printf("Queue: captured job %d — %d pars, %d steps — %s\n",
	       QueueLength, job->numPars, job->numSteps, job->filename);
}


/***********************************************************************
 * Queue_LoadJobToPanel
 * Write a job's tables back into the main panel POS_TABLE and VAL_TABLE.
 ***********************************************************************/
void Queue_LoadJobToPanel(int jobIdx)
{
	struct QueueJob *job;
	int curCols, curRows;
	int i, j;
	int sentinelRow;

	if (jobIdx < 0 || jobIdx >= QueueLength)
		return;

	job = &JobQueue[jobIdx];

	// --- POS_TABLE: ensure it has exactly job->numPars columns ---
	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &curCols);

	if (curCols > job->numPars) {
		// Delete extra columns from the right
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                   job->numPars + 1, curCols - job->numPars);
	} else if (curCols < job->numPars) {
		// Insert columns at the end
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                   -1, job->numPars - curCols, VAL_CELL_NUMERIC);
	}

	// --- Sync VAL_TABLE column count ---
	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &curCols);
	if (curCols > job->numPars) {
		DeleteTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                   job->numPars + 1, curCols - job->numPars);
	} else if (curCols < job->numPars) {
		InsertTableColumns(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                   -1, job->numPars - curCols, VAL_CELL_NUMERIC);
	}

	// Write POS_TABLE values (3 rows: page / col / row)
	for (j = 0; j < job->numPars; j++) {
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 1), job->posPage[j]);
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 2), job->posCol[j]);
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 3), job->posRow[j]);
	}

	// --- VAL_TABLE: ensure enough rows for steps + 1 sentinel ---
	GetNumTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &curRows);
	sentinelRow = job->numSteps + 1; // 1-based index of sentinel row

	if (curRows < sentinelRow) {
		InsertTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                -1, sentinelRow - curRows, VAL_CELL_NUMERIC);
	}

	// Write step values
	for (i = 0; i < job->numSteps; i++) {
		for (j = 0; j < job->numPars; j++) {
			SetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
			                MakePoint(j + 1, i + 1), job->values[i][j]);
		}
	}

	// Write sentinel in the row after last step (column 1)
	SetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
	                MakePoint(1, sentinelRow), STOP_REPLACE_ORIGINAL_SENTINEL);
	// Clear remaining columns in sentinel row to avoid stale data
	for (j = 1; j < job->numPars; j++) {
		SetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                MakePoint(j + 1, sentinelRow), 0.0);
	}

	printf("Queue: loaded job %d to panel (%d pars, %d steps)\n",
	       jobIdx, job->numPars, job->numSteps);
}


/***********************************************************************
 * Queue_LoadCSV
 * Load scan values from a CSV file into JobQueue[jobIdx].values.
 * Returns 0 on success, -1 on dimension mismatch or other error.
 ***********************************************************************/
int Queue_LoadCSV(int jobIdx, const char *csvpath)
{
	FILE *fp;
	struct QueueJob *job;
	char linebuf[8192];
	char *tok;
	int headerCols, row, col;
	double val;

	if (jobIdx < 0 || jobIdx >= QueueLength)
		return -1;

	job = &JobQueue[jobIdx];

	if (job->numPars < 1) {
		MessagePopup("CSV Load Error",
		             "Configure scan parameters before loading a CSV file.");
		return -1;
	}

	fp = fopen(csvpath, "r");
	if (fp == NULL) {
		MessagePopup("CSV Load Error", "Could not open the selected CSV file.");
		return -1;
	}

	// Count columns in the header row
	if (fgets(linebuf, sizeof(linebuf), fp) == NULL) {
		MessagePopup("CSV Load Error", "CSV file appears to be empty.");
		fclose(fp);
		return -1;
	}

	headerCols = 0;
	tok = strtok(linebuf, ",\t\r\n");
	while (tok != NULL) {
		headerCols++;
		tok = strtok(NULL, ",\t\r\n");
	}

	if (headerCols != job->numPars) {
		char errMsg[256];
		sprintf(errMsg,
		        "CSV has %d column(s) but the job has %d scan parameter(s).\n"
		        "Expected one column per scanned parameter.",
		        headerCols, job->numPars);
		MessagePopup("CSV Dimension Error", errMsg);
		fclose(fp);
		return -1;
	}

	// Read data rows
	row = 0;
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL &&
	       row < SCANBUFFER_LENGTH) {
		col = 0;
		tok = strtok(linebuf, ",\t\r\n");
		while (tok != NULL && col < job->numPars) {
			val = atof(tok);
			job->values[row][col] = val;
			col++;
			tok = strtok(NULL, ",\t\r\n");
		}
		if (col > 0)
			row++;
	}
	fclose(fp);

	job->numSteps = row;
	strncpy(job->csvpath, csvpath, MAX_PATHNAME_LEN - 1);
	job->csvpath[MAX_PATHNAME_LEN - 1] = '\0';

	printf("Queue: loaded CSV into job %d — %d steps\n", jobIdx, job->numSteps);
	Queue_RefreshPanelList();
	return 0;
}


/***********************************************************************
 * Queue_SaveToFile
 * Write queue configuration to a plain-text .queue file.
 * Format: structured sections, one value row per line.
 * This avoids very long lines (up to 3000 steps × 70 pars each).
 ***********************************************************************/
void Queue_SaveToFile(const char *path)
{
	FILE *fp;
	int i, j, k;
	struct QueueJob *job;

	fp = fopen(path, "w");
	if (fp == NULL) {
		MessagePopup("Save Error", "Could not open file for writing.");
		return;
	}

	fprintf(fp, "JOBQUEUE_V2\n");
	fprintf(fp, "NumJobs=%d\n", QueueLength);

	for (i = 0; i < QueueLength; i++) {
		job = &JobQueue[i];

		fprintf(fp, "JOB_START\n");
		fprintf(fp, "filename=%s\n", job->filename);
		fprintf(fp, "csvpath=%s\n",  job->csvpath);
		fprintf(fp, "numPars=%d\n",  job->numPars);
		fprintf(fp, "numSteps=%d\n", job->numSteps);

		// posPage, posCol, posRow as comma-separated on one line each
		fprintf(fp, "posPage=");
		for (j = 0; j < job->numPars; j++)
			fprintf(fp, "%d%s", job->posPage[j], (j < job->numPars - 1) ? "," : "");
		fprintf(fp, "\n");

		fprintf(fp, "posCol=");
		for (j = 0; j < job->numPars; j++)
			fprintf(fp, "%d%s", job->posCol[j], (j < job->numPars - 1) ? "," : "");
		fprintf(fp, "\n");

		fprintf(fp, "posRow=");
		for (j = 0; j < job->numPars; j++)
			fprintf(fp, "%d%s", job->posRow[j], (j < job->numPars - 1) ? "," : "");
		fprintf(fp, "\n");

		// One value row per line: v[k][0],v[k][1],...
		fprintf(fp, "VALUES_START\n");
		for (k = 0; k < job->numSteps; k++) {
			for (j = 0; j < job->numPars; j++) {
				fprintf(fp, "%.10g", job->values[k][j]);
				if (j < job->numPars - 1)
					fprintf(fp, ",");
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "VALUES_END\n");
		fprintf(fp, "JOB_END\n");
	}

	fclose(fp);
	printf("Queue: saved %d job(s) to %s\n", QueueLength, path);
}


/***********************************************************************
 * Queue_LoadFromFile
 * Read queue configuration from a .queue file.
 * Returns 0 on success, -1 on error.
 ***********************************************************************/
int Queue_LoadFromFile(const char *path)
{
	FILE *fp;
	char linebuf[4096]; // one step row per line: 70 doubles × ~18 chars = ~1260 bytes
	char *tok, *p;
	int numJobs, i, j, k;
	struct QueueJob *job;
	int inValues;

	fp = fopen(path, "r");
	if (fp == NULL) {
		MessagePopup("Load Error", "Could not open the selected .queue file.");
		return -1;
	}

	// Version header
	if (fgets(linebuf, sizeof(linebuf), fp) == NULL ||
	    strncmp(linebuf, "JOBQUEUE_V2", 11) != 0) {
		MessagePopup("Load Error",
		             "File does not appear to be a valid .queue file (expected V2).");
		fclose(fp);
		return -1;
	}

	// NumJobs line
	if (fgets(linebuf, sizeof(linebuf), fp) == NULL) {
		fclose(fp);
		return -1;
	}
	tok = strchr(linebuf, '=');
	numJobs = tok ? atoi(tok + 1) : 0;
	if (numJobs < 0 || numJobs > MAX_QUEUE_JOBS) {
		MessagePopup("Load Error", "Invalid job count in .queue file.");
		fclose(fp);
		return -1;
	}

	QueueLength = 0;
	CurrentJobIndex = -1;
	QueueActive = FALSE;

	for (i = 0; i < numJobs && i < MAX_QUEUE_JOBS; i++) {
		job = &JobQueue[i];
		memset(job, 0, sizeof(struct QueueJob));
		job->status = QUEUE_STATUS_PENDING;
		inValues = 0;
		k = 0; // current step row being read

		// Read lines until JOB_END
		while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
			// Strip trailing newline
			int len = strlen(linebuf);
			while (len > 0 && (linebuf[len-1] == '\n' || linebuf[len-1] == '\r'))
				linebuf[--len] = '\0';

			if (strcmp(linebuf, "JOB_START") == 0) {
				continue;
			} else if (strcmp(linebuf, "JOB_END") == 0) {
				break;
			} else if (strcmp(linebuf, "VALUES_START") == 0) {
				inValues = 1;
				k = 0;
			} else if (strcmp(linebuf, "VALUES_END") == 0) {
				inValues = 0;
			} else if (inValues) {
				// One step row: v0,v1,...
				if (k < SCANBUFFER_LENGTH) {
					p = strtok(linebuf, ",");
					for (j = 0; j < job->numPars && p; j++, p = strtok(NULL, ","))
						job->values[k][j] = atof(p);
					k++;
				}
			} else {
				// Key=value lines
				tok = strchr(linebuf, '=');
				if (tok == NULL) continue;
				*tok = '\0';
				char *key = linebuf;
				char *val = tok + 1;

				if (strcmp(key, "filename") == 0) {
					strncpy(job->filename, val, MAX_PATHNAME_LEN - 1);
				} else if (strcmp(key, "csvpath") == 0) {
					strncpy(job->csvpath, val, MAX_PATHNAME_LEN - 1);
				} else if (strcmp(key, "numPars") == 0) {
					job->numPars = atoi(val);
				} else if (strcmp(key, "numSteps") == 0) {
					job->numSteps = atoi(val);
				} else if (strcmp(key, "posPage") == 0) {
					p = strtok(val, ",");
					for (j = 0; j < job->numPars && p; j++, p = strtok(NULL, ","))
						job->posPage[j] = atoi(p);
				} else if (strcmp(key, "posCol") == 0) {
					p = strtok(val, ",");
					for (j = 0; j < job->numPars && p; j++, p = strtok(NULL, ","))
						job->posCol[j] = atoi(p);
				} else if (strcmp(key, "posRow") == 0) {
					p = strtok(val, ",");
					for (j = 0; j < job->numPars && p; j++, p = strtok(NULL, ","))
						job->posRow[j] = atoi(p);
				}
			}
		}

		QueueLength++;
	}

	fclose(fp);
	Queue_RefreshPanelList();
	printf("Queue: loaded %d job(s) from %s\n", QueueLength, path);
	return 0;
}


/***********************************************************************
 * Queue_Start
 * Begin executing the queue from the first Pending job.
 ***********************************************************************/
void Queue_Start(void)
{
	int i, status;

	if (QueueLength == 0) {
		MessagePopup("Queue Empty", "Add jobs to the queue before starting.");
		return;
	}

	if (QueueActive) {
		MessagePopup("Queue Running", "Queue is already running.");
		return;
	}

	// Find first Pending job
	for (i = 0; i < QueueLength; i++) {
		if (JobQueue[i].status == QUEUE_STATUS_PENDING)
			break;
	}
	if (i == QueueLength) {
		MessagePopup("No Pending Jobs",
		             "All jobs are already Done. Reset job statuses or add new jobs.");
		return;
	}

	// Load job tables into the main panel
	Queue_LoadJobToPanel(i);

	// Set up directories and save the .seq file (non-interactive)
	status = SetupScanFilesAuto(JobQueue[i].filename, MultiScan.CommandsFilePath);
	if (status != 1) {
		MessagePopup("Queue Start Error",
		             "Failed to set up scan files for the first job. Check the filename.");
		return;
	}

	// Initialize scan engine (mirrors CMD_SCAN_CALLBACK)
	CurrentJobIndex = i;
	JobQueue[i].status = QUEUE_STATUS_RUNNING;
	QueueActive = TRUE;

	MultiScan.NextCommandsFileNumber = 0;
	MultiScan.CurrentStep = 0;
	GetNewMultiScanCommands();
	UpdateMultiScanValues(TRUE);
	SetCtrlVal(panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
	writeToScanInfoFile();

	SaveLastGuiSettings();
	ChangedVals = TRUE;
	SetCtrlVal(panelHandle, PANEL_TOGGLEREPEAT, TRUE);
	SetCtrlAttribute(panelHandle, PANEL_TIMER, ATTR_ENABLED, 1);
	RunOnce();

	Queue_RefreshPanelList();
	printf("Queue: started — running job %d of %d\n", i + 1, QueueLength);
}


/***********************************************************************
 * Queue_Stop
 * Halt queue execution. The current scan iteration finishes normally.
 ***********************************************************************/
void Queue_Stop(void)
{
	QueueActive = FALSE;
	MultiScan.Active = FALSE;
	SetCtrlVal(panelHandle, PANEL_TOGGLEREPEAT, FALSE);
	SetCtrlAttribute(panelHandle, PANEL_TIMER, ATTR_ENABLED, 0);
	EnableScanControls();
	SetCtrlVal(panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
	Queue_RefreshPanelList();
	printf("Queue: stopped by user\n");
}


/***********************************************************************
 * Queue_AdvanceToNextJob
 * Called after scan completion when QueueActive == TRUE.
 * Marks the current job Done, then loads and starts the next Pending job.
 * If none remain, declares the queue complete.
 ***********************************************************************/
void Queue_AdvanceToNextJob(void)
{
	int i, status;

	// Mark the just-finished job as Done
	if (CurrentJobIndex >= 0 && CurrentJobIndex < QueueLength)
		JobQueue[CurrentJobIndex].status = QUEUE_STATUS_DONE;

	// Find the next Pending job
	for (i = CurrentJobIndex + 1; i < QueueLength; i++) {
		if (JobQueue[i].status == QUEUE_STATUS_PENDING)
			break;
	}

	if (i >= QueueLength) {
		// No more pending jobs — queue complete
		QueueActive = FALSE;
		CurrentJobIndex = -1;
		EnableScanControls();
		SetCtrlVal(panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
		SetCtrlVal(panelHandle, PANEL_MULTISCAN_LED2, MultiScan.Done);
		SetCtrlAttribute(panelHandle, PANEL_MULTISCAN_DECORATION,
		                 ATTR_FRAME_COLOR, VAL_TRANSPARENT);
		DrawNewTable(TRUE);
		Queue_RefreshPanelList();
		MessagePopup("Queue Complete",
		             "All queued jobs have finished. Check output directories for results.");
		printf("Queue: complete — all jobs done\n");
		return;
	}

	// Load next job into the main panel tables
	Queue_LoadJobToPanel(i);

	// Set up directories and save .seq (non-interactive)
	status = SetupScanFilesAuto(JobQueue[i].filename, MultiScan.CommandsFilePath);
	if (status != 1) {
		// Error setting up scan files — stop the queue
		QueueActive = FALSE;
		JobQueue[i].status = QUEUE_STATUS_ERROR;
		EnableScanControls();
		Queue_RefreshPanelList();
		char errMsg[MAX_PATHNAME_LEN + 80];
		sprintf(errMsg, "Failed to set up scan files for job %d:\n%s\n\nQueue halted.",
		        i + 1, JobQueue[i].filename);
		MessagePopup("Queue Error", errMsg);
		return;
	}

	// Advance state
	CurrentJobIndex = i;
	JobQueue[i].status = QUEUE_STATUS_RUNNING;

	// Initialize and start the next scan (mirrors CMD_SCAN_CALLBACK)
	MultiScan.NextCommandsFileNumber = 0;
	MultiScan.CurrentStep = 0;
	GetNewMultiScanCommands();
	UpdateMultiScanValues(TRUE);
	SetCtrlVal(panelHandle, PANEL_MULTISCAN_LED1, MultiScan.Active);
	writeToScanInfoFile();

	SetCtrlVal(panelHandle, PANEL_TOGGLEREPEAT, TRUE);
	SetCtrlAttribute(panelHandle, PANEL_TIMER, ATTR_ENABLED, 1);
	RunOnce();

	Queue_RefreshPanelList();
	printf("Queue: advanced to job %d of %d\n", i + 1, QueueLength);
}


/***********************************************************************
 * Button Callbacks
 ***********************************************************************/

int CVICALLBACK Queue_CB_AddJob(int panel, int control, int event,
                                void *cd, int e1, int e2)
{
	if (event == EVENT_COMMIT)
		Queue_CaptureCurrentConfig();
	return 0;
}

int CVICALLBACK Queue_CB_RemoveJob(int panel, int control, int event,
                                   void *cd, int e1, int e2)
{
	int idx, i;
	if (event != EVENT_COMMIT)
		return 0;

	idx = GetSelectedJobIndex();
	if (idx < 0 || idx >= QueueLength) {
		MessagePopup("No Selection", "Select a job from the list first.");
		return 0;
	}
	if (QueueActive && idx == CurrentJobIndex) {
		MessagePopup("Job Running", "Cannot remove the currently running job.");
		return 0;
	}

	// Shift jobs down
	for (i = idx; i < QueueLength - 1; i++)
		JobQueue[i] = JobQueue[i + 1];
	QueueLength--;

	if (CurrentJobIndex > idx)
		CurrentJobIndex--;
	else if (CurrentJobIndex == idx)
		CurrentJobIndex = -1;

	Queue_RefreshPanelList();
	return 0;
}

int CVICALLBACK Queue_CB_EditJob(int panel, int control, int event,
                                  void *cd, int e1, int e2)
{
	int idx;
	if (event != EVENT_COMMIT)
		return 0;

	idx = GetSelectedJobIndex();
	if (idx < 0 || idx >= QueueLength) {
		MessagePopup("No Selection", "Select a job from the list first.");
		return 0;
	}

	Queue_LoadJobToPanel(idx);
	editingJobIndex = idx;

	char msg[128];
	sprintf(msg,
	        "Job %d has been loaded into the scan tables.\n"
	        "Edit the POS_TABLE and VAL_TABLE on the main panel,\n"
	        "then click 'Update Job' to save your changes.",
	        idx + 1);
	MessagePopup("Edit Job", msg);
	return 0;
}

// "Update Job" — recapture the currently-being-edited job from the main panel tables
static int updateJobCtrl = -1; // handle set in initializeQueuePanel
int CVICALLBACK Queue_CB_UpdateJob(int panel, int control, int event,
                                    void *cd, int e1, int e2)
{
	int numPars, numRows, i, j;
	double cellVal;
	struct QueueJob *job;

	if (event != EVENT_COMMIT)
		return 0;

	if (editingJobIndex < 0 || editingJobIndex >= QueueLength) {
		MessagePopup("Not Editing",
		             "No job is currently loaded for editing. Use 'Edit Job' first.");
		return 0;
	}

	job = &JobQueue[editingJobIndex];

	GetNumTableColumns(panelHandle, PANEL_MULTISCAN_POS_TABLE, &numPars);
	if (numPars < 1) {
		MessagePopup("No Parameters", "No scan parameters in the table.");
		return 0;
	}

	job->numPars = numPars;
	for (j = 0; j < numPars; j++) {
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 1), &job->posPage[j]);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 2), &job->posCol[j]);
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_POS_TABLE,
		                MakePoint(j + 1, 3), &job->posRow[j]);
	}

	GetNumTableRows(panelHandle, PANEL_MULTISCAN_VAL_TABLE, &numRows);
	job->numSteps = 0;
	for (i = 0; i < numRows && job->numSteps < SCANBUFFER_LENGTH; i++) {
		GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
		                MakePoint(1, i + 1), &cellVal);
		if (cellVal <= -999.0)
			break;
		for (j = 0; j < numPars; j++) {
			GetTableCellVal(panelHandle, PANEL_MULTISCAN_VAL_TABLE,
			                MakePoint(j + 1, i + 1), &cellVal);
			job->values[job->numSteps][j] = cellVal;
		}
		job->numSteps++;
	}

	{
		int doneIdx = editingJobIndex;
		editingJobIndex = -1;
		Queue_RefreshPanelList();
		char msg[80];
		sprintf(msg, "Job %d updated (%d pars, %d steps).",
		        doneIdx + 1, numPars, job->numSteps);
		MessagePopup("Job Updated", msg);
	}
	return 0;
}

int CVICALLBACK Queue_CB_MoveUp(int panel, int control, int event,
                                 void *cd, int e1, int e2)
{
	int idx;
	struct QueueJob tmp;

	if (event != EVENT_COMMIT)
		return 0;

	idx = GetSelectedJobIndex();
	if (idx <= 0 || idx >= QueueLength)
		return 0;

	tmp = JobQueue[idx - 1];
	JobQueue[idx - 1] = JobQueue[idx];
	JobQueue[idx] = tmp;

	if (CurrentJobIndex == idx)
		CurrentJobIndex = idx - 1;
	else if (CurrentJobIndex == idx - 1)
		CurrentJobIndex = idx;

	Queue_RefreshPanelList();
	SetCtrlIndex(queuePanelHandle, queueListCtrl, idx - 1);
	return 0;
}

int CVICALLBACK Queue_CB_MoveDown(int panel, int control, int event,
                                   void *cd, int e1, int e2)
{
	int idx;
	struct QueueJob tmp;

	if (event != EVENT_COMMIT)
		return 0;

	idx = GetSelectedJobIndex();
	if (idx < 0 || idx >= QueueLength - 1)
		return 0;

	tmp = JobQueue[idx + 1];
	JobQueue[idx + 1] = JobQueue[idx];
	JobQueue[idx] = tmp;

	if (CurrentJobIndex == idx)
		CurrentJobIndex = idx + 1;
	else if (CurrentJobIndex == idx + 1)
		CurrentJobIndex = idx;

	Queue_RefreshPanelList();
	SetCtrlIndex(queuePanelHandle, queueListCtrl, idx + 1);
	return 0;
}

int CVICALLBACK Queue_CB_LoadCSV(int panel, int control, int event,
                                  void *cd, int e1, int e2)
{
	int idx, status;
	char csvPath[MAX_PATHNAME_LEN];

	if (event != EVENT_COMMIT)
		return 0;

	idx = GetSelectedJobIndex();
	if (idx < 0 || idx >= QueueLength) {
		MessagePopup("No Selection", "Select a job from the list first.");
		return 0;
	}

	status = FileSelectPopup("", "*.csv", "", "Load CSV into Job",
	                         VAL_LOAD_BUTTON, 0, 0, 1, 0, csvPath);
	if (status <= 0)
		return 0;

	Queue_LoadCSV(idx, csvPath);
	return 0;
}

int CVICALLBACK Queue_CB_SaveFile(int panel, int control, int event,
                                   void *cd, int e1, int e2)
{
	int status;
	char queuePath[MAX_PATHNAME_LEN];

	if (event != EVENT_COMMIT)
		return 0;

	status = FileSelectPopup("", "*.queue", "", "Save Queue File",
	                         VAL_SAVE_BUTTON, 0, 0, 1, 1, queuePath);
	if (status <= 0)
		return 0;

	Queue_SaveToFile(queuePath);
	return 0;
}

int CVICALLBACK Queue_CB_LoadFile(int panel, int control, int event,
                                   void *cd, int e1, int e2)
{
	int status;
	char queuePath[MAX_PATHNAME_LEN];

	if (event != EVENT_COMMIT)
		return 0;

	if (QueueActive) {
		MessagePopup("Queue Running",
		             "Stop the queue before loading a new queue file.");
		return 0;
	}

	status = FileSelectPopup("", "*.queue", "", "Load Queue File",
	                         VAL_LOAD_BUTTON, 0, 0, 1, 0, queuePath);
	if (status <= 0)
		return 0;

	Queue_LoadFromFile(queuePath);
	return 0;
}

int CVICALLBACK Queue_CB_StartQueue(int panel, int control, int event,
                                     void *cd, int e1, int e2)
{
	if (event == EVENT_COMMIT)
		Queue_Start();
	return 0;
}

int CVICALLBACK Queue_CB_StopQueue(int panel, int control, int event,
                                    void *cd, int e1, int e2)
{
	if (event == EVENT_COMMIT)
		Queue_Stop();
	return 0;
}


/***********************************************************************
 * initializeQueuePanel
 * Build the queue panel at runtime using NewPanel / NewCtrl.
 * Call this from initializeGUI() in main.c.
 ***********************************************************************/
void initializeQueuePanel(void)
{
	int panW = 580, panH = 460;
	int btnH = 25, btnW = 110, gap = 6;
	int x, y;

	// Create the top-level panel (child of desktop: parentPanel = 0)
	queuePanelHandle = NewPanel(0, "Job Queue", 120, 120, panH, panW);

	// --- Job list (list box) ---
	queueListCtrl = NewCtrl(queuePanelHandle, CTRL_LIST_BOX, "Jobs", 10, 5);
	SetCtrlAttribute(queuePanelHandle, queueListCtrl, ATTR_WIDTH, panW - 15);
	SetCtrlAttribute(queuePanelHandle, queueListCtrl, ATTR_HEIGHT, 185);

	// --- Button rows ---
	y = 205; x = 5;

	btnAddJob = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                    "Add Current Config", y, x);
	SetCtrlAttribute(queuePanelHandle, btnAddJob, ATTR_WIDTH, 130);
	x += 130 + gap;

	btnRemoveJob = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                       "Remove Job", y, x);
	SetCtrlAttribute(queuePanelHandle, btnRemoveJob, ATTR_WIDTH, 90);
	x += 90 + gap;

	btnEditJob = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                     "Edit Job", y, x);
	SetCtrlAttribute(queuePanelHandle, btnEditJob, ATTR_WIDTH, 75);
	x += 75 + gap;

	updateJobCtrl = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                        "Update Job", y, x);
	SetCtrlAttribute(queuePanelHandle, updateJobCtrl, ATTR_WIDTH, 85);
	x += 85 + gap;

	btnMoveUp = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                    "Up", y, x);
	SetCtrlAttribute(queuePanelHandle, btnMoveUp, ATTR_WIDTH, 40);
	x += 40 + gap;

	btnMoveDown = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                      "Down", y, x);
	SetCtrlAttribute(queuePanelHandle, btnMoveDown, ATTR_WIDTH, 40);

	// Row 2
	y += btnH + gap;  x = 5;

	btnLoadCSV = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                     "Load CSV into Selected Job", y, x);
	SetCtrlAttribute(queuePanelHandle, btnLoadCSV, ATTR_WIDTH, 190);

	// Row 3
	y += btnH + gap;  x = 5;

	btnSaveFile = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                      "Save Queue...", y, x);
	SetCtrlAttribute(queuePanelHandle, btnSaveFile, ATTR_WIDTH, 110);
	x += 110 + gap;

	btnLoadFile = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                      "Load Queue...", y, x);
	SetCtrlAttribute(queuePanelHandle, btnLoadFile, ATTR_WIDTH, 110);

	// Separator decoration
	y += btnH + 10;
	NewCtrl(queuePanelHandle, CTRL_SEPARATOR, "", y, 5);

	// Row 4 — run controls
	y += 12;  x = 5;

	btnStartQueue = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                        "▶  Start Queue", y, x);
	SetCtrlAttribute(queuePanelHandle, btnStartQueue, ATTR_WIDTH, 120);
	x += 120 + gap;

	btnStopQueue = NewCtrl(queuePanelHandle, CTRL_COMMAND_BUTTON,
	                       "■  Stop", y, x);
	SetCtrlAttribute(queuePanelHandle, btnStopQueue, ATTR_WIDTH, 75);
	x += 75 + gap * 3;

	lblProgress = NewCtrl(queuePanelHandle, CTRL_TEXT_MSG, "", y, x);
	SetCtrlAttribute(queuePanelHandle, lblProgress, ATTR_WIDTH, 200);
	SetCtrlVal(queuePanelHandle, lblProgress, "Queue idle");

	// Register callbacks
	InstallCtrlCallback(queuePanelHandle, btnAddJob,     Queue_CB_AddJob,     NULL);
	InstallCtrlCallback(queuePanelHandle, btnRemoveJob,  Queue_CB_RemoveJob,  NULL);
	InstallCtrlCallback(queuePanelHandle, btnEditJob,    Queue_CB_EditJob,    NULL);
	InstallCtrlCallback(queuePanelHandle, updateJobCtrl, Queue_CB_UpdateJob,  NULL);
	InstallCtrlCallback(queuePanelHandle, btnMoveUp,     Queue_CB_MoveUp,     NULL);
	InstallCtrlCallback(queuePanelHandle, btnMoveDown,   Queue_CB_MoveDown,   NULL);
	InstallCtrlCallback(queuePanelHandle, btnLoadCSV,    Queue_CB_LoadCSV,    NULL);
	InstallCtrlCallback(queuePanelHandle, btnSaveFile,   Queue_CB_SaveFile,   NULL);
	InstallCtrlCallback(queuePanelHandle, btnLoadFile,   Queue_CB_LoadFile,   NULL);
	InstallCtrlCallback(queuePanelHandle, btnStartQueue, Queue_CB_StartQueue, NULL);
	InstallCtrlCallback(queuePanelHandle, btnStopQueue,  Queue_CB_StopQueue,  NULL);

	DisplayPanel(queuePanelHandle);
}
