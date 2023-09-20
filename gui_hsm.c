/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "gui_hsm.h"
#include "lcd.h"
#include "trig.h"
#include "note.h"

char* arr[5] = {"1", "2", "3", "4", "5"};
short Auto = 1;
int octaveSelected = 2;
short frequencyRequired = 0;

typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
	short default_menu;
	short menu_cursor;
	short prev_vol;
	int a4_freq;
	short mode;
	short A4Freq_timeout;
	short mode_ticks_till_timeout;
	short mode_idle;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState LCD_initial (Lab2A *me);
static QState LCD_on      (Lab2A *me);
static QState LCD_Settings  (Lab2A *me);
static QState LCD_Idle  (Lab2A *me);
static QState LCD_Menu(Lab2A *me);
static QState LCD_Tuner(Lab2A *me);
static QState LCD_Hist(Lab2A *me);
static QState LCD_Octave(Lab2A *me);

/**********************************************************************/


Lab2A AO_Lab2A;


void LCD_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	me->default_menu = 0;
	me->menu_cursor = 0;
	me->prev_vol = 0;
	me->a4_freq = 440;
	me->A4Freq_timeout = 8;
	me->mode = 0; // 0 = frequency mode, 1 = note mode
	me->mode_ticks_till_timeout = 8;
	me->mode_idle = 1;
	Auto = 1;
	octaveSelected = 2;
	frequencyRequired = 0;
	QActive_ctor(&me->super, (QStateHandler)&LCD_initial);
}


QState LCD_initial(Lab2A *me) {
//	xil_printf("\n\rInitialization");
    return Q_TRAN(&LCD_on);
}


QState LCD_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			return Q_HANDLED();
			}

		case ENCODER_CLICK: {
			return Q_TRAN(&LCD_Menu);
		}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&LCD_Idle);
			}

//		case MODE: {
//			switch(Q_PAR(me)) {
//				case 1: {
////					printMode("Mode:1", 70, 140);
//					break;
//				}
//				case 2: {
////					printMode("Mode:2", 70, 140);
//					break;
//				}
//				case 3: {
//					if (me->mode) {
//						me->mode = 0;
//					} else {
//						me->mode = 1;
//					}
//					hideElement(TUNER_MODE);
//					printMode(me->mode);
//					break;
//				}
//				case 4: {
////					printMode("Mode:4", 70, 140);
//					break;
//				}
//				case 5: {
////					printMode("Mode:5", 70, 140);
//					break;
//				}
//			}
////			me->mode_ticks_till_timeout = 8;
////			me->mode_idle = 0;
//			return Q_HANDLED();
//		}
	}
	
	return Q_SUPER(&QHsm_top);
}

QState LCD_Menu(Lab2A *me) {
	switch(Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			paintPageHeader("Menu");
			paintMenu(me->menu_cursor);
			return Q_HANDLED();
		}

		case ENCODER_CLICK: {
//			xil_printf("to menu item %d", me->menu_cursor);
			switch(me->menu_cursor) {
				case 0: { // tuner
					return Q_TRAN(&LCD_Tuner);
				}

				case 1: { // settings
					return Q_TRAN(&LCD_Settings);
				}

				case 2: { // octave
					return Q_TRAN(&LCD_Octave);
				}

				case 3: { // histogram
					return Q_TRAN(&LCD_Hist);
				}
			}
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			if (me->menu_cursor < (sizeof(menu_items)/4)-1) {
				me->menu_cursor++;
				paintMenu(me->menu_cursor);
			}
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			if (me->menu_cursor > 0) {
				me->menu_cursor--;
				paintMenu(me->menu_cursor);
			}
			return Q_HANDLED();
		}

		case Q_EXIT_SIG: {
			hideElement(MENU);
//			hideElement(HEADER);
			return Q_HANDLED();
		}
	}
	return Q_SUPER(&LCD_on);
}

QState LCD_Octave(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			paintPageHeader("Octave Select");
			printOctave(octaveSelected);
			Auto = 0;
			frequencyRequired = 1;
			return Q_HANDLED();
		}

		case FREQ_READY: {
			QParam freq = Q_PAR(me);
//			xil_printf("frequency: %dHz\r\n", (int)freq);
			char* str_switch = notes[findNote(freq, me->a4_freq)];
			if (me->mode) {
				updateTunerDisplay(str_switch) ;
			} else {
				static char str_freq[8];
				static char units[] = "Hz";
				sprintf(str_freq, "%d",(int)freq);
				strcat(str_freq, units);
				updateTunerDisplay(str_freq);
			}

			return Q_HANDLED();
		}

		case ENCODER_UP: {
			if (octaveSelected < 7) {
				octaveSelected++;
				printOctave(octaveSelected);
			}
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			if (octaveSelected > 2) {
				octaveSelected--;
				printOctave(octaveSelected);
			}
			return Q_HANDLED();
		}

		case Q_EXIT_SIG: {
			Auto = 1;
			frequencyRequired = 0;
			hideElement(TUNER_DISPLAY);
			hideElement(OCTAVE);
			return Q_HANDLED();
		}
	}
	return Q_SUPER(&LCD_on);
}

