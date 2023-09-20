/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xgpio.h" 		// LED driver, used for General purpose I/i
#include "xspi.h"
#include "xspi_l.h"
#include "gui_hsm.h"
#include "lcd.h"
#include "trig.h"
#include "note.h"
#include "fft.h"
#include "stream_grabber.h"

/*****************************/

/* Define all variables and Gpio objects here  */

#define GPIO_CHANNEL1 1

#define RESET_VALUE 0x017D7840	// @ 1MHz = 0.25 seconds

#define STATE_MASK 3

void debounceInterrupt(); // Write This function
void debounceTwistInterrupt(int next_state);

static volatile u8 state = 3;

static XIntc intc;
static XTmrCtr tmr;

static XGpio encoder;
static XGpio btns;

static XGpio dc;
static XSpi spi;
XSpi_Config *spiConfig;
static short dir = 0; //0 = CCW, 1 = CW

// Create ONE interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

/*..........................................................................*/
void BSP_init(void) {
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

	// Press Knob

	// Twist Knob

	XStatus status;
	u32 controlReg;
	status = XST_FAILURE;

	status = XIntc_Initialize(&intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller initialization failed.\r\n");
		return XST_FAILURE;
	}

	status = XTmrCtr_Initialize(&tmr, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	status = XIntc_Connect(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID, (XInterruptHandler)TimerHandler, &tmr);
	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller connection to AXI Timer 0 failed.\r\n");
		return XST_FAILURE;
	}

	status = XIntc_Start(&intc, XIN_REAL_MODE);

	if (status != XST_SUCCESS){
		xil_printf("Failed to start Interrupt Controller.\r\n");
		return XST_FAILURE;
	}

	XIntc_Enable(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID);
	XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);
	XIntc_Enable(&intc, XPAR_INTC_0_GPIO_1_VEC_ID);


	XTmrCtr_SetOptions(&tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION ); //| XTC_AUTO_RELOAD_OPTION
	XTmrCtr_SetResetValue(&tmr, 0, 0xFFFFFFFF-RESET_VALUE);

	if (status != XST_SUCCESS){
		xil_printf("Failed to initialize Timer.\r\n");
		return XST_FAILURE;
	}


	if (status != XST_SUCCESS) {
		xil_printf("Failed to enable interrupts.\r\n");
		return XST_FAILURE;
	}
	xil_printf("Interrupts enabled!\r\n");

	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
//	microblaze_register_handler((XInterruptHandler)TwistHandler, (void*)XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
//	microblaze_register_handler((XInterruptHandler)GpioHandler, (void*)XPAR_AXI_GPIO_BTN_DEVICE_ID);
	microblaze_enable_interrupts();

	// Initialize Encoder
	status = XGpio_Initialize(&encoder, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	status = XIntc_Connect(&intc, XPAR_INTC_0_GPIO_0_VEC_ID, (XInterruptHandler)TwistHandler, &encoder);
	status = XIntc_Connect(&intc, XPAR_INTC_0_GPIO_1_VEC_ID, (XInterruptHandler)GpioHandler, &btns);
	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller connection to GPIO button failed.\r\n");
		return XST_FAILURE;
	}

	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller connection to GPIO button failed.\r\n");
		return XST_FAILURE;
	}

	XGpio_InterruptEnable(&encoder, 0x1);
	XGpio_InterruptGlobalEnable(&encoder);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to enable button interrupts");
		return XST_FAILURE;
	}

	status = XGpio_Initialize(&btns, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to initialize buttons.");
		return XST_FAILURE;
	}

	XGpio_InterruptEnable(&btns, 1);
	XGpio_InterruptGlobalEnable(&btns);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to enable button interrupts");
		return XST_FAILURE;
	}

	status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO dc fail!\n");
		return XST_FAILURE;
	}

	/*
	 * Set the direction for all signals to be outputs
	 */
	XGpio_SetDataDirection(&dc, 1, 0x0);

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 */
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
		return XST_DEVICE_NOT_FOUND;
	}

	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
		return XST_FAILURE;
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&spi);

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	initLCD();
	clrScr();

	xil_printf("Successfully set up peripherals and interrupts.");
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	drawBackground(0, 0, 240, 320);
	stream_grabber_start();
	XTmrCtr_Start(&tmr, 0);
}


