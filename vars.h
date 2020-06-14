#ifndef VAR_DEFS          // Make sure this file is included only once
#define VAR_DEFS

#include <visatype.h>
#include <userint.h>// For MAX_PATHNAME_LEN



/***********************************************************************
Typedef Declarations
*************************************************************************/

#define TRUE (1)
#define FALSE (0)

typedef int BOOL;


/************************************************************************
ADwin Variables and GUI things
*************************************************************************/

#define SEQUENCER_VERSION "ADwin Sequencer V16.4.7 - "

// ADwin info
#define DefaultEventPeriod (0.100)   // in milliseconds
int processorT1x;
double AdwinTick;


// Interfacing with the ADwin
double EventPeriod;  //The Update Period Defined by the pull down menu (in ms)
int processnum;

// Mark and Interval vars for Synchronous Scan and Repeat wait periods
static double beginProcMark;
static double CycleTime;			// Min time allocated for sequence calculations and execution


// User interface stuff
#define LASTGUISAVELOCATION "c:\\LastGui.pan"
#define MAXERRS (10)// Size of sequencer error list


// Error reporting
static char* SeqErrorBuffer[MAXERRS];	   // Holds error messages
static int SeqErrorCount;
static char procbuff[1];



/************************************************************************
Analog and Digital Channels (and laser table and necessary parts of dds and anritsu)
*************************************************************************/

#define NUMBEROFCOLUMNS (17)		//
#define NUMBEROFPAGES (11)			//currently hardwired up to 10
									// to be quick & dirty about it, just change
									//numberofpages to 1 more than actual (WTF???)


// Channel stuff
#define NUMBERANALOGCHANNELS (48)   // Number of analog Channels available for control
#define NUMBERDIGITALCHANNELS (48) 	// number of digital channels DISPLAYED!!!
									// some are not user controlled, e.g. DDS lines
									// 32 in total.  5 used for DDS1
									// 5 for DD2 (K40 evap)
									// reserved for DDS1
									// reserved for DDS2
									// reserved for DDS3				  NU
#define NUMBERDDS (3)				// Number of DDS's
#define DDS2_CLOCK (983.04)			// clock speed of DDS 2 in MHz
#define DDS3CLOCK (300.0)			// clock speed of DDS 2 in MHz
#define NUMBERLASERS (4)			// Number of Lasers
#define NUMBEROFANRITSU (1) 	 	// Number of microwave generators (on the off chance we get more than one)

//Explicitly make extra space in analog and digital arrays
#define MAXANALOG (50)				// Need 40 lines, leave room for 48
#define MAXDIGITAL (70)				// need 64 lines, leave some leeway

//total number of different analog channels (Adwin and otherwise) that we care about
#define NUMBERANALOGROWS NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS+2*NUMBEROFANRITSU


// number of columns needed in the meta-tables (with some overhead)
// reducing it to (NUMBEROFPAGES+1)*(NUMBEROFCOLUMNS+1) leads to a
// screwed-up update list.
#define NUMBEROFMETACOLUMNS (500)


// The array with the time values
double TimeArray[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];

// The array with the column descriptions
struct InfoArrayValues{
	int index;
	double value;
	char text[32];
};
struct InfoArrayValues InfoArray[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];



// Table cell structs
struct AnalogTableValues{
	int		fcn;		//fcn is an integer refering to a function to use.
						// 1-step, 2-linear, 3- exp, 4- 'S' curve 5-sine 6-"same as last cell"
	double 	fval;		//the final value
	double	tscale;		//the timescale to approach final value
	};

struct AnalogTableValues AnalogTable[NUMBEROFCOLUMNS+1][NUMBERANALOGROWS+1][NUMBEROFPAGES]; //+1 needed because all code done assumed base 1 arrays...
	// the structure is the values/elements contained at each point in the
	// analog panel.  The array aval, is set up as [x][y][page]

int DigTableValues[NUMBEROFCOLUMNS+1][MAXDIGITAL][NUMBEROFPAGES];

// Channel properties structs
struct AnalogChannelProperties{
	int		chnum;		// channel number 1-8 DAC1	9-16 DAC2
	char    chname[50]; // name to appear on the panel
	char	units[50];
	double  tfcn;		// Transfer function.  i.e. 10V out = t G/cm etc...
	double  tbias;
	int		resettozero;
	double  maxvolts;
	double  minvolts;
	}  AChName[MAXANALOG+NUMBERDDS];

