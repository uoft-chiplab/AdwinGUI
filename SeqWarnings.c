#include <ansi_c.h>
#include "GUIDesign.h"
#include "vars.h"
#include <userint.h>
#include <string.h>
#include "SeqWarnings.h"

static int warningCount = 0;

void ClearWarnings(void)
{
	warningCount = 0;
	ResetTextBox(panelHandle, warningBoxCtrl, "");
}

void AddWarning(const char *msg)
{
	warningCount++;
	InsertTextBoxLine(panelHandle, warningBoxCtrl, -1, msg);
}

// strip leading spaces, there is actually a default leading space in DChName[i].chname
// returns int for boolean comparison
static int nameMatches(const char *chname, const char *target)
{
	while (*chname == ' ') chname++;
	return strcmp(chname, target) == 0;
}

// --- Individual Checks ---

static void CheckOpShutter(void)
{
	int i, col, anyHigh;
	for (i = 1; i <= NUMBERDIGITALCHANNELS; i++)
	{
		//printf("DEBUG: SeqWarnings: DChName[%d].chanem = |%s|\n", i, DChName[i].chname);
		if (nameMatches(DChName[i].chname, "OP shutter"))
		{
			anyHigh = 0;
			for (col = 1; col <= NUMBEROFCOLUMNS; col++)
			{
				if (DigTableValues[col][i][2] == 1)
				{
					anyHigh = 1;
					break;
				}
			}
			if (!anyHigh)
				AddWarning("WARNING: OP Shutter is LOW on all cells of page 2.");
			return;
		}
	}
}

// --- Add new static check functions above this line ---

void RunWarningChecks(void)
{
	ClearWarnings();
	CheckOpShutter();
	// Call additional check functions here

	if (warningCount == 0)
		InsertTextBoxLine(panelHandle, warningBoxCtrl, -1, "No warnings.");
}
