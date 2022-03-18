#include <stdio.h>
#include <at89lp51rd2.h>
#include <math.h>
//#include "hardware.h"


// ~C51~ 
 
#define CLK 22118400L
#define BAUD 115200L
#define ONE_USEC (CLK/1000000L) // Timer reload for one microsecond delay
#define BRG_VAL (0x100-(CLK/(16L*BAUD)))

#define ADC_CE  P2_0
#define BB_MOSI P2_1
#define BB_MISO P2_2
#define BB_SCLK P2_3

#if (CLK/(16L*BAUD))>0x100
#error Can not set baudrate
#endif
//#define BRG_VAL (0x100-(CLK/(16L*BAUD)))

#define LCD_RS P3_2
//#define LCD_RW PX_X // Not used in this code.  Connect pin to GND
#define LCD_E  P3_3
#define LCD_D4 P3_4
#define LCD_D5 P3_5
#define LCD_D6 P3_6
#define LCD_D7 P3_7
#define CHARS_PER_LINE 16

unsigned char SPIWrite(unsigned char out_byte)
{
	// In the 8051 architecture both ACC and B are bit addressable!
	ACC=out_byte;
	
	BB_MOSI=ACC_7; BB_SCLK=1; B_7=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_6; BB_SCLK=1; B_6=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_5; BB_SCLK=1; B_5=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_4; BB_SCLK=1; B_4=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_3; BB_SCLK=1; B_3=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_2; BB_SCLK=1; B_2=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_1; BB_SCLK=1; B_1=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_0; BB_SCLK=1; B_0=BB_MISO; BB_SCLK=0;
	
	return B;
}

unsigned char _c51_external_startup(void)
{
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O

	P0M0=0x00; P0M1=0x00;    
	P1M0=0x00; P1M1=0x00;    
	P2M0=0x00; P2M1=0x00;    
	P3M0=0x00; P3M1=0x00;
	
	// Initialize the pins used for SPI
	ADC_CE=0;  // Disable SPI access to MCP3008
	BB_SCLK=0; // Resting state of SPI clock is '0'
	BB_MISO=1; // Write '1' to MISO before using as input
	
	// Configure the serial port and baud rate
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    #if (CLK/(16L*BAUD))>0x100
    #error Can not set baudrate
    #endif
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
    
	CLKREG=0x00; // TPS=0000B

    return 0;
}

void wait_us (unsigned char x)
{
	unsigned int j;
	
	TR0=0; // Stop timer 0
	TMOD&=0xf0; // Clear the configuration bits for timer 0
	TMOD|=0x01; // Mode 1: 16-bit timer
	
	if(x>5) x-=5; // Subtract the overhead
	else x=1;
	
	j=-ONE_USEC*x;
	TF0=0;
	TH0=j/0x100;
	TL0=j%0x100;
	TR0=1; // Start timer 0
	while(TF0==0); //Wait for overflow
}


void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) wait_us(250);
}


unsigned int volatile GetADC(unsigned char channel)
{
	unsigned int adc;
	unsigned char spid;

	ADC_CE=0; // Activate the MCP3008 ADC.
	
	SPIWrite(0x01);// Send the start bit.
	spid=SPIWrite((channel*0x10)|0x80);	//Send single/diff* bit, D2, D1, and D0 bits.
	adc=((spid & 0x03)*0x100);// spid has the two most significant bits of the result.
	spid=SPIWrite(0x00);// It doesn't matter what we send now.
	adc+=spid;// spid contains the low part of the result. 
	
	ADC_CE=1; // Deactivate the MCP3008 ADC.
		
	return adc;
}

#define VREF 4.096


void LCD_pulse (void)
{
	LCD_E=1;
	wait_us(40);
	LCD_E=0;
}

void LCD_byte (unsigned char x)
{
	// The accumulator in the 8051 is bit addressable!
	ACC=x; //Send high nible
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	LCD_pulse();
	wait_us(40);
	ACC=x; //Send low nible
	LCD_D7=ACC_3;
	LCD_D6=ACC_2;
	LCD_D5=ACC_1;
	LCD_D4=ACC_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E=0; // Resting state of LCD's enable is zero
	//LCD_RW=0; // We are only writing to the LCD in this program.  Connect pin to GND.
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}


float find_v(unsigned char channel)
{
	float max = 0.0;
	float current_voltage;
	current_voltage = (GetADC(channel)*VREF)/(1023.0);
	while(max<current_voltage)
	{
		if(current_voltage>max)
			max = current_voltage;
		current_voltage = (GetADC(channel)*VREF)/(1023.0);
	}
	return max;
}
/*Read 10 bits from the MCP3008 ADC converter*/

void main (void)
{
	float myof;
	float HalfPeriodR;
	float FrequencyR;
	float HalfPeriodT;
	float VoltageR;
	float VoltageT;
	float TimeDif;
	float Phase;
	float max;
	float current_voltage;
	float max1;
	float current_voltage1;
	char STRvoltageR[16];
//	char STRvoltageT[16];
	float flag;
	char ch = 223;
	
	LCD_4BIT();
	flag = 0.0;
	max = 0.0;
	max1 = 0.0;

	waitms(50);
	

	//Measure Reference Signal Half Period
	while(1){
	TR0 = 0;
	TMOD&=0B_1111_0000;
	TMOD|=0B_0000_0001;
	TH0=0; TL0=0; myof=0;
	TF0=0;
	while (P1_0==1);
	while (P1_0==0);
	TR0 = 1;
	printf("1\n");
	while (P1_0==1){
		if(TF0){
			TF0 =0;
			myof++;
		}
	}
	TR0=0;
	printf("2\n");
	
	HalfPeriodR = (myof*65536.0+TH0*256.0+TL0);
	HalfPeriodR = HalfPeriodR / CLK;
	
	FrequencyR = 1/(2.0*HalfPeriodR);
	
	//Measure Reference Signal Peak Voltage
	max1=0.0;
	while(P1_0==0);
	current_voltage1 = (GetADC(0)*VREF)/(1023.0);
	while(max1<=current_voltage1)
	{
		max1 = current_voltage1;
		current_voltage1 = (GetADC(0)*VREF)/(1023.0);
	}
	VoltageR = max1/1.4142;
	
	printf("peakVolts measured\n");
	
	printf("11getadc0: %f getadc1: %f\n",GetADC(0),GetADC(1));
	
	//Measure Test Signal Half Period
	TH0=0; TL0=0; myof=0;
	TF0=0;
	while (P1_1==1);
	while (P1_1==0);
	TR0 = 1;
	printf("starting next period measurement\n");
	while (P1_1==1){
		if(TF0){
			TF0 =0;
			myof++;
		}
	}
	TR0=0;
	
	HalfPeriodT = (myof*65536.0+TH0*256.0+TL0);
	
	HalfPeriodT = HalfPeriodT / CLK;
	
	printf("Other period measured\n");
	
	//Measure Test Signal Peak Voltage
	while(P1_1==0);
	max = 0.0;
	current_voltage = (GetADC(1)*VREF)/(1023.0);
	while(max<=current_voltage)
	{
		max = current_voltage;
		current_voltage = (GetADC(1)*VREF)/(1023.0);
		printf("max: %f\n", max);
	}
	VoltageT = (max);
	
	//Phase Measurment
	TH0=0; TL0=0; myof=0;
	TF0=0;
	while(P1_0==1);
	while(P1_0==0);
	TR0=1;
	
	while(P1_0==1){
	
		if(TF0){
			TF0 =0;
			myof++;
		}
		
		if(P1_1 == 1){
			break;
		}
	}
	TR0=0;  
	
	TimeDif = (myof*65536.0+TH0*256.0+TL0);
	TimeDif = TimeDif / CLK;
	
	Phase = TimeDif*360.0/(HalfPeriodR*2.0);
	
	if(Phase < 1.0){
		TH0=0; TL0=0; myof=0;
		TF0=0;
		while(P1_1==1);
		while(P1_1==0);
		TR0=1;
	
		while(P1_1==1){
	
		if(TF0){
			TF0 =0;
			myof++;
		}
		
		if(P1_0 == 1){
			break;
		}
	}
		
		TimeDif = (myof*65536.0+TH0*256.0+TL0);
		TimeDif = TimeDif / CLK;
	
		Phase = -1.0*(TimeDif*360.0/(HalfPeriodR*2.0));
		
	}
	
	Phase = -1.0 * Phase;
	//printf("myof: %f\n", TimeDif);
	
	if(flag == 1.0){
		sprintf(STRvoltageR,"R:%.2fV %.0fHz ",VoltageR,FrequencyR);
		LCDprint(STRvoltageR,1,0);
		flag--;
	}
	
	else{
		sprintf(STRvoltageR,"T:%.2fV %1.1f%1.1c     ",VoltageT,Phase,ch);
		LCDprint(STRvoltageR,2,0);
		flag++;
	}
	
	
	
	printf("HalfPeriod: %f Frequency: %f Reference RMS Voltage: %f Test RMS Voltage: %f Phase: %f\n", HalfPeriodR, FrequencyR, VoltageR, VoltageT, Phase);
	waitms(100);
	}
	
}