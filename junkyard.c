// A junkyard for function definitions and declarations.











//***************************************************************************************************
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
