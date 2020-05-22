#ifndef SAVELOAD_H
#define SAVELOAD_H

#include <ansi_c.h>// For FILE


void SaveSettings(int);
void LoadSettings(int);

void SaveLastGuiSettings(void);
void LoadLastSettings(int);

int SaveSequenceV17(char* save_name, int sn_length);
int LoadSequenceV17(char* load_name, int ln_length);

int checkVersionFromFile(FILE *fbuff, long fpos_eof);
long getSaveVersionFromFile(FILE *fbuff, long fpos_eof, int *majorVer, int *minorVer);
int putTimeArrayToFile(FILE *fbuff);
long getTimeArrayFromFile(FILE *fbuff, long fpos_eof);
int putAnalogTableToFile(FILE *fbuff);
long getAnalogTableFromFile(FILE *fbuff, long fpos_eof);
int putDigitalTableToFile(FILE *fbuff);
long getDigitalTableFromFile(FILE *fbuff, long fpos_eof);
int putAnalogChPropsToFile(FILE *fbuff);
long getAnalogChPropsFromFile(FILE *fbuff, long fpos_eof);
int putDigitalChPropsToFile(FILE *fbuff);
long getDigitalChPropsFromFile(FILE *fbuff, long fpos_eof);
int putDds1TableToFile(FILE *fbuff);
long getDds1TableFromFile(FILE *fbuff, long fpos_eof);
int putDds2TableToFile(FILE *fbuff);
long getDds2TableFromFile(FILE *fbuff, long fpos_eof);
int putDds3TableToFile(FILE *fbuff);
long getDds3TableFromFile(FILE *fbuff, long fpos_eof);
int putDdsGlobalsToFile(FILE *fbuff);
long getDdsGlobalsFromFile(FILE *fbuff, long fpos_eof);
int putDdsEorsToFile(FILE *fbuff);
long getDdsEorsFromFile(FILE *fbuff, long fpos_eof);
int putLaserTableToFile(FILE *fbuff);
long getLaserTableFromFile(FILE *fbuff, long fpos_eof);
int putLaserPropsToFile(FILE *fbuff);
long getLaserPropsFromFile(FILE *fbuff, long fpos_eof);
int putAnritsuTableToFile(FILE *fbuff);
long getAnritsuTableFromFile(FILE *fbuff, long fpos_eof);
int putAnritsuPropsToFile(FILE *fbuff);
long getAnritsuPropsFromFile(FILE *fbuff, long fpos_eof);
int putInfoArrayToFile(FILE *fbuff);
long getInfoArrayFromFile(FILE *fbuff, long fpos_eof);
int putPageNamesToFile(FILE *fbuff);
long getPageNamesFromFile(FILE *fbuff, long fpos_eof);
int putPageCheckboxesToFile(FILE *fbuff);
long getPageCheckboxesFromFile(FILE *fbuff, long fpos_eof);
int putUpdatePeriodToFile(FILE *fbuff);
long getUpdatePeriodFromFile(FILE *fbuff, long fpos_eof);
int putGpibDevsToFile(FILE *fbuff);
long getGpibDevsFromFile(FILE *fbuff, long fpos_eof);

void nullCharBuff(char *buff, int max_len);
int writeHeader(FILE *fbuff, char *stag, int elem_size, int num_dims, int *dims);
int writeFooter(FILE *fbuff, char *etag);
long readHeader(FILE *fbuff, char *tag, int *elem_size, int *num_dims, int *dims, const int max_dims, long fpos_eof);
long checkFooter(FILE *fbuff, char *endtag, long fpos_eof);

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