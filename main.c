//-------------------------------------------------------------------
#include "dsk6713.h"
#include "dsk6713_dip.h"
#include "dsk6713_led.h"
#include "dsk6713_aic23.h"  	  						//codec-dsk support file

//-------------------------------------------------------------------
//-------------------------------------------------------------------

//-------------------------------------------------------------------
DSK6713_AIC23_Config config1 = {
	0x0017 , /* 0 Left line input channel volume */
	0x0017 , /* 1 Right line input channel volume */
	0x00F9 , /* 2 Left channel headphone volume */
	0x00F9 , /* 3 Right channel headphone volume */
	0x0011 , /* 4 Analog audio path control */
	0x0000 , /* 5 Digital audio path control */
	0x0000 , /* 6 Power down control */
	0x0043 , /* 7 Digital audio interface format *///
	0x0081 , /* 8 Sample rate control (48 kHz) */
	0x0001 /* 9 Digital interface activation */
};

DSK6713_AIC23_CodecHandle hCodec; // handler type variable

union { // every element represents same bits of data
	Uint32 uint;
	short channel[2];
	} AIC_data;

//---------------------------------------------------------------------------------------------

#define LEFT  1                  //data structure for union of 32-bit data
#define RIGHT 0                  //into two 16-bit data

#define MAX_GAIN 	0x79 		 // corresponds to 0dB

//-------------------------------------------------------------------


//-------------------------------------------------------------------
void main(void);
void output_sample(int out_data);
Uint32 input_sample(void);
void Display_Switches(char Switches[4]);

int buff[16];
int j=0;								// universal counter of samples


//-------------------------------------------------------------------

//-------------------------------------------------------------------
void main(void)
{
	int i=0;							// local counter for buffer

	CSL_init();

	char Switches[4];

	DSK6713_init();
	hCodec = DSK6713_AIC23_openCodec(0, &config1);			// defaults to 16-bit frame.

//	* (volatile int *) 0x0190000C = 0x000000A0;						// prefer a double 16-bit frame
//	* (volatile int *) 0x01900010 = 0x000000A0;

	Uint32 outGain = 0; // keeps track of DIP switch configuration
	DSK6713_DIP_init(); // doesn't do anything

	DSK6713_LED_init(); // initialize LED state machine
	Uint32 k, DIP_Concat;
	for(;;) // ever
	{
		// polling the DIP switches, returns Uint32
//		Switches = 0; // clears register for polling again
		for(k = 0; k < 4; k++){
			int temp = DSK6713_DIP_get(k);
			Switches[k] = ~(0xFE | temp);
		}

//		Switches = ~(0xFFFFFFF0|Switches); // MSb's will ~ back to 0, Switches previous value is not'd

		Display_Switches(Switches); // update the LEDs to reflect switches

		// Boundary condition: No switches pressed = mute left and right channels
		if(!(Switches[0] || Switches[1] || Switches[2] || Switches[3])){
			DSK6713_AIC23_mute(hCodec,1);
			// this activates the Soft Mute mode for the DAC
			// Digital Audio Path Control register
		} else {
			DSK6713_AIC23_mute(hCodec,0);
		}


		// outGain is a 7-bit value
		// according to AIC23 datasheet: 111 1111 = +6dB
		//								 000 0000 = -72dB
		// Goal: Max output = 0dB gain 		= 111 1001
		// 		 Min output = -15dB gain 	= 110 1010
		DIP_Concat = 0;
		for(k = 0; k < 3; k++){
			DIP_Concat += Switches[k] << k;
		}
		outGain = MAX_GAIN - DIP_Concat; // 4-bit value
		DSK6713_AIC23_outGain(hCodec, outGain);
		// Left/Right Channel Headphone Volume Control register

		// output gain register bits refresh every loop from polling, no interrupts used

		buff[i]= input_sample();
		output_sample(buff[i]);
		i++;
		j++;
		i &= 0xF;

	} // for(ever)
} // void main(void)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
void output_sample(int out_data)    //for out to Left and Right channels
{

	AIC_data.uint=out_data;          //32-bit data -->data structure

    while(!MCBSP_xrdy(DSK6713_AIC23_DATAHANDLE));//if ready to transmit

    MCBSP_write(DSK6713_AIC23_DATAHANDLE, AIC_data.uint);//write/output data

} // void output_sample(int out_data)
//-------------------------------------------------------------------


//-------------------------------------------------------------------
Uint32 input_sample(void)                      	  	//for 32-bit input
{
	while(!MCBSP_rrdy(DSK6713_AIC23_DATAHANDLE));//if ready to receive

	AIC_data.uint=MCBSP_read(DSK6713_AIC23_DATAHANDLE);//read data

	return(AIC_data.uint);
} // Uint32 input_sample(void)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
void Display_Switches(char Switches[4]){
	int i;
	for(i = 0; i < 4; i++){ // 4 LSb's represent the 4 switches being pressed
		if(Switches[i]){
			DSK6713_LED_on(i); // if the bit is true, turn the LED on
		} else {
			DSK6713_LED_off(i); // if the bit is false, turn the LED off
		}
	}
}
//-------------------------------------------------------------------
