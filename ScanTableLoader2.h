#ifndef SCANTABLELOADER_H
#define SCANTABLELOADER_H

#include <userint.h>
#include <ansi_c.h>
#include "vars.h"


void LoadLinearRamp(int,int,int steps,double first,double last,int iter,int STCELLNUMS);
void LoadExpRamp(int,int,int steps,double first,double last,int iter,int STCELLNUMS);
void LoadFixedToMaster(int tableHandle, int tableColumn, int masterColumn, int mode, double fixedVal);

#endif