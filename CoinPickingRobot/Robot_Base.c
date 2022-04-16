#include <XC.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK
#pragma config FSOSCEN = OFF        // Turn off secondary oscillator on A4 and B4

// Defines
#define SYSCLK 40000000L
#define FREQ 100000L // We need the ISR for timer 1 every 10 us
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

/* Pinout for DIP28 PIC32MX130:

                                   MCLR (1   28) AVDD 
  VREF+/CVREF+/AN0/C3INC/RPA0/CTED1/RA0 (2   27) AVSS 
        VREF-/CVREF-/AN1/RPA1/CTED2/RA1 (3   26) AN9/C3INA/RPB15/SCK2/CTED6/PMCS1/RB15
   PGED1/AN2/C1IND/C2INB/C3IND/RPB0/RB0 (4   25) CVREFOUT/AN10/C3INB/RPB14/SCK1/CTED5/PMWR/RB14
  PGEC1/AN3/C1INC/C2INA/RPB1/CTED12/RB1 (5   24) AN11/RPB13/CTPLS/PMRD/RB13
   AN4/C1INB/C2IND/RPB2/SDA2/CTED13/RB2 (6   23) AN12/PMD0/RB12
     AN5/C1INA/C2INC/RTCC/RPB3/SCL2/RB3 (7   22) PGEC2/TMS/RPB11/PMD1/RB11
                                    VSS (8   21) PGED2/RPB10/CTED11/PMD2/RB10
                     OSC1/CLKI/RPA2/RA2 (9   20) VCAP
                OSC2/CLKO/RPA3/PMA0/RA3 (10  19) VSS
                         SOSCI/RPB4/RB4 (11  18) TDO/RPB9/SDA1/CTED4/PMD3/RB9
         SOSCO/RPA4/T1CK/CTED9/PMA1/RA4 (12  17) TCK/RPB8/SCL1/CTED10/PMD4/RB8
                                    VDD (13  16) TDI/RPB7/CTED3/PMD5/INT0/RB7
                    PGED3/RPB5/PMD7/RB5 (14  15) PGEC3/RPB6/PMD6/RB6
                    
                                        (1   28) AVDD 
  									    (2   27) AVSS 
  									    (3   26) AN9/C3INA/RPB15/SCK2/CTED6/PMCS1/RB15
  									    (4   25) CVREFOUT/AN10/C3INB/RPB14/SCK1/CTED5/PMWR/RB14
  									    (5   24) AN11/RPB13/CTPLS/PMRD/RB13
  									    (6   23) AN12/PMD0/RB12
  									    (7   22) PGEC2/TMS/RPB11/PMD1/RB11
  									VSS (8   21) PGED2/RPB10/CTED11/PMD2/RB10
  									    (9   20) VCAP
  									    (10  19) VSS
  									    (11  18) TDO/RPB9/SDA1/CTED4/PMD3/RB9
  									    (12  17) TCK/RPB8/SCL1/CTED10/PMD4/RB8
  									 VDD(13  16) TDI/RPB7/CTED3/PMD5/INT0/RB7
  									    (14  15) PGEC3/RPB6/PMD6/RB6
                 
*/

volatile int ISR_pwm1=150, ISR_pwm2=150, ISR_cnt=0, i, servoflag;

// The Interrupt Service Routine for timer 1 is used to generate one or more standard
// hobby servo signals.  The servo signal has a fixed period of 20ms and a pulse width
// between 0.6ms and 2.4ms.
void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1_Handler(void)
{
	IFS0CLR=_IFS0_T1IF_MASK; // Clear timer 1 interrupt flag, bit 4 of IFS0

	ISR_cnt++;
	if(ISR_cnt==ISR_pwm1)
	{
		LATBbits.LATB0 = 0;
	}
	if(ISR_cnt==ISR_pwm2)
	{
		LATBbits.LATB1 = 0;
	}
	if(ISR_cnt>=2000)
	{
    	ISR_cnt=0; // 2000 * 10us=20ms
		LATBbits.LATB0 = 1;
		LATBbits.LATB1 = 1;
	}
}

void SetupTimer1 (void)
{
	// Explanation here: https://www.youtube.com/watch?v=bu6TTZHnMPY
	__builtin_disable_interrupts();
	PR1 =(SYSCLK/FREQ)-1; // since SYSCLK/FREQ = PS*(PR1+1)
	TMR1 = 0;
	T1CONbits.TCKPS = 0; // 3=1:256 prescale value, 2=1:64 prescale value, 1=1:8 prescale value, 0=1:1 prescale value
	T1CONbits.TCS = 0; // Clock source
	T1CONbits.ON = 1;
	IPC1bits.T1IP = 5;
	IPC1bits.T1IS = 0;
	IFS0bits.T1IF = 0;
	IEC0bits.T1IE = 1;
	
	INTCONbits.MVEC = 1; //Int multi-vector
	__builtin_enable_interrupts();
}