QState LCD_Tuner(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			paintPageHeader("Tuner");
			frequencyRequired = 1;
			return Q_HANDLED();
		}

		case FREQ_READY: {
			QParam freq = Q_PAR(me);
//			xil_printf("frequency: %dHz\r\n", (int)freq);
			char* str_switch = notes[findNote(freq, me->a4_freq)];
			if (me->mode) {
				updateTunerDisplay(str_switch) ;
			} else {
				static char str_freq[8];
				static char units[] = "Hz";
				sprintf(str_freq, "%d",(int)freq);
				strcat(str_freq, units);
				updateTunerDisplay(str_freq);
			}

			return Q_HANDLED();
		}

		case Q_EXIT_SIG: {
			frequencyRequired = 0;
//			hideElement(HEADER);
			setFont(BigFont);
			return Q_HANDLED();
		}
	}
	return Q_SUPER(&LCD_on);
}

QState LCD_Hist(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
//			hideElement(HEADER);
			printHistogramBackground();
			frequencyRequired = 1;
			return Q_HANDLED();
		}

		case FREQ_READY: {
			printHistogram();
			break;
		}

		case Q_EXIT_SIG: {
			hideElement(HISTOBG);
			frequencyRequired = 0;
			return Q_HANDLED();
		}
	}
	return Q_SUPER(&LCD_on);
}

/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState LCD_Settings(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
//			xil_printf("Startup State A\n");
			paintPageHeader("Settings");
			printA4Freq(me->a4_freq);
			printMode(me->mode);
			me->A4Freq_timeout = 8;
			return Q_HANDLED();
		}

		case TICK: {
//			if (!me->mode_idle) {
//				if (me->mode_ticks_till_timeout <= 1) {
//					hideElement(0);
//					me->mode_idle = 1;
//				} else {
//					me->mode_ticks_till_timeout--;
//				}
//			}

			if (me->A4Freq_timeout <= 1) {
				hideElement(A4FREQ);
//				return Q_TRAN(&LCD_Idle);
			} else {
				me->A4Freq_timeout--;
			}
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			xil_printf("Encoder Up from State A\n");
			if (me->a4_freq >= 460)
				me->a4_freq = 460;
			else
				me->a4_freq++;
			me->A4Freq_timeout = 8;
//			printVolume(me->a4_freq, 0);
			printA4Freq(me->a4_freq);
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
//			xil_printf("Encoder Down from State A\n");
			if (me->a4_freq <= 420) {
				me->a4_freq = 420;
//				me->prev_vol = 0;
			} else {
				me->a4_freq--;
			}
			me->A4Freq_timeout = 8;
//			printVolume(me->a4_freq, 0);
			printA4Freq(me->a4_freq);
			return Q_HANDLED();
		}

		case MODE: {
			switch(Q_PAR(me)) {
				case 1: {
//					printMode("Mode:1", 70, 140);
					break;
				}
				case 2: {
//					printMode("Mode:2", 70, 140);
					me->mode = 1;
					hideElement(TUNER_MODE);
					printMode(me->mode);
					break;
				}
				case 3: {
					break;
				}
				case 4: {
//					printMode("Mode:4", 70, 140);
					me->mode = 0;
					hideElement(TUNER_MODE);
					printMode(me->mode);
					break;
				}
				case 5: {
//					printMode("Mode:5", 70, 140);
					break;
				}
			}
//			me->mode_ticks_till_timeout = 8;
//			me->mode_idle = 0;
			return Q_HANDLED();
		}

		case Q_EXIT_SIG: {
			hideElement(A4FREQ);
//			hideElement(HEADER);
			hideElement(TUNER_MODE);
			return Q_HANDLED();
		}
	}

	return Q_SUPER(&LCD_on);

}

QState LCD_Idle(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State B\n");
			drawBackground(0, 0, 240, 320);
			return Q_HANDLED();
		}

		case TICK: {
			if (!me->mode_idle) {
				if (me->mode_ticks_till_timeout <= 1) {
					hideElement(0);
					me->mode_idle = 1;
				} else {
					me->mode_ticks_till_timeout--;
				}
			}
			return Q_HANDLED();
		}

		case ENCODER_UP: {
//			xil_printf("Encoder Up from State A\n");
//			if (me->a4_freq >= 460)
//				me->a4_freq = 460;
//			else
//				me->a4_freq++;
//			return Q_TRAN(&LCD_Settings);
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
//			xil_printf("Encoder Down from State A\n");
//			if (me->a4_freq <= 420) {
//				me->a4_freq = 420;
////				me->prev_vol = 0;
//			} else {
//				me->a4_freq--;
//			}
//			return Q_TRAN(&LCD_Settings);
			return Q_HANDLED();
		}

		case Q_EXIT_SIG: {
			me->A4Freq_timeout = 8;
			return Q_HANDLED();
		}

	}

	return Q_SUPER(&LCD_on);

}