void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();                       /* unlock interrupts */
    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM
    	static int l;
    	static float sample_f;
    	static float  frequency_i;
    	static float q[SAMPLES];
    	static float w[SAMPLES];
    	if (frequencyRequired) {
//    		xil_printf("idle");
    		if (Auto) {
				if (octave == 2) {
					samples = 4096;
					m = 5;
					avg = 128;
				} else if (octave == 3) {
					samples = 4096;
					m = 6;
					avg = 64;
				} else if (octave == 4) {
					samples = 4096;
					m = 7;
					avg = 32;
				} else if (octave == 5) {
					samples = 2048;
					m = 7;
					avg = 16;
				} else if (octave == 6) {
					samples = 1024;
					m = 7;
					avg = 8;
				} else if (octave == 7) {
					samples = 1024;
					m = 8;
					avg = 4;
				} else  {

				}
    		} else {
				if (octaveSelected == 2) {
					samples = 4096;
					m = 5;
					avg = 128;
				} else if (octaveSelected == 3) {
					samples = 4096;
					m = 6;
					avg = 64;
				} else if (octaveSelected == 4) {
					samples = 4096;
					m = 7;
					avg = 32;
				} else if (octaveSelected == 5) {
					samples = 4096;
					m = 8;
					avg = 16;
				} else if (octaveSelected == 6) {
					samples = 1024;
					m = 7;
					avg = 8;
				} else if (octaveSelected == 7) {
					samples = 1024;
					m = 8;
					avg = 4;
				} else  {

				}
    		}
    		read_fsl_values(q, samples);

    		 sample_f = 100*1000*1000/2048.0;

			  for(l=0;l<SAMPLES;l++)
				 w[l]=0;

			  frequency_i=fft(q, w, samples/avg, m, sample_f/avg);
//			xil_printf(convert);
//			xil_printf("fft\r\n");
//			  updateTunerDisplay(notes[findNote(frequency, 440)], 0);
    		QActive_postISR((QActive *)&AO_Lab2A, FREQ_READY, (QParam)(frequency_i+0.5));
//    		QActive_postISR((QActive *)&AO_Lab2A, HISTO_READY, (QParam)new_);
    	}
}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void GpioHandler(void *CallbackRef) {
		XGpio_InterruptDisable(&btns, 1);
		short mode = 0;

		int buttonPressed = XGpio_DiscreteRead(&btns, 1);
		if (buttonPressed == 1) {
			// up button = MODE1
			mode = 1;
		} else if (buttonPressed == 2) {
			// left button = MODE2
			mode = 2;
		} else if (buttonPressed == 4) {
			// right button = MODE4
			mode = 4;
		} else if (buttonPressed == 8) {
			// down button = MODE5
			mode = 5;
		} else if (buttonPressed == 16) {
			// center button = MODE3
			mode = 3;
		}

		QActive_postISR((QActive *)&AO_Lab2A, MODE, mode);

		XGpio_InterruptClear(&btns, 1);
		XGpio_InterruptEnable(&btns, 1);
}

void TwistHandler(void *CallbackRef) {
	int next_state = XGpio_DiscreteRead(&encoder, 1);
	if ((next_state & 4) == 4) { //encoder click
		debounceInterrupt();
	} else { // encoder twist
		debounceTwistInterrupt(next_state);
	}
}

void TimerHandler(void *CallbackRef) {
	Xuint32 ControlStatusReg;

	QActive_postISR((QActive *)&AO_Lab2A, TICK, 0);

	ControlStatusReg = XTimerCtr_ReadReg(tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(tmr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);
}

void debounceTwistInterrupt(int next_state){
	// Read both lines here? What is twist[0] and twist[1]?
	// How can you use reading from the two GPIO twist input pins to figure out which way the twist is going?

	if (next_state == 0) { // ABC = 000; AB = 00; BC = 00
		state = next_state & STATE_MASK;
	} else if (next_state == 2) { // ABC = 010; BC = 10
		if (state == 3) {
			dir = 0;
		}
		state = next_state & STATE_MASK;
	} else if (next_state == 1) { // ABC = 100; BC = 01
		if (state == 3) {
			dir = 1;
		}
		state = next_state & STATE_MASK;
	} else if (next_state == 3) { // ABC = 110; AB = 11; BC = 11
		if (state == 1) {
			if (dir == 0){
				QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN, 0);
			}
		} else if (state == 2) {
			if (dir == 1){
				QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP, 0);
			}
		}
		state = next_state & STATE_MASK;
	}
	XGpio_InterruptClear(&encoder, 1);
}

void debounceInterrupt() {

	for(int i = 0; i < 5000000; i++);
//	XGpio_InterruptClear(&encoder, 1);
//	XIntc_Acknowledge(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);
	QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK, 0);
	 XGpio_InterruptClear(&btns, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}

void read_fsl_values(float* q, int n) {
   int i;
   unsigned int x;
   int sum;

   stream_grabber_wait_enough_samples(n);

   sum = 0;

   for(i = 0; i < n; i++) {
	   sum = sum + stream_grabber_read_sample(i);
      if ((i+1) % AVG == 0)
      {
    	  x = sum;
    	  q[i/AVG] = 3.3*x/67108864.0; // 3.3V and 2^26 bit precision.
    	  sum = 0;
      }
   }
   stream_grabber_start();
}