struct DigitalChannelProperties{
	int		chnum;		// digital line to control
	char 	chname[50];	// name of the channel on the panel
	int 	resettolow;
	}	DChName[MAXDIGITAL];


int ChMap[MAXANALOG];	// The channel mapping (for analog). i.e. if we program line 1 as channel
				// 12, the ChMap[12]=1



/************************************************************************
Color definitions for panel display
*************************************************************************/
#define CLR_ANALOG_STEP		0xFF3366L
#define CLR_ANALOG_LINEAR 	0x33FF66L
#define CLR_ANALOG_EXP 		0x33CCFFL//VAL_BLUE
#define CLR_ANALOG_MINJERK 	0xCC33FFL
#define CLR_ANALOG_SINEWAVE VAL_CYAN
#define CLR_ANALOG_COPYPREV 0xFF99CCL

#define CLR_DIGITAL_HI 		0xFF3366L
#define CLR_DIGITAL_LO 		VAL_GRAY
#define CLR_DIGITAL_LO_ALT 	0x00B0B0B0

#define CLR_DDS_RAMPDOWN 	0x33CCFFL
#define CLR_DDS_RAMPUP 		0x33FF99L
#define CLR_DDS_HOLD 		0x99FFFFL

#define CLR_LASER_STEP		0x33FFCCL
#define CLR_LASER_RAMPDOWN 	0x33CCFFL
#define CLR_LASER_RAMPUP 	0x33FF66L
#define CLR_LASER_HOLD 		0x99FFFFL

#define CLR_SCAN_TIME	 	0xFFAA00L
#define CLR_SCAN_ANALOG 	0xFFAA00L
#define CLR_SCAN_DIGITAL_HI 0xFFAA00L
#define CLR_SCAN_DIGITAL_LO 0xAA9900L

#define CLR_CHANGED_BUTTON_DEFAULT VAL_OFFWHITE// This seems to be the default command button colour.
#define CLR_CHANGED_BUTTON_PANEL_CHANGED 0x00E69CDF// TO BE DONE: Changes have been made but button not pressed.
#define CLR_CHANGED_BUTTON_CLICKED 0x00E854DC// Changed button clicked, waiting for next cycle.


/***********************************************************************
Control Handles
*************************************************************************/
int MNU_ANALOGTABLE_SCANCELL; // Control Menu Handles
int MNU_ANALOGTABLE_SCANCELL_TIMESCALE;
int MNU_DIGTABLE_SCANCELL;
int MNU_TIMETABLE_SCANCELL;
int MNU_DDS_EOR_SCANCELL;

int PANEL_TB_SHOWPHASE[NUMBEROFPAGES]; 	// Toggle buttons for pages
int PANEL_CHKBOX[NUMBEROFPAGES];		// Array for collecting the handles for the enable check boxes for pages
int ischecked[NUMBEROFPAGES];			// Array of if a page is enabled or not
int PANEL_OLD_LABEL[NUMBEROFPAGES];   	// Old column labels; superceded by InfoArray; kept for backwards compatibility with old save files

// Various panels
//panelHandles: 8: ScanTableLoader  9:NumSet  10:LaserSettings  11:LaserControl
int panelHandle0;
int panelHandle,panelHandle2,panelHandle3,panelHandle4,panelHandle5;
int panelHandle6,panelHandle7,panelHandle8,panelHandle9,panelHandle10;
int panelHandle11,panelHandle12,panelHandle13;
int panelHandleANRITSU, panelHandleANRITSUSETTINGS;
int panelHandle_sub1,panelHandle_sub2;
int menuHandle;


/************************************************************************
DDS variables
*************************************************************************/

// DDS stuff
typedef struct ddsoptions_struct {
	float start_frequency; /* in MHz*/
	float end_frequency;
	float amplitude;
	float delta_time;
	BOOL is_stop;
} ddsoptions_struct;

typedef struct dds2options_struct {
	float start_frequency; /* in MHz*/
	float end_frequency;
	float amplitude;
	float delta_time;
	BOOL is_stop;
} dds2options_struct;

typedef struct dds3options_struct {
	float start_frequency; /* in MHz*/
	float end_frequency;
	float amplitude;
	float delta_time;
	BOOL is_stop;
} dds3options_struct;

// Create the ddstable arrays. One array per row of cells.
// Note that if dds2options_struct (and 3) is ever different from ddsoptions_struct
//	then many functions will break with dds2 things if they are ever uncommented again.
ddsoptions_struct ddstable[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];
dds2options_struct dds2table[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];
dds3options_struct dds3table[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];

