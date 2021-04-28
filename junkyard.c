// A junkyard for function definitions and declarations.











//***************************************************************************************************
// from main.c
void ConvertIntToStr(int int_val, char *int_str);
void ConvertIntToStr(int int_val, char *int_str)
{

	int i,j;

	for (i=j=floor(log10(int_val));i>=0;i--)
	{
		int_str[i] = (char) (((int) '0') + floor(((int) floor((int_val/pow(10,(j-i))))%10)));
	}

	int_str[j+1] = '\0';

	return;
}

//***************************************************************************************************
// From main.c
	//DrawCanvasArrows();
void DrawCanvasArrows(void);
void DrawCanvasArrows(void)
{
 int loopx=0,loopx0=0;
// draws an arrow in each of 2 canvas's on the GUI.  These canvas's are moved around to indicate the location
// of a loop in the Adwin output.
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_WIDTH,1);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_COLOR,VAL_DK_GREEN);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PICT_BGCOLOR,VAL_TRANSPARENT);

CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(0,0), MakePoint(15,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(15,0), MakePoint(8,12));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,12), MakePoint(0,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,11), MakePoint(1,1));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,10), MakePoint(2,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(0,0), MakePoint(8,4));
SetCtrlAttribute (panelHandle, PANEL_CANVAS_START, ATTR_PEN_WIDTH,4);
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(4,2), MakePoint(11,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(11,2), MakePoint(8,8));
CanvasDrawLine (panelHandle, PANEL_CANVAS_START, MakePoint(8,8), MakePoint(4,2));

SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_WIDTH,1);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_COLOR,VAL_DK_RED);
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PICT_BGCOLOR,VAL_TRANSPARENT);
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(0,0), MakePoint(8,4));

CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(0,0), MakePoint(15,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(15,0), MakePoint(8,12));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,12), MakePoint(0,0));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,11), MakePoint(1,1));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,10), MakePoint(2,2));
SetCtrlAttribute (panelHandle, PANEL_CANVAS_END, ATTR_PEN_WIDTH,4);
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(4,2), MakePoint(11,2));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(11,2), MakePoint(8,8));
CanvasDrawLine (panelHandle, PANEL_CANVAS_END, MakePoint(8,8), MakePoint(4,2));

loopx0=170;		//horizontal offset
loopx=8;		//x position...in horizontal table units
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_LEFT,loopx0+loopx*40);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_START,ATTR_TOP,141);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_LEFT,loopx0+loopx*40+25);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_END,ATTR_TOP,141);

SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_LEFT,168);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_TOP,140);
SetCtrlAttribute(panelHandle,PANEL_CANVAS_LOOPLINE,ATTR_WIDTH,685);
}



// From GUIDesign.c via saveload.c
// Never used and I don't think it's fully implemented. Always want to load something we know is good anyways.
//
void LoadLastSettings(int);
void LoadLastSettings(int check)
{
	//If .ini exists, then open the file dialog.  Else just open the file dialog.  Add a method to load
	//last used settings on program startup.
	FILE *fpini;
	char fname[100]="",c,fsavename[500]="",loadname[100]="",buff[600];
	int i=0,inisize=0, success;
	//Check if .ini file exists.  Load it if it does.
	if(!(fpini=fopen("gui.ini","r"))==NULL)
	{
		while (fscanf(fpini,"%c",&c) != -1) fname[inisize++] = c;  //Read the contained name
	}
	fclose(fpini);

	c=fname[inisize-1];
	strncpy(loadname,fname,inisize-2);
	if((check==0)||(c==49))			  // 49 is the ascii code for "1"
	{
		RecallPanelState(PANEL, loadname, 1); // This one can be problematic when elements have been removed from the GUI!!!
		success = LoadArraysV16(fname,strlen( loadname));

		if (success)
		{
			LaserSettingsInit();

			i=500;
			while(i>=0&&fsavename[i]!='\\')
				i--;
			strcpy(buff,SEQUENCER_VERSION);
			strcat(buff,&fsavename[i+1]);
			SetPanelAttribute (panelHandle, ATTR_TITLE, buff);
		}

	}
	DrawNewTable(0);
}


// A portion of SaveSettings
	if(!(fpini=fopen("gui.ini","w"))==NULL)
	{
		fprintf(fpini,fsavename);
		fprintf(fpini,"\n%d",loadonboot);
	}
	fclose(fpini);