// Use the core timer to wait for 1 ms.
void wait_1ms(void)
{
    unsigned int ui;
    _CP0_SET_COUNT(0); // resets the core timer count

    // get the core timer count
    while ( _CP0_GET_COUNT() < (SYSCLK/(2*1000)) );
}

void waitms(int len)
{
	while(len--) wait_1ms();
}

#define PIN_PERIOD (PORTB&(1<<5))

// GetPeriod() seems to work fine for frequencies between 200Hz and 700kHz.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}

    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}
	
    _CP0_SET_COUNT(0); // resets the core timer count
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
	}

	return  _CP0_GET_COUNT();
}
 
void UART2Configure(int baud_rate)
{
    // Peripheral Pin Select
    U2RXRbits.U2RXR = 4;    //SET RX to RB8
    RPB9Rbits.RPB9R = 2;    //SET RB9 to TX

    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(baud_rate); // U2BRG = (FPb / (16*baud)) - 1
    
    U2MODESET = 0x8000;     // enable UART2
}

void uart_puts(char * s)
{
	while(*s)
	{
		putchar(*s);
		s++;
	}
}

char HexDigit[]="0123456789ABCDEF";
void PrintNumber(long int val, int Base, int digits)
{ 
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (val>0) | (digits>0) )
	{
		buff[j--]=HexDigit[val%Base];
		val/=Base;
		if(digits!=0) digits--;
	}
	uart_puts(&buff[j+1]);
}

// Good information about ADC in PIC32 found here:
// http://umassamherstm5.org/tech-tutorials/pic32-tutorials/pic32mx220-tutorials/adc
void ADCConf(void)
{
    AD1CON1CLR = 0x8000;    // disable ADC before configuration
    AD1CON1 = 0x00E0;       // internal counter ends sampling and starts conversion (auto-convert), manual sample
    AD1CON2 = 0;            // AD1CON2<15:13> set voltage reference to pins AVSS/AVDD
    AD1CON3 = 0x0f01;       // TAD = 4*TPB, acquisition time = 15*TAD 
    AD1CON1SET=0x8000;      // Enable ADC
}

int ADCRead(char analogPIN)
{
    AD1CHS = analogPIN << 16;    // AD1CHS<16:19> controls which analog pin goes to the ADC
 
    AD1CON1bits.SAMP = 1;        // Begin sampling
    while(AD1CON1bits.SAMP);     // wait until acquisition is done
    while(!AD1CON1bits.DONE);    // wait until conversion done
 
    return ADC1BUF0;             // result stored in ADC1BUF0
}

void ConfigurePins(void)
{
    // Configure pins as analog inputs
    ANSELAbits.ANSA0 = 1;   // set RB2 (AN4, pin 6 of DIP28) as analog pin
    TRISAbits.TRISA0 = 1;   // set RB2 as an input
    ANSELAbits.ANSA1 = 1;   // set RB3 (AN5, pin 7 of DIP28) as analog pin
    TRISAbits.TRISA1 = 1;   // set RB3 as an input
    
	// Configure digital input pin to measure signal period
	ANSELB &= ~(1<<5); // Set RB5 as a digital I/O (pin 14 of DIP28)
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5
    
    // Configure output pins
	
	//Electromagnet Control Pins
	TRISAbits.TRISA1 = 0; // pin  3 of DIP28
	
	//Servo Control Pins
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	
	//H-Bridge Control Pins
	TRISAbits.TRISA0 = 0; // pin  2 of DIP28
	TRISAbits.TRISA2 = 0; // pin  9 of DIP28

	TRISBbits.TRISB4 = 0; // pin 11 of DIP28
	TRISAbits.TRISA4 = 0; // pin 12 of DIP28
	
	INTCONbits.MVEC = 1;
}


//int iscoindetected(){
//int thetruth;


//}

/*
int isCoinDetected(){

	unsigned long int count, f = 0, f_new = 0, min = 0, max = 0, avg = 0, cnt = 0;
		count=GetPeriod(100);
		if(count>0)
		{
			if (f == 0) {
				while (cnt < 100) {
					waitms(200);
					cnt++;
					count = GetPeriod(100);
					avg += ((SYSCLK / 2L) * 100L) / count;
				}
				f = (avg / cnt);
			}

			f_new = ((SYSCLK / 2L) * 100L) / count;
			uart_puts("f_new=");
			PrintNumber(f_new, 10, 7);
			uart_puts("  f=");
			PrintNumber(f, 10, 7);
			uart_puts("\n\r");
			//	uart_puts("Hz\n");
				//uart_puts("          \r"); //overides current line of terminal

			if ((f_new * 10000 / f) < 10045 && (double)(f_new * 10000 / f) > 9955) { //fluctuation is between 18670 and 18690
			//no coin detected
				printf("No coin detected!\n\n\r");
			}
		else
		{
			uart_puts("NO SIGNAL                     \r");
		}

}

void miniServoRoutine();

 void zservocontrol(int currentpos, int destinationpos, int speed){

    if (currentpos< destinationpos){
	for(i=current pos; i<=destinationpos; ++i){
  
 	pw =i;
 	ISR_pw1=pw;

 	delay_ms(speed);
   
	}
		}
 
 else {
 for(i=currentpos; i<=destinationpos; --i){
  
 	pw =i;
 	ISR_pw1=pw;

 	delay_ms(speed);

 }
 
 
 return newcurrentpos;
}
void xservocontrol(int pos, int speed);*/