int Active_DDS_Panel; // 1 for Rb evap dds, 2 for K40 evap dds, 3 for HFS dds   !!!!


struct DDSClock{
	double 	extclock;
	int 	PLLmult;
	double	clock;// clock is a derived quantity from extclock and PLLmult
}	DDSFreq;


/************************************************************************
Laser variables
*************************************************************************/

// LaserTable Data and Settings Arrays

struct LaserTableValues{
	int fcn;			   // 0 for hold,1 for step,2 for ramp
	double fval;
};
struct LaserTableValues LaserTable[NUMBERLASERS][NUMBEROFCOLUMNS+1][NUMBEROFPAGES];

// Channel properties structs
struct LaserProps{
	int Active;				 	  // 1 if that laser is being used, 0 otherwise
	char Name[20];			 	  // Laser name
	char IP[20];			 	  // IP addresses for Rabbit controller TCP Socket
	unsigned int Port;		 	  // Port for Rabbit controller TCP Socket
	int DigitalChannel;		      // An array built from the LASCHAN values
	double DDS_Clock;		      // Laser DDS Clock Frequency
	double ICPREF;			      // Charge Pump Ref Current(mA) Note: ICP=1.24V/CPISET --> CPISET resistor is 2.4kOhm on eval board
	int ICP_FD_Mult;			  // Charge pump current mode multipliers: FD -> Freq Detect
	int ICP_WL_Mult;			  // Wide Loop
	int ICP_FL_Mult;			  // Final Loop
	unsigned int DDS_Div;
	unsigned int DDS_Type;		  // either 9854 or 9858. NewExtavour
}LaserProperties[NUMBERLASERS];


#define MINRAMP_LEADOUT (0.0)   //This much time in (ms) is given to the rabbit to begin preprocessing of events following a ramp.
#define MAX_DDS_SCANRATE (100.0) //in MHz/ms (this is actually currently limited by the max laser scan response rate
#define MIN_DDS_FREQ (0.001) //MHz - this is a result of the minimum freq which is accepted into Hittite prescalers
#define MAX_DDS_FREQ (450.0) //MHz
#define MAX_PFD_INPUT (150.0) //MHz - limitation of AD9858 detector
///#define ONBOARD_ROSA_DIV (1) //On board integer divders ratio cutting down rosa signal (16 on eval boad)




/************************************************************************
Anritsu variables
*************************************************************************/

typedef struct anritsuoptions_struct {
	//float start_frequency;// in GHz
	float end_frequency;
	//float start_power;// in dBm
	float end_power;
	int framptype;
	int pramptype;
	BOOL is_stop;
} anritsuoptions_struct;

anritsuoptions_struct anritsutable[NUMBEROFCOLUMNS+1][NUMBEROFPAGES];


struct AnritsuSetting{
	char 	ip[50];	// ip address of Anritsu MG37022A
	double dwell_time; //dwell time of Anritsu
	int dig_trigger;   // assigned digital trigger
	int com_on; // communication on/off switch
	double offset; // offset for frequency in GHz
	};
struct AnritsuSetting AnritsuSettingValues[NUMBEROFANRITSU];


/************************************************************************
VISA and GPIB interface
*************************************************************************/

// VISA interface
ViSession VIsess;
ViSession rmSession;

// GPIB stuff
// All are unused after switch to VISA; kept for backwards compatibility
// Stefan's new GPIB panel I think made the SRS ones obsolete before VISA made them all obsolete
int GPIB_device;
int GPIB_address;
int GPIB_ON;
double SRS_amplitude, SRS_offset, SRS_frequency;// SRS_frequency is still live in scan.c which isn't used anymore


#define NUMBERGPIBDEV (20)			// number of programmable GPIB devices
#define NUMGPIBPROGVALS (20)		// max number of numerical values to be used in programming -- should match number of columns in settings table
#define NUMGPIBCMDREPS (10)			// max number of "%f"'s that can be replaced in a single command (between ";"'s)
#define MAXVISAADDR (199)			// max pseudo-address for VISA devices (GIPB addrs are <= 32)

#define RIGOL_DG1022Z_NAME ("Rigol_DG1022Z")// VISA resouce name or alias of the Rigol DG1022Z
#define RIGOL_DG1022Z_ADDR (100)	// pseudo-address of the Rigol DG1022Z

