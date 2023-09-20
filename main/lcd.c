/*
 * lcd.c

 *
 *  Created on: Nov 3, 2022
 *      Author: vivado
 */

/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
  UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved

  This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
  and RGB_GLCD libraries for Arduino and chipKit. As the number of
  supported display modules and controllers started to increase I felt
  it was time to make a single, universal library as it will be much
  easier to maintain in the future.

  Basic functionality of this library was origianlly based on the
  demo-code provided by ITead studio (for the ITDB02 modules) and
  NKC Electronics (for the RGB GLCD module/shield).

  This library supports a number of 8bit, 16bit and serial graphic
  displays, and will work with both Arduino, chipKit boards and select
  TI LaunchPads. For a full list of tested display modules and controllers,
  see the document UTFT_Supported_display_modules_&_controllers.pdf.

  When using 8bit and 16bit display modules there are some
  requirements you must adhere to. These requirements can be found
  in the document UTFT_Requirements.pdf.
  There are no special requirements when using serial displays.

  You can find the latest version of the library at
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the
  examples and tools supplied with the library.
*/

#include "lcd.h"
#include "note.h"

// Global variables
int fch;
int fcl;
int bch;
int bcl;
struct _current_font cfont;
int prev_vol_pixel_width = 0;
char itoa_temp[7];
static int prev_tunerDisplay_width = DISP_X_SIZE;
static int log_prev_counts[512];
static int prev_log_max = 0;
int prevErr = 0;


// Read data from LCD controller
// FIXME: not work
u32 LCD_Read(char VL)
{
    u32 retval = 0;
    int index = 0;

    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);

    //while (0 == (Xil_In32(SPI_SR) & XSP_SR_TX_EMPTY_MASK));
    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
    Xil_Out32(SPI_DC, 0x01);

    while (1 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));


    while (0 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK)) {
       retval = (retval << 8) | Xil_In32(SPI_DRR);
       xil_printf("receive %dth byte\n", index++);
    }

    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    return retval;
}


// Write command to LCD controller
void LCD_Write_COM(char VL)
{
    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 16-bit data to LCD controller
void LCD_Write_DATA16(char VH, char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VH);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Initialize LCD controller
void initLCD(void)
{
    int i;

    // Reset
    LCD_Write_COM(0x01);
    for (i = 0; i < 500000; i++); //Must wait > 5ms


    LCD_Write_COM(0xCB);
    LCD_Write_DATA(0x39);
    LCD_Write_DATA(0x2C);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x34);
    LCD_Write_DATA(0x02);

    LCD_Write_COM(0xCF);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0XC1);
    LCD_Write_DATA(0X30);

    LCD_Write_COM(0xE8);
    LCD_Write_DATA(0x85);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x78);

    LCD_Write_COM(0xEA);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x00);

    LCD_Write_COM(0xED);
    LCD_Write_DATA(0x64);
    LCD_Write_DATA(0x03);
    LCD_Write_DATA(0X12);
    LCD_Write_DATA(0X81);

    LCD_Write_COM(0xF7);
    LCD_Write_DATA(0x20);

    LCD_Write_COM(0xC0);   //Power control
    LCD_Write_DATA(0x23);  //VRH[5:0]

    LCD_Write_COM(0xC1);   //Power control
    LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0]

    LCD_Write_COM(0xC5);   //VCM control
    LCD_Write_DATA(0x3e);  //Contrast
    LCD_Write_DATA(0x28);

    LCD_Write_COM(0xC7);   //VCM control2
    LCD_Write_DATA(0x86);  //--

    LCD_Write_COM(0x36);   // Memory Access Control
    LCD_Write_DATA(0x48);

    LCD_Write_COM(0x3A);
    LCD_Write_DATA(0x55);

    LCD_Write_COM(0xB1);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x18);

    LCD_Write_COM(0xB6);   // Display Function Control
    LCD_Write_DATA(0x08);
    LCD_Write_DATA(0x82);
    LCD_Write_DATA(0x27);

    LCD_Write_COM(0x11);   //Exit Sleep
    for (i = 0; i < 100000; i++);

    LCD_Write_COM(0x29);   //Display on
    LCD_Write_COM(0x2c);

    //for (i = 0; i < 100000; i++);

    // Default color and fonts
    fch = 0xFF;
    fcl = 0xFF;
    bch = 0x00;
    bcl = 0x00;
    setFont(SmallFont);
    for (int i = 0; i < 512; i++)
    	log_prev_counts[i] = 0;
}


// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2)
{
    LCD_Write_COM(0x2A);
    LCD_Write_DATA(x1 >> 8);
    LCD_Write_DATA(x1);
    LCD_Write_DATA(x2 >> 8);
    LCD_Write_DATA(x2);
    LCD_Write_COM(0x2B);
    LCD_Write_DATA(y1 >> 8);
    LCD_Write_DATA(y1);
    LCD_Write_DATA(y2 >> 8);
    LCD_Write_DATA(y2);
    LCD_Write_COM(0x2C);
}


// Remove boundry
void clrXY(void)
{
    setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}


// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    fch = (r & 0x0F8) | g >> 5;
    fcl = (g & 0x1C) << 3 | b >> 3;
}


// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    bch = (r & 0x0F8) | g >> 5;
    bcl = (g & 0x1C) << 3 | b >> 3;
}


// Clear display
void clrScr(void)
{
    // Black screen
    setColor(0, 0, 0);

    fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}


// Draw horizontal line
void drawHLine(int x, int y, int l)
{
    int i;

    if (l < 0) {
        l = -l;
        x -= l;
    }

    setXY(x, y, x + l, y);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA16(fch, fcl);
    }

    clrXY();
}

int tri(int x, int y) {
    x = x % 40;
    y = y % 40;
    x = abs(20 - x);
    return (y >= 2 * x);
}

void drawBackground(int x, int y, int w, int h) {
    if(w==0 || h==0)
    	return;

//	setColor(120, 180, 20);
//	setColorBg(70, 130, 10);
    setColorBg(96, 125, 139);
    setXY(x, y, x + w - 1, y + h - 1); // set bounding region to draw shapes in
    int val;

    for (int j = y; j < y+h; j++) {
        for (int i = x; i < x+w; i++) {
//            val = tri(i, j);
//            if (val == 1) {
//                LCD_Write_DATA16(fch, fcl);
//            }
//            else
//            {
//                LCD_Write_DATA16(bch, bcl);
//            }
        	LCD_Write_DATA16(bch, bcl);
        }
    }
    clrXY();
}

// Fill a rectangular
void fillRect(int x1, int y1, int x2, int y2)
{
    int i;

    if (x1 > x2)
        swap(int, x1, x2);

    if (y1 > y2)
        swap(int, y1, y2);

    setXY(x1, y1, x2, y2);
    for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
    	LCD_Write_DATA16(fch, fcl);
    }

   clrXY();
}

void printA4Freq(int freq) {
	char noteA4[10] = "A4:";
	char units[] = "Hz";
	setColor(255, 255, 255);
	setColorBg(96, 125, 139);
	setFont(BigFont);
	sprintf(itoa_temp, "%d", freq);
	strcat(noteA4, itoa_temp);
	strcat(noteA4, units);
	lcdPrint(noteA4, a4freq_x, a4freq_y);
}

void printVolume(short volume, short was_idle) {
	int curr_vol_pixel_width = (int)(160*(volume/63.0));

	if (!was_idle) {
		if (curr_vol_pixel_width < prev_vol_pixel_width){
			setXY(vol_x+curr_vol_pixel_width, vol_y, vol_x+prev_vol_pixel_width, vol_y+10);
			setColor(200,200,200);
			fillRect(vol_x+curr_vol_pixel_width, vol_y, vol_x+prev_vol_pixel_width, vol_y+10);
		} else if (curr_vol_pixel_width > prev_vol_pixel_width) {
			setXY(vol_x+prev_vol_pixel_width, vol_y, vol_x+curr_vol_pixel_width, vol_y+10);
			setColor(240, 141, 54);
			fillRect(vol_x+prev_vol_pixel_width, vol_y, vol_x+curr_vol_pixel_width, vol_y+10);
		} else { //are equal; implies push button switches were clicked
			// do nothing
		}
	} else {
		setXY(vol_x+prev_vol_pixel_width, vol_y, vol_x + 160, vol_y + 10);

		setColor(200, 200, 200);
		fillRect(vol_x, vol_y, vol_x + 160, vol_y + 10);
		setColor(240, 141, 54);
		fillRect(vol_x, vol_y, vol_x + curr_vol_pixel_width, vol_y + 10);
	}

	clrXY();
	prev_vol_pixel_width = curr_vol_pixel_width;
}

void hideElement(int s) {
	if (s == A4FREQ) { //volume timer went off
		for (int x = a4freq_x; x <= a4freq_x+cfont.x_size*10; x += 2) {
			drawBackground(x, a4freq_y, 2, cfont.y_size+4);
		}
	} else if (s==MENU) {
		int start_x = DISP_X_SIZE/2 - (8+cfont.x_size*9)/2;
		int start_y = DISP_Y_SIZE/2 - (4+(cfont.y_size+4)*(sizeof(menu_items)/4))/2;
		drawBackground(start_x, start_y, 9+cfont.x_size*9, 5+(cfont.y_size+4)*(sizeof(menu_items)/4));
	} else if (s == HEADER) {
		drawBackground(0, 0, DISP_X_SIZE+1, 9+cfont.y_size);
	} else if (s == TUNER_MODE) {
		drawBackground( DISP_X_SIZE/2 - 14*cfont.x_size/2, 180, 1+14*cfont.x_size, cfont.y_size+1);
	} else if (s == TUNER_DISPLAY) {
		drawBackground(DISP_X_SIZE/2 - prev_tunerDisplay_width/2, 100,prev_tunerDisplay_width, cfont.y_size + 1);
	} else if (s == HISTOBG) {
	    for (int i = 0; i < 512; i++)
	    	log_prev_counts[i] = 0;
	    drawBackground(0, 0, DISP_X_SIZE+1, DISP_Y_SIZE+1);
	} else if (s == OCTAVE) {
		setFont(SevenSegNumFont);
		drawBackground(DISP_X_SIZE/2 - cfont.x_size/2, 180, cfont.x_size+1, cfont.y_size+1);
		setFont(BigFont);
	} else if (s == CENTS) {
		setFont(BigFont);
		if (prevErr < 0) {
			drawBackground(DISP_X_SIZE/2+prevErr, 104+cfont.y_size, prevErr+1, 15+1);
		} else if (prevErr >0) {
			drawBackground(DISP_X_SIZE/2, 104+cfont.y_size, prevErr+1, 15+1);
		} else {
			// no need to hide
		}
	} else {

	}
}

void paintPageHeader(char* hd) {
	setFont(BigFont);
	setColor(207, 216, 220);
	fillRect(0,0, DISP_X_SIZE, cfont.y_size+8);
	setColor(0,0,0);
	setColorBg(207, 216, 220);
	short size = 0;
	short i = 0;
	while (hd[i++] != '\0') {
		size++;
	}
	lcdPrint(hd, DISP_X_SIZE/2 - (size*cfont.x_size)/2, 4);
}

void updateTunerDisplay(char* dis) {
	setColorBg(96, 125, 139);

//	xil_printf("cents: %d\r\n", cents);
	unsigned int abs_cents = abs(cents) ;
//		xil_printf("abs cents: %d\r\n", abs_cents);
	if (abs_cents > 0 && abs_cents <= 15) {
		setColor(130, 255, 130);
	} else if (abs_cents > 15 && abs_cents <=40) {
		setColor(255, 234, 0);
	} else {
		setColor(255, 100, 100);
	}

	short size = 0;
		short i = 0;
		while (dis[i++] != '\0')
			size++;
		if (size*cfont.x_size != prev_tunerDisplay_width) {
			hideElement(TUNER_DISPLAY);
		}
		lcdPrint(dis, DISP_X_SIZE/2 - (size*cfont.x_size)/2, 100);
		prev_tunerDisplay_width = size*cfont.x_size;

//		updateCentsBar();
}

void updateCentsBar() {
	setColor(240, 141, 54);
	if (cents == 0) {
		hideElement(CENTS);
	} else if (cents < 0 ) {
		if (prevErr > 0) {
			hideElement(CENTS);
			fillRect(DISP_X_SIZE/2 + cents, 104+cfont.y_size, DISP_X_SIZE/2, 119+cfont.y_size);
		} else if (prevErr < 0){
			if (prevErr < cents) {
				drawBackground(DISP_X_SIZE/2 + prevErr, 104+cfont.y_size, cents-prevErr, 15);
			} else if (prevErr > cents) {
				fillRect(DISP_X_SIZE/2+cents, 104+cfont.y_size, DISP_X_SIZE/2 + prevErr, 119+cfont.y_size);
			} else {
				// not necessary
//				xil_printf("hello\r\n");
			}
		} else {

		}
	} else { //error > 0
		if (prevErr < 0) {
			hideElement(CENTS);
			fillRect(DISP_X_SIZE/2, 104+cfont.y_size, DISP_X_SIZE/2 + cents, 119+cfont.y_size);
		} else if (prevErr > 0) {
			if (prevErr < cents) {
				fillRect(DISP_X_SIZE/2 + prevErr, 104+cfont.y_size, cents-prevErr, 119+cfont.y_size);
			} else if (prevErr > cents) {
				drawBackground(DISP_X_SIZE/2+cents, 104+cfont.y_size, prevErr-cents, 15);
			} else {
				// not necessary
			}
		} else {

		}
	}
	prevErr = cents;
}

void paintMenu(short selected) {
//	octave selection
//	Freq detect
//	histogram
	if (selected < 0) selected = 0;
	if (selected >= (sizeof(menu_items)/4)) selected = (sizeof(menu_items)/4) - 1;
	setFont(BigFont);

	int rect_center_x = DISP_X_SIZE/2 - (8+cfont.x_size*9)/2;
	int rect_center_y =  DISP_Y_SIZE/2 - (4+(cfont.y_size+4)*(sizeof(menu_items)/4))/2;

	setXY(rect_center_x, rect_center_y, rect_center_x+8+cfont.x_size*9, rect_center_y+4+(cfont.y_size+4)*(sizeof(menu_items)/4));
	setColor(255,255,255);
	fillRect(rect_center_x, rect_center_y, rect_center_x+8+cfont.x_size*9, rect_center_y+4+(cfont.y_size+4)*(sizeof(menu_items)/4));
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	int center_x, center_y, size_word, j;
	for (int i= 0; i < sizeof(menu_items)/4; i++) {
		size_word = 0;
		j = 0;
		while (menu_items[i][j++] != '\0') {
			size_word++;
		}
		center_x = rect_center_x+(8+cfont.x_size*9)/2 - (size_word*cfont.x_size)/2;
		if (i == selected) {
			setColorBg(0, 176, 255);
			setColor(255,255,255);
			lcdPrint(menu_items[i], center_x, rect_center_y+4+(cfont.y_size+4)*i);
			setColorBg(255, 255, 255);
			setColor(0,0,0);
		} else {
			lcdPrint(menu_items[i], center_x, rect_center_y+4+(cfont.y_size+4)*i);
		}
	}
	clrXY();
}

void printMode(short freqMode) {//(char *st, int x, int y) {
//	setColor(0, 0, 0);
//	setColorBg(255, 255, 255);
//	setFont(BigFont);
//	lcdPrint(st, x, y);
	setColorBg(96, 125, 139);
	setColor(255,255,255);
	if (!freqMode) {
		lcdPrint("Mode:Frequency", DISP_X_SIZE/2 - 14*cfont.x_size/2, 180);
	} else {
		lcdPrint("Mode:Note", DISP_X_SIZE/2 - 9*cfont.x_size/2, 180);
	}

}

void printHistogramBackground() {
	setFont(BigFont);
	setColor(255,255,255);
	fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

void printWhiteBar(int x, int y, int width, int height) {
	setColor(255, 255, 255);
	fillRect(y, x, y + height, x+width);
}

void printHistogram() {
	setFont(BigFont);
	int n = 128;//samples/avg;
	int log_max = 0;
		for (int i = 0; i < n; i++) {
			if (new_[i] > log_max) {
				log_max = new_[i];
			}
		}

		int offset =20;
		int j = 0;
		for (int i = 0; i < n; i++) {
//			int plot_width = DISP_Y_SIZE-(2*cfont.y_size + 12);
//			int plot_height = DISP_X_SIZE/2;
//			xil_printf("new_[i]: %d \r\n",(int) new_copy[i]);
			if (new_copy[i] < log_prev_counts[i]) {
//				printBar((plot_width/n)*i, cfont.x_size + 8,plot_width/n, plot_height*(log_val/log_max));
				printWhiteBar(offset, ((int)log(log_prev_counts[i]))<<4, 1, (((int)log(new_copy[j]))<<4)-(((int)log(log_prev_counts[i]))<<4));
				log_prev_counts[i] = new_copy[i];
			} else if (new_copy[i] > log_prev_counts[i]) {
				printBar(offset,  (((int)log(new_copy[j]))<<4), 1, (((int)log(log_prev_counts[i]))<<4)-(((int)log(new_copy[j]))<<4));
				log_prev_counts[i] = new_copy[i];
			} else {

			}
			offset += 2;
			j += 1;
		}

		prev_log_max = log_max;
//		printHistInfo();
//	}
}

//{
//	if(AOtuner.state == 4)//if in histogram
//	    {
//	        //printShape(65, 100 , 100, 200, &triangle);
//	        int offset = 0;
//	        if(runOnce == 0)
//	        {
//	            for(int i = 0; i < 128; i+=2)
//	            printShape(30, offset, (((int)log(new[i]))<<2), 2, &returnOne);
//	            offset+=2;
//	            runOnce = 1;
//	        }
//	        else{
//	        int j = 0;
//	        for(int i = 0; i < 128; i++)
//	            {
//	                if(screenValues[i] < new[j])
//	                {
//	                    printShape(( ((int)log(screenValues[i]))<<2 ), offset, (((int)log(new[j]))<<2)-(((int)log(screenValues[i]))<<2), 2, &returnOne);
//	                }
//	                else if(screenValues[i] > new[j])
//	                {
//	                    printShape((((int)log(new[j]))<<2), offset, (((int)log(screenValues[i]))<<2)-(((int)log(new_[j]))<<2), 2, &triangle);
//	                }
//	                j+=2;
//	                offset+=2;
//	            }
//	        }
//	    }
//	}
//}

void printBar(int x, int y, int width, int height) {
	setColor(50, 50, 255);
	fillRect(y, x, y + height, x+width);
}

void printHistInfo() {
	char pre[21] = "Max Frequency:";
	char max_str[3];
	char units[] = "Hz";
	int max_str_size = 0;
	int i = 0;

	int start_x = DISP_X_SIZE/2 - (21*cfont.x_size)/2;// DISP_X_SIZE/2 - (max_str_size*cfont.x_size)/2
	int start_y = 4;
	setColorBg(255,255,255);
	setColor(0,0,0);

	sprintf(max_str, "%d", (1 << prev_log_max));
	strcat(pre, max_str);
	strcat(pre, units);

	while (pre[i++] != '\0')
		max_str_size++;
	lcdPrint(pre, start_x, start_y);

	setColorBg(96, 125, 139);
}

void printOctave(int oct) {
	setFont(SevenSegNumFont);
	char str_oct[2];
	setColorBg(96, 125, 139);
	setColor(255,255,255);
	sprintf(str_oct, "%d", oct);
	lcdPrint(str_oct, DISP_X_SIZE/2 - cfont.x_size/2, 180);
	setFont(BigFont);
}

// Select the font used by print() and printChar()
void setFont(u8* font)
{
	cfont.font=font;
	cfont.x_size = font[0];
	cfont.y_size = font[1];
	cfont.offset = font[2];
	cfont.numchars = font[3];
}


// Print a character
void printChar(u8 c, int x, int y)
{
    u8 ch;
    int i, j, pixelIndex;


    setXY(x, y, x + cfont.x_size - 1,y + cfont.y_size - 1);

    pixelIndex =
            (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
    for(j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
        ch = cfont.font[pixelIndex];
        for(i = 0; i < 8; i++) {
            if ((ch & (1 << (7 - i))) != 0)
                LCD_Write_DATA16(fch, fcl);
            else
                LCD_Write_DATA16(bch, bcl);
        }
        pixelIndex++;
    }

    clrXY();
}


// Print string
void lcdPrint(char *st, int x, int y)
{
    int i = 0;

    while(*st != '\0')
        printChar(*st++, x + cfont.x_size * i++, y);
}

