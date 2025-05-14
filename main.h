#ifndef MAIN_H
#define MAIN_H

#include <ansi_c.h>
#include <userint.h>
#include <cvirte.h>


#include "AnalogSettings.h"
#include "AnalogSettings2.h"
#include "DigitalSettings2.h"
#include "GUIDesign.h"
#include "GUIDesign2.h"
#include "math.h"
#include "GPIB_SRS_SETUP.h"
#include "GPIB_SRS_SETUP2.h"
#include "TimingVisualizer.h"

void initializeGUI(void);

void initializeAnalogChProps(void);
void initializeAnalogTable(void);
void initializeDigChProps(void);
void initializeDigArray(void);
void initializeInfoArray(void);
void initializeLaserArrays(void);
void initializeGpibDevArray(void);
void initializeDdsTables(void);
void initializeMultiScan(void);
void minimizeConsole(void);


#endif
