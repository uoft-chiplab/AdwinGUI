/*  GPIB_SRS_SETUP2.h */
	

	 
void SetGPIBPanel(void);  
void BuildGPIBString(int j, char *outstr);
void ApplyGPIBPanel(void);
void SendGPIBString(int);
int BuildSRSWaveform(short *data, double *pars, int *wavfchk); 
void SendGPIBWaveform(int, double*);
void SendVISAWaveform(ViSession, double*);
void convertRigolDataToCmd(double *data, char *voltages, int ndata, int nChars, int outputnumber);
int BuildRIGOLWaveform(char *voltages, double *pars, int *wavfchk, int nChars);
		 