#define RIGOL_DG4162_NAME ("Rigol_DG4162")// VISA resouce name or alias of the Rigol DG4162
#define RIGOL_DG4162_ADDR (101)	// pseudo-address of the Rigol DG4162



struct GPIBDDeviceProperties{
	int		address;	// GPIB address (1..32), 0 means: not initialized
	char    devname[50]; // name of the device
	char	cmdmask[1024];
	char	command[1024];
	char	lastsent[1024];
	double	value[20];
	BOOL	active;
} GPIBDev[NUMBERGPIBDEV];






/************************************************************************
MultiScan
*************************************************************************/


// MultiScan
#define NUMMAXSCANPARAMETERS (70)	// Number of allowed scan parameters
#define NUMDIGITALSCANOFFSET (100)	// Offset for digital channels in row-addressing
									// Should be larger than NUMBERANALOGROWS
#define NUMGPIBSCANOFFSET	 (200)
#define NUMANALOGTIMESCALEOFFSET (1000)	// Offset to create pseudo-rows corresponding to the timescale
										// variable for each analog channel cell.
										// Should be <panel> <regular column> <regular row + 1000>
#define DDS_EOR_SCAN_COLUMN_VALUE	(1000)	// The column value (not offset) of the psuedo-column
											// MultiScan uses for the EOR of the DDS's.



// MultiScan (shared with old 1 and 2 dim scans)
int parameterscanmode; // 0: multi-scan, 1: one-parameter, 2: two-parameter


// The length of ScanBuffer used to be ScanBuffer[1000] hard coded. I want to be able to change it.
// Nobody uses Scan2Buffer anymore so I will leave that as is.
// I also only changed the multi-parameter scan code that uses ScanBuffer. I didn't bother with
// the one and two parameter scan code.
#define SCANBUFFER_LENGTH				(3000)
#define SCANBUFFER_TIMEBUFFER_LENGTH	(30)


struct ScanBuff{ // Needing a scan buffer to watch scan, since list can be updated during scan
	int Iteration;
	int Step;
	double Value;
	double MultiScanValue[NUMMAXSCANPARAMETERS]; // Added for MultiScans, not interferring with other scans
	char Time[SCANBUFFER_TIMEBUFFER_LENGTH];
	int BufferSize;
} ScanBuffer[SCANBUFFER_LENGTH];



// Multi-parameter scan structures and global variables
// 2012-10-06 Stefan Trotzky - V16.0.0
struct MultiScanParameters{
	int    	Row;
	int    	Column;
	int    	Page;
	int    	Type;// 0 for Analog, 1 for Time, 2 for DDS, 3 for Laser, 4 for Anritsu
	double	CurrentScanValue;
	BOOL   	CellExists;
	double	Time;					  // for Time scan
	struct 	AnalogTableValues Analog; // for Analog Channel scan
	struct 	AnalogTableValues DDS;	  // for DDS scan
	struct 	LaserTableValues Laser;   // for Laser scan
	struct	AnalogTableValues Anritsu;// for Anritsu scan
	int		Digital;				  // for Digital Channel scan
	double  GPIBvalue;				  // for GPIB value scans
};
struct MultiScanValues{
	int		CurrentStep;
	int		CurrentIteration;
	int		Iterations;
	int		NumPars;
	int 	Counter;
	int		NextCommandsFileNumber;// 0 for commands_00000.txt, 1 for commands_00001.txt etc.
	char	CommandsFilePath[MAX_PATHNAME_LEN];
	char	ScanDirPath[MAX_PATHNAME_LEN];// Used for auto saving the .mscan file. (and any future usage)
	int		HoldingPattern;// 0 for normal replacement, 1 for replace with previous values(ie. no change), 2 for replace cells with their initial values
	double 	SentinelFoundValue;
	BOOL	Done;
	BOOL	Active;
	BOOL	Initialized;
	struct	MultiScanParameters Par[NUMMAXSCANPARAMETERS];
} MultiScan;

