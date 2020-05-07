#ifndef SAVELOAD_H
#define SAVELOAD_H



void SaveSettings(int);
void LoadSettings(int);

void SaveLastGuiSettings(void);
void LoadLastSettings(int);

void SaveArraysV16(char*,int);
int LoadArraysV16(char*,int);

void SaveArraysV15(char*,int);
void LoadArraysV15(char*,int);

void LoadArraysV13(char*,int);

void SaveLaserData(char *,int);
void LoadLaserData(char *,int);



void ExportPanel(char*,int);
void ExportConfig(char*,int);



#endif