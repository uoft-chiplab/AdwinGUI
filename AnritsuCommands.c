#include "vars.h"
#include <userint.h>
#include <ivi.h>
#include<ansi_c.h>
#include "AnritsuSettings.h"
#include "AnritsuSettings2.h"
#include "AnritsuControl.h"
#include "AnritsuControl2.h"

#include "AnalogControl2.h"
#include <math.h>
#include <time.h>
#include <windows.h>

//#include "LaserSettings.h"
//#include "LaserSettings2.h"

ViSession VIsess = -1;
ViSession rmSession = -1;
ViRsrc resname;
ViString msg,msg1, msg_2, msg1_2;
ViPUInt32 retcount;
ViStatus osuc;

int time_is_good[NUMBEROFCOLUMNS+1][NUMBEROFPAGES]; // determines whether elements in time array are valid
int last_col = 1;
int last_page =1;
int first_col =1;
int first_page =1;


int round (double x) //stupid incomplete C libs
{
	int y;
	double diff1;
	double diff2;

	diff1 = fabs(ceil(x) - x);  // ABS =/= FABS
	diff2 = fabs(floor(x) - x);


	if( diff1 > diff2){
	y = floor(x);
	}
	else{
	y = ceil(x);
	}
	//printf("Trying to round %f. Difference from ceil %f; floor %f; rounded value %f\n",x,diff1,diff2,y);
	return y;


}


double TOTALTIME (void)   // Finds the anritsu table length (in milli seconds) by scanning for the first non zero frequency
{
	double total_time = 0;
	double target_freq;
	double last_freq;
	double first_freq;
	int first_pos;
	int current_pos;
	int kk;
	int jj;
	int f1;
	int f2 = 0;

	int f3 =0;
	int a;
	int c;
	//int total_subpoints=0;

	f1 = 0;

	// initialize is time good array to ones
	for (kk = 1; kk < NUMBEROFPAGES; kk++){

		for(jj = 1; jj <NUMBEROFCOLUMNS + 1; jj++){

		time_is_good[jj][kk] =1;


		}
	}

	//Now turn off i.e zero invalid time cell points
	for (kk = 1; kk < NUMBEROFPAGES; kk++){

		if (ischecked[kk] == 0){	  // decheck a page
		for (a = 1; a <NUMBEROFCOLUMNS +1; a++){

		time_is_good[a][kk] = 0;
		}

		}

		for(jj =1; jj <NUMBEROFCOLUMNS +1; jj++){

		if(TimeArray[jj][kk] ==0){
			for (c = jj; c <NUMBEROFCOLUMNS +1; c++){

			time_is_good[c][kk] = 0;
			}
			//break;
		}
		if(TimeArray[jj][kk] < 0){
			time_is_good[jj][kk] = 0;
		}
		}
	}

	//flag 1
	for (kk = NUMBEROFPAGES; kk >= 1; kk--){ // scan through backwards to find
	// last instance of a nonzero element

		if(ischecked[kk] ==1){
			for (jj = NUMBEROFCOLUMNS; jj>= 1; jj--){

			last_freq = AnalogTable[jj][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][kk].fval;

			if (time_is_good[jj][kk] == 1){   // only consider non-dimmed columns
				if(last_freq > 0){
				last_col = jj;
				last_page =kk;
				f1 = 1; //sets flag to exit
				break;
				}
			}
			}
			}
			if (f1 ==1){
			break;
			}
	}

	for (kk = 1; kk < NUMBEROFPAGES; kk++){ // scan through forwards to find
	// first instance of a nonzero element

		if(ischecked[kk] ==1){
			for (jj = 1; jj < NUMBEROFCOLUMNS+1; jj++){

			last_freq = AnalogTable[jj][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][kk].fval;
				if(TimeArray[jj][kk] == 0){
				break;
				}

				if (TimeArray[jj][kk] > 0){

				if(last_freq > 0){
				first_col = jj;
				first_page =kk;
				f2 = 1; //sets flag to exit
				break;
				}
				}
			}
			}
			if (f2 ==1){
			break;
			}
	}

	first_pos = (first_page-1)*(NUMBEROFCOLUMNS-1) + first_col;

	//adding to the total time
	for (kk = first_page; kk < NUMBEROFPAGES; kk++){

		jj=1;

		if (kk == first_page){
			 	jj = first_col;
		}

		for (; jj < NUMBEROFCOLUMNS + 1; jj++){


			if (time_is_good[jj][kk]== 1){
			total_time = total_time + TimeArray[jj][kk];
			if(kk == last_page && jj == last_col){
				break;
			}
		}

		if(kk == last_page && jj == last_col){
			break;
			}
		}



	}

	// actually adding up the time cells for total time

	/*for (kk =1; kk <= last_page; kk++){  // scan through pages (from page = 1 to page =10)

		if (ischecked[kk] == 1){		//see if page is enabled (checkbox)

			for (jj = 1; jj < NUMBEROFCOLUMNS; jj++){ // scan through columns

				if (TimeArray[jj][kk] == 0){  // skips rest of page
					break;
				}

				target_freq = AnalogTable[jj][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][kk].fval; 	   // frequency val in the cell

				current_pos = (kk-1)*(NUMBEROFCOLUMNS-1) + jj;

				if (TimeArray[jj][kk] > 0 && target_freq > 0 && current_pos >= first_pos){

				total_time = total_time + TimeArray[jj][kk];

				}

			}

		}

	if(kk == last_page && jj == last_col){

	break;
	}

	}
	 */

	printf(" First column: %i\n", first_col);
   printf("First page: %i\n", first_page);

	printf(" Last column: %i\n", last_col);
   printf("last_page: %i\n", last_page);

	if (total_time == 0){
	total_time =100;
	}

	return total_time; //in ms


}





//This function converts the entries in the analog / anritsu table into a form
// the microwave source can understand
// Input anritsu table.



//This function tells the Anritsu the list of power/freq values it should spit out when triggered.
int AnritsuCOMMUNICATE (void)
{
	int j,len,currpos;
	double freq[10050] = { 0 };  //define the table length. Freq in
	double power[10090];
	double freqi[10090] = { 0 };
	double poweri[10050] = { 0 };
	int subpoints;		// number of anritsu table values per cell in the analog table
	int kk = 1; //
	float end_f;
	float start_f;
	float end_p;
	float start_p;
	float slope_f;
	float slope_p;
	int array_pos = 0;
	float temp_sum = 0;
	int jj, a, b, c, d;
	int ramp_f;
	int ramp_p;
	double total_t;
	double t_step;
	char dwell[100];
	char total_points[100];
	double testing;
	int time_pos;
	int first_time_pos;
	int flag = 0;
	int total_subpoints=0;
	double t_step_anr;
 	time_t start, stop;
   double diff;

   //total_t = 5000;
   total_t = TOTALTIME(); // in ms.  find the total time the anritsu needs include in the array
   //t_step_anr = (total_t /1000)- 0.38;  // in ms. the time step each array point will represent
   //t_step = (total_t /1000);

   t_step_anr = (10 - 0.38);  //subtract off the 0.38 ms switching time
   t_step = 10;			// dwell time that we tell the anritsu to set

   first_time_pos = (first_page-1)*(NUMBEROFCOLUMNS-1) + first_col;
   if (t_step < 0.05){
   t_step =0.05;
   }

   //setting up Anritsu Trigger

   //DChName[68].chname = "Anritsu Trigger";
   //DChName[68].chnum = 118;
   //DChName[68].resettolow = 1;

   //DigTableValues[first_col][68][first_page] = 1;

   //DChName[]

   //LaserProperties[3]
   start = time(NULL);


	resname = malloc(100);


	printf("Begin communication\n");		//

	//rmSession = -1;
	if (rmSession == -1)
	{
	   viOpenDefaultRM(&rmSession);
	}

	printf("Begin rm session\n");			  //

	//VIsess = -1;
	if (VIsess == -1)
	{
		sprintf(resname,"TCPIP0::%s::INSTR",AnritsuSettingValues[0].ip);

		printf(resname);

		//printf("%d\n",rmSession);
		osuc = viOpen(rmSession,resname, VI_NULL, VI_NULL, &VIsess);
		//printf("%d",osuc);
		if ( osuc !=0)  {
			return -1;
			}
	}

	printf("Begin Nathan Code\n");

	////***********	 NATHAN's BADASS CODE

	for (kk = 1; kk < NUMBEROFPAGES; kk++){	//scan pages

		if (flag ==1){
		break;
		}

		if (ischecked[kk] == 1){ //page is check box enabled

		for (jj = 1; jj <NUMBEROFCOLUMNS+1; jj++){   // scan through columns

			if (TimeArray[jj][kk] == 0){  // skips rest of page

			break;
			}

	    	time_pos = (kk -1)*(NUMBEROFCOLUMNS-1) + jj;
	    	ramp_f = ("%i\n", AnalogTable[jj][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][kk].fcn);

	    	subpoints = round(TimeArray[jj][kk]/t_step);
	    	printf("subpoints %d\n", subpoints);

	    	end_f = AnalogTable[jj][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][kk].fval;


	    	if (time_is_good[jj][kk] ==1 && time_pos >= first_time_pos ){		// skips negative columns or time steps that are too small
	    							//if positive, fill in anritsu array

	   			 // if (jj ==0 && kk==0){
	   			if (array_pos == 0){
	    		start_f = 0;

	    		}
	    		else {
	    		start_f = freqi[array_pos -1];

	    		}



				if (ramp_f == 1){ //step ramp
					 printf("Stepping to %lf\n", end_f);
					for (b = 0; b < subpoints; b++){
						freqi[b + array_pos] =  end_f;
						//printf("%lf\n", freqi[b]);
					}
				}

				else if (ramp_f ==2){	  // assumed linear ramp
					for (b = 0; b < subpoints; b++){
						freqi[b + array_pos] =  start_f + (end_f - start_f)*b/(subpoints);
					//	printf("%lf\n", freqi[b]);
					}
				}

				else if(ramp_f ==6){ //hold last
					for (b = 0; b < subpoints; b++){
						freqi[b + array_pos] =  start_f;
				}
				}

				else
				{
				  for (b = 0; b < subpoints; b++){
						freqi[b + array_pos] =  0.01;
						}

				}

			printf("Current position in life %d\n",array_pos);
			array_pos += subpoints;
			total_subpoints += subpoints;

			if (jj ==last_col && kk == last_page){
			flag =1;

			break;
			}


			}

		}
		}

	}



	  //loading arrays for anritsu
	for (j = 0; j<10050; j++)
	{

	freq[j] = freqi[j] + AnritsuSettingValues[0].offset;
		//  freq[j]= 7.8;
		//freq[j] = AnalogTable[4][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][1].fval;
	}

	//for (j = 0; j<1001; j++)
	//{
	  	//printf("%lf\n", freq[j]);

	//}

	printf("DONE NATHAN's CODE\n");
	printf("%lf\n", freq[1]);

	//printf("%lf\n", anritsutable[5][1].end_power);
	//printf("%lf\n", 500);


	viPrintf(VIsess,":SOURce:FREQ:MODE LIST\n");
	printf("209 done mode list\n");
	viPrintf(VIsess,":TRIGger:SOURce EXTernal\n");
	printf("211external trigger\n");


	//viPrintf(VIsess,":SOURce:LIST:TYPE FLEVel\n");
	viPrintf(VIsess,":SOURce:LIST:TYPE FREQuency\n");
	viPrintf(VIsess,":OUTPut:PROTection ON\n");

	printf("211 FLEVEL\n");
	viPrintf(VIsess,":OUTPut:STATe ON\n");
	printf("211STATE ON\n");

	viPrintf(VIsess,":SOURce:LIST:STARt 0\n");
	printf("211 start array\n");

	total_subpoints+=1;
	sprintf(total_points, "%s %.4i%s",  ":SOURce:LIST:STOP", total_subpoints, "\n");
	viPrintf(VIsess, total_points);

	//viPrintf(VIsess,":SOURce:LIST:STOP 1150\n");
	printf("211 list stop\n");



	sprintf(dwell, "%s %.4f%s",  ":SOURce:LIST:DWELl", t_step_anr, "ms\n");

	viPrintf(VIsess, dwell);
	printf("211 dwell \n");

	//viPrintf(VIsess,":OUTPut:PROTection OFF\n");
	 //******************************************

    //start loading FREQ values in string form

    msg = malloc(600000);

	msg1 = malloc(100);
    sprintf(msg1,"%s",":SOURce:LIST:FREQuency ");
    len = strlen(msg1);
    strncpy (msg,msg1,len);
    currpos = strlen(msg1);

    for (j = 0; j<10001; j++)
    {
        sprintf(msg1," %.8fGHz,",freq[j]);
        len = strlen(msg1);
        strncpy (msg+currpos,msg1,len);
        currpos = currpos + len;
        //printf("I am a terrible person");
    }

    printf("236I am a terrible person\n");
    printf("%s\n", msg1);

    msg[currpos-1] = '\n';
    msg[currpos] = '\0';

    //viPrintf(VIsess,":SOURce:LIST:INDex 0\n");		   //other magic
    viPrintf(VIsess,msg);			   // actual sending of msg
    /*
    msg_2 = malloc(300000);
	msg1_2 = malloc(100);
    sprintf(msg1_2,"%s",":SOURce:LIST:POWer ");
    len = strlen(msg1_2);
    strncpy (msg_2,msg1_2,len);
    currpos = strlen(msg1_2);

    for (j = 0; j<5000; j++)
    {
        sprintf(msg1_2," %.2fdBm,",power[j]);
        len = strlen(msg1_2);
        strncpy (msg_2+currpos,msg1_2,len);
        currpos = currpos + len;
    }

    printf("236I am a terrible person\n");
    printf("%lf\n", freq[200]);

    msg_2[currpos-1] = '\n';
    msg_2[currpos] = '\0';

    //viPrintf(VIsess,":SOURce:LIST:INDex 0\n");	//black magic   Jan 2011
    viPrintf(VIsess,msg_2);
    */

    //viPrintf(VIsess,":SOURce:LIST:INDex 0\n");
    //sleep(100);
	printf("before close\n");
	//viClose(VIsess); // was left out.. makes first cycle "faster". regprograming the table requires restart if uncommented.
	//viClose(rmSession); //fast but screws up when cycle is halted
	printf("263 done being stupid\n");
	stop = time(NULL);
   diff = difftime(start, stop);
   printf ("cell : %f\n", AnalogTable[5][NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS + 1][1].fval);
   printf("Execution Time: %f\n", diff);
   printf(" Total time: %lf\n", total_t);
   printf(" Total subpoints: %i\n", total_subpoints);
   printf(" OFFSET: %f\n", AnritsuSettingValues[0].offset);
	return 0;

}

//double STARTTIME (void) //





int CVICALLBACK CancelANRITSUConCALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel(panelHandleANRITSU);
			DrawNewTable(0);
			break;
		}
	return 0;
}


//Updates the analog and anritsu tables after user has double clicked  on a cell
// in the GUI, panel is brought up, and user enters in parameters, presses OK
int CVICALLBACK SetANRITSUConCALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int itemp;
	double dtemp;
	int ancury, offset;
	offset =  NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS;
	ancury = currenty - offset-1;
	ancury = 2*(ancury/2);
	ancury = ancury + offset+1;
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_CONTROL_MODE, &itemp);
			GetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_FREQ, &dtemp);

			AnalogTable[currentx][ancury][currentpage].fcn = itemp;
			anritsutable[currentx][currentpage].framptype = itemp;
			if(itemp!=6)
			{
				AnalogTable[currentx][ancury][currentpage].fval=dtemp;
				anritsutable[currentx][currentpage].end_frequency = dtemp;
			}

			GetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_CONTROL_MOD_P, &itemp);
			GetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_POWR, &dtemp);

			AnalogTable[currentx][ancury+1][currentpage].fcn = itemp;
			anritsutable[currentx][currentpage].pramptype = itemp;
			if(itemp!=6)
			{
				AnalogTable[currentx][ancury+1][currentpage].fval=dtemp;
				anritsutable[currentx][currentpage].end_power = dtemp;
			}


			HidePanel(panelHandleANRITSU);
			DrawNewTable(0);
			break;
		}
	return 0;
}

//***************************************************************************
void SetAnritsuControlPanel(void)
{
	// Sets all relevant info into the Anritsu Control Panel based on the current cell selection

	//SetCtrlVal(panelHandleANRITSU,CTRL_PANEL_STRUNITS, AChName[currenty].units);
	//SetCtrlVal(panelHandleANRITSU,CTRL_PANEL_STR_CHNAME,AChName[currenty].chname);
	int ancury, offset;
	offset =  NUMBERANALOGCHANNELS+NUMBERDDS+NUMBERLASERS;
	ancury = currenty - offset-1;
	ancury = 2*(ancury/2);
	ancury = ancury + offset+1;
	SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_CONTROL_MODE,AnalogTable[currentx][ancury][currentpage].fcn);
	SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_CONTROL_MOD_P,AnalogTable[currentx][ancury+1][currentpage].fcn);


	if(AnalogTable[currentx][ancury][currentpage].fcn!=6)
		SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_FREQ, AnalogTable[currentx][ancury][currentpage].fval);
	else
		SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_FREQ,findLastVal(ancury,currentx, currentpage));

	SetCtrlVal(panelHandleANRITSU,PANEL_ANRITSU_LAST_FREQ,findLastVal(ancury,currentx, currentpage));

	if(AnalogTable[currentx][ancury+1][currentpage].fcn!=6)
		SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_POWR, AnalogTable[currentx][ancury+1][currentpage].fval);
	else
		SetCtrlVal (panelHandleANRITSU,PANEL_ANRITSU_TARGET_POWR,findLastVal(ancury+1,currentx, currentpage));

	SetCtrlVal(panelHandleANRITSU,PANEL_ANRITSU_LAST_POWR,findLastVal(ancury+1,currentx, currentpage));

}