//0-forward, 1-reverse, 2-rotate right, 3-rotate left, 4-stop
void drive(int directive){
	switch (directive){
		case 0:
			LATAbits.LATA2 = 0;
			LATAbits.LATA0 = 1;
			LATAbits.LATA4 = 0;
			LATBbits.LATB4 = 1;
			break;
		case 1:
			LATAbits.LATA2 = 1;
			LATAbits.LATA0 = 0;
			LATAbits.LATA4 = 1;
			LATBbits.LATB4 = 0;
			break;
		case 2:
			LATAbits.LATA2 = 1;
			LATAbits.LATA0 = 0;
			LATAbits.LATA4 = 0;
			LATBbits.LATB4 = 1;
			break;
		case 3:
			LATAbits.LATA2 = 0;
			LATAbits.LATA0 = 1;
			LATAbits.LATA4 = 1;
			LATBbits.LATB4 = 0;
			break;
		case 4:
			LATAbits.LATA2 = 0;
			LATAbits.LATA0 = 0;
			LATAbits.LATA4 = 0;
			LATBbits.LATB4 = 0;
			break;
		default:
			break;
	}

}

void servocontrol() {

	int startval = 80;
	int endval = 240;
	int pw;
	
	drive(1);
	waitms(20);
	drive(4);
	
	
	LATAbits.LATA1 = 1;
	
	//lower
	for (i = 90; i <= 240; ++i) {
		pw = i;
		ISR_pwm1 = pw;
		waitms(5);
	}
	
	
	//sweeping motion
	for (i = 160; i <= 240; ++i) {
		pw = i;
		ISR_pwm2 = pw;
		waitms(5);
	}
	
	//readjust for raising
	for (i = 240; i >= 170; --i) {
		pw = i;
		ISR_pwm2 = pw;
		waitms(5);
	}
	
	//raising coin
	for (i = 240; i >= 90; --i) {
		pw = i;
		ISR_pwm1 = pw;
		waitms(5);
	}
	
	for (i = 170; i >= 120; --i) {
		pw = i;
		ISR_pwm2 = pw;
		waitms(5);
	}
	
	LATAbits.LATA1 = 0;
	
	
	//return to base 
	for (i = 120; i <= 160; ++i) {
		pw = i;
		ISR_pwm2 = pw;
		waitms(5);
	}
	
	
	/*for (i = 90; i <= 150; ++i) {
		pw = i;
		ISR_pwm1 = pw;
		waitms(5);
	}*/
	
	
	//ISR_pwm1 = 100;
	//ISR_pwm2 = 240;
}

void coinPickUp() {
	servocontrol();
}

void escapeRoutine() {
	drive(1);
	waitms(1000);
	drive(2);
	waitms(1000);
}

int perimeterReached(int pin) {
	int threshold = 500;

	int adcval;
	long int v;
	adcval = ADCRead(pin);
	uart_puts("ADC[5]=0x");
	PrintNumber(adcval, 16, 3);
	uart_puts(", V=");
	v = (adcval * 3290L) / 1023L; // 3.290 is VDD
	PrintNumber(v / 1000, 10, 1);
	uart_puts(".");
	PrintNumber(v % 1000, 10, 3);
	uart_puts("V ");

	if (v >= threshold){
		return 1;
	}
	else{
		return 0;
	}
}

