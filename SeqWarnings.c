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
// OP Shutter is often turned off for MOT diagnostics. Remember to turn it back on.
static void CheckOpShutter(void)
{
	int i = 5; // hard-coded dig channel
	//printf("DEBUG: SeqWarnings: DChName[%d].chanem = |%s|\n", i, DChName[i].chname);
	if (nameMatches(DChName[i].chname, "OP shutter"))
	{
		anyHigh = 0;
		for (col = 1; col <= NUMBEROFCOLUMNS; col++)
		{
			if (DigTableValues[col][i][2] == 1) // col, row, page
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

// K MOT is sometimes improved by adding D1 at end of CMOT.
static void checkD1BlueMOT(void)
{
	int i = 45; // hard-coded analog channel

	if (nameMatches(AChName[i].chname, "D1 AM"))
	{
		if (AnalogTable[8][i][1] == 0) // col, channel (row), page
		{
			AddWarning("WARNING: D1 AM (blue MOT) is not on in column 8 of MOT page.");
		return;
		}
	}
}

// LF imaging requires K Trap AM off and Probe AM on.
static void checkLFImagingK(void)
{
	int digChKProbe = 19;
	int anaChKTrapAM = 21;
	int page = 10;

	if (nameMatches(DChName[digChKProbe].chname, "K Probe shutter"))
	{
		anyHigh = 0;
		for (col = 1; col <= NUMBEROFCOLUMNS; col++)
		{
			if (DigTableValues[col][digChKProbe][page] == 1) // col, row, page
			{
				anyHigh = 1;
				break;
			}
		}
		if (anyHigh)
		{
			if (nameMatches(AChName[anaChKTrapAM].chname, "K TRAP AM"))
			{
				if (AnalogTable[7][anaChKTrapAM][page] > 0)
				{
					AddWarning("WARNING: K Probe shutter is on during Imaging but K Trap AM is also on during column 7.");
				}
			}
		}
	}
	return;
}

// LF imaging requires Rb trap AM and RP digital gate to be on.
static void checkLFImagingRb(void)
{
	int digChRbProbe = 3;
	int anaChRbTrapAM = 2;
	int digChRbRP = 135;
	int page = 10;

	if (nameMatches(DChName[digChRbProbe].chname, "Rb probe shutter"))
	{
		anyHigh = 0;
		for (col = 1; col <= NUMBEROFCOLUMNS; col++)
		{
			if (DigTableValues[col][digChRbProbe][page] == 1) // col, row, page
			{
				anyHigh = 1;
				break;
			}
		}
		if (anyHigh)
		{
			if (nameMatches(AChName[anaChRbTrapAM].chname, "Rb TRAP AM"))
			{
				if (AnalogTable[7][anaChRbTrapAM][page] <= 0) && (AnalogTable[7][dgChRbRP][page]==0)
				{
					AddWarning("WARNING: Rb Probe shutter is on during Imaging but Rb Trap and RP lights in column 7 should both be on.");
				}
			}
		}
	}
	return;
	return;
}

// If MOT duration <~ 15s, it was probably set short for debugging purposes.
// Declare warning to return to normal settings afterwards.
static void checkMOTDuration(void)
{
	return;
}

// --- Add new static check functions above this line ---

void RunWarningChecks(void)
{
	ClearWarnings();
	CheckOpShutter();
	checkD1BlueMOT();
	checkLFImagingK();
	checkLFImagingRb();
	// Call additional check functions here

	if (warningCount == 0)
		InsertTextBoxLine(panelHandle, warningBoxCtrl, -1, "No warnings.");
}