int CVICALLBACK ANRITSU_SET_TABLECALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

			break;
		}
	return 0;
}

int CVICALLBACK ANRITSU_SET_DONE_CALLBACK (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char 	ipadd[50];
	double 	dwell_t;
	int 	dig_trig;
	int anritsu_com_on;
	double offset;

	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANRITSU_IPADDBOX,ipadd);
			sprintf(AnritsuSettingValues[0].ip,ipadd);

		//	GetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANRITSU_OFFSET,&offset);
		//	AnritsuSettingValues[0].offset = offset;
			GetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANRITSU_COM_SWITCH,&anritsu_com_on);
			AnritsuSettingValues[0].com_on = anritsu_com_on;
			/*GetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANR_DIG_TRIG,dig_trig);
			AnritsuSettingValues[0].dig_trigger = dig_trig;
			GetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANR_DWELL_TIME,dwell_t);
			AnritsuSettingValues[0].dwell_time = dwell_t;
			*/
			HidePanel(panelHandleANRITSUSETTINGS);
			DrawNewTable(0);
			break;
		}
	return 0;
}

void LoadAnritsuSettings(void)
{

	  SetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANRITSU_IPADDBOX,AnritsuSettingValues[0].ip);
	  SetCtrlVal (panelHandleANRITSUSETTINGS, ANRITPANEL_ANRITSU_COM_SWITCH,AnritsuSettingValues[0].com_on);
	  return;
}