// Final Code
// In order to keep this as nimble as possible, avoid
// using floating point or printf() on any of its forms!
void main(void)
{

	volatile unsigned long t = 0;
	int adcval;
	long int v;
	int coincounter = 0;

	//coin detector:
	//unsigned long int count, f = 0, f_new = 0, min = 0, max = 0, avg = 0, cnt = 0;

	unsigned char LED_toggle = 0;

	//Variable for Coin-Detection
	// (INPUT)
	int coinflag;       // 0 if no coin detected
						// 1 if a coin is detected
	//Variable for Perimeter-Detection
	// (INPUT)				
	int perimeterflag;  // 0 if no perimeter detected
						// 1 if perpendicular perimeter detected
						// 2 if parallel perimeter detected

	//Variables for H-Bridge Control - To be set to appropriate pins
	//controlled in drive function
	// (OUTPUTS)
	//int rightForward; -> LATAbits.LATA2 (pin 9)
	//int rightReverse; -> LATAbits.LATA0 (pin 2)
	//int leftForward;  -> LATAbits.LATA4 (pin 12)
	//int leftReverse;  -> LATAbits.LATB4 (pin 11)

	//Variable for Electromagnet Control - To be set to appropriate pin
	//controlled in coinPickup function
	// (OUTPUT)
	int magnetOn;

	CFGCON = 0;

	UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
	ConfigurePins();
	SetupTimer1();
	drive(4);
	ADCConf(); // Configure ADC

	waitms(500); // Give PuTTY time to start
	uart_puts("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.

	int count, f = 0, f_new = 0, min = 0, max = 0, avg = 0, cnt = 0, thetruth = 0;
	perimeterflag = 0, min;
    int minval, min2, valcompare=0;
	
	while (1) {
	    ISR_pwm1 = 90;
		ISR_pwm2 = 160;
		count = GetPeriod(100);
		if (count > 0){
			
			if (f_new ==0){
		 		valcompare =((SYSCLK / 2L) * 100L) / count;
		 	}
			f_new = ((SYSCLK / 2L) * 100L) / count;
			uart_puts("f_new=");
			PrintNumber(f_new, 10, 7);
			uart_puts("  val=");
			PrintNumber(valcompare, 10, 7);
			uart_puts("\n\r");
			if (f_new<(valcompare+200)){ //&& (double)(f_new * 10000 / f) > 9975) { //fluctuation is between 18670 and 18690
				//no coin detected
				printf("No coin detected!\n\n\r");
				thetruth = 0;
			}
			else {
				//must stop everything, measure an avg of new 
				//frequencies for precice determination of coin type
				printf("Coin detected!\n\n\r");
				//waitms(200);
				thetruth = 1;
			}
		}
		else
		{
			uart_puts("NO SIGNAL                     \r");
		}

		
		if (thetruth == 1) {
	
			servocontrol();
		//	f_new=0;
		}
		thetruth = 0;

		//servocontrol();
		//Parallel Perimeter Check
		perimeterflag += perimeterReached(4);

		//Perpendicular Perimeter Check
		perimeterflag += perimeterReached(5);
		
		if(perimeterflag != 0){
			escapeRoutine();
		}
		
		perimeterflag = 0;
		drive(0);//to be drive(0)
		waitms(20);
	}

    
	/*


	uart_puts("\r\nPIC32 multi I/O example.\r\n");
	uart_puts("Measures the voltage at channels 4 and 5 (pins 6 and 7 of DIP28 package)\r\n");
	uart_puts("Measures period on RB5 (pin 14 of DIP28 package)\r\n");
	uart_puts("Toggles RA0, RA1, RB0, RB1, RA2 (pins 2, 3, 4, 5, 9, of DIP28 package)\r\n");
	uart_puts("Generates Servo PWM signals at RA3, RB4 (pins 10, 11 of DIP28 package)\r\n\r\n");
	while (1)
	{
		adcval = ADCRead(4); // note that we call pin AN4 (RB2) by it's analog number
		uart_puts("ADC[4]=0x");
		PrintNumber(adcval, 16, 3);
		uart_puts(", V=");
		v = (adcval * 3290L) / 1023L; // 3.290 is VDD
		PrintNumber(v / 1000, 10, 1);
		uart_puts(".");
		PrintNumber(v % 1000, 10, 3);
		uart_puts("V ");

		adcval = ADCRead(5);
		uart_puts("ADC[5]=0x");
		PrintNumber(adcval, 16, 3);
		uart_puts(", V=");
		v = (adcval * 3290L) / 1023L; // 3.290 is VDD
		PrintNumber(v / 1000, 10, 1);
		uart_puts(".");
		PrintNumber(v % 1000, 10, 3);
		uart_puts("V ");


		// Now toggle the pins on/off to see if they are working.
		// First turn all off:
		LATAbits.LATA0 = 0;
		LATAbits.LATA1 = 0;
		LATBbits.LATB0 = 0;
		LATBbits.LATB1 = 0;
		LATAbits.LATA2 = 0;
		// Now turn on one of the outputs per loop cycle to check
		switch (LED_toggle++)
		{
		case 0:
			LATAbits.LATA0 = 1;
			break;
		case 1:
			LATAbits.LATA1 = 1;
			break;
		case 2:
			LATBbits.LATB0 = 1;
			break;
		case 3:
			LATBbits.LATB1 = 1;
			break;
		case 4:
			LATAbits.LATA2 = 1;
			break;
		default:
			break;
		}
		if (LED_toggle > 4) LED_toggle = 0;

		waitms(200);
	}*/
}