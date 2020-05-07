#include <ansi_c.h>
#include <userint.h>
#include "Processor.h"
#include "vars.h"

int CVICALLBACK PROCESSORSWITCH_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle12, PANEL_PROCESSORSWITCH, &processorT1x);
			break;
		}
	return 0;
}



int CVICALLBACK PROCESSOR_OK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel(panelHandle12);
			break;
		}
	return 0;
}