// These should be local variables in UpdateMultiScanTable function but there is precedence for
// putting things here.
// Sentinel REPEAT_LAST_SENTINEL causes a holding pattern where the last line in the scan table
// before this sentinel is repeated semi-infinetely.
// Sentinel REPEAT_ORIGINAL_SENTINEL causes a holding pattern where the original values before
// the scan started are repeated semi-infinetly.
// Sentinel STOP_REPLACE_ORIGINAL_SENTINEL is the normal sentinel we know and love which stops
// the cycle and replaces the scanned cells with their original values.
// Note that in the code, anything less than -999.0 and not a special sentinel will behave the
// same as STOP_REPLACE_ORIGINAL_SENTINEL.
// Sentinel STOP_REPLACE_LAST_SENTINEL will stop the cycle, but will replace the scanned cells
// with the values from the last line of the scan table.
#define REPEAT_LAST_SENTINEL 			(-1111.0)
#define REPEAT_ORIGINAL_SENTINEL		(-1110.0)
#define STOP_REPLACE_ORIGINAL_SENTINEL	(-1000.0)
#define STOP_REPLACE_LAST_SENTINEL		(-1001.0)





/************************************************************************
Misc
*************************************************************************/

// Keep track of where in a table things are taking place
int currentx,currenty,currentpage;

// Draw or do not draw unused columns (eg. true if want draw negative time columns faded)
int isdimmed;

// Flow of control variables
BOOL ChangedVals;
BOOL UseSimpleTiming;
BOOL TwoParam;
BOOL ChangeScan1Param;

// Try debugging the random sequencer errors that seem to be happening with excessive GPIB command use.
int DEBUG_NUMBER_IBWRT_CALLS;





// Don't know stuff past here.








/* Parameter Scan variables*/
typedef struct AnalogScanParameters{
	double Start_Of_Scan;
	double End_Of_Scan;
	double Scan_Step_Size;
	int	   Iterations_Per_Step;
	int	   Analog_Channel;
	int	   Analog_Mode;
} AnalogScan;

typedef struct TimeScanParameters{
	double 	Start_Of_Scan;
	double 	End_Of_Scan;
	double 	Scan_Step_Size;
	int	   	Iterations_Per_Step;
}	TimeScan;

typedef struct DDSScanParameters{
	double  Base_Freq;
	double  Start_Of_Scan;
	double 	End_Of_Scan;
	double 	Scan_Step_Size;
	double  Current;
	int	   	Iterations_Per_Step;
} DDSScan;

typedef struct DDSFloorScanParameters{
	double 	Floor_Start;
	double 	Floor_End;
	double 	Floor_Step;
	int		Iterations_Per_Step;
} DDSFloorScan;

typedef struct SRSScanParameters{
	double 	SRS_Start;
	double 	SRS_End;
	double 	SRS_Step;
	int		Iterations_Per_Step;
} SRSScan;

typedef struct LaserScanParameters{
	double 	Start_Of_Scan;
	double 	End_Of_Scan;
	double 	Scan_Step_Size;
	int		Iterations_Per_Step;
} LaserScan;


struct ScanParameters{
	int    Row;
	int    Column;
	int    Page;
	int    ScanMode;// 0 for Analog, 1 for Time, 2 for DDS, 3 for Anritsu
	BOOL   ScanDone;
	BOOL   Scan_Active;
	BOOL   Use_Scan_List;
	struct AnalogScanParameters	Analog;
	struct TimeScanParameters	Time;
	struct DDSScanParameters   	DDS;
	struct DDSFloorScanParameters DDSFloor;
	struct SRSScanParameters SRS;
	struct LaserScanParameters Laser;
}  PScan;

struct Scan2Parameters{
	int    Row;
	int    Column;
	int    Page;
	int    ScanMode;// 0 for Analog, 1 for Time, 2 for DDS
	BOOL   ScanDone;
	BOOL   Scan_Active;
	BOOL   Use_Scan_List;
	struct AnalogScanParameters	Analog;
	struct TimeScanParameters	Time;
	struct DDSScanParameters   	DDS;
	struct DDSFloorScanParameters DDSFloor;
	struct SRSScanParameters SRS;
	struct LaserScanParameters Laser;
}  PScan2;

struct ScanSet{
	double Start;
	double End;
	double Step;
	int	   Iterations;
	int    Current_Step;
	double Current_Value;
	int    Current_Iteration;
}	ScanVal;

struct Scan2Set{
	double Start;
	double End;
	double Step;
	int	   Iterations;
	int    Current_Step;
	double Current_Value;
	int    Current_Iteration;
}	Scan2Val;





struct Scan2Buff{
	int Iteration1;
	int Step1;
	double Value1;
	int Iteration2;
	int Step2;
	double Value2;
	char Time[100];
	int BufferSize;
} Scan2Buffer[1000];








struct Switches{
	BOOL loop_active;
} Switches;

struct LoopPoints{
	int startpage;
	int endpage;
	int startcol;
	int endcol;
} LoopPoints;



#endif

