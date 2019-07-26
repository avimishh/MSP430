#include <msp430.h>
#include <stdio.h>
void numToTerm(double n);
double getDutyCycle(int TMP);
int numFromUser = 0;
int TMP = 0;
void LCDinit();
void Display_Number(double n);
void startConv();
void wait(int ms);
double tmpArr[5] = {0,0,0,0,0};
int counter = 0;
int isResultOk = 0;
double tmpAVG();
static double tempAVG = 0;
void Display_Number2 (long long n);
void startPWM();

void main(){
  WDTCTL = WDTPW | WDTHOLD;       	// Stop WDT
  PM5CTL0 &= ~LOCKLPM5;	        	// Clear locked IO Pins
  LCDinit ();
  
  P1DIR |= BIT0; // red-LED feedback for system working, Set P1.0 to output direction
  
  // ---------------------TA1.0---------------------
//  TA1CTL = TASSEL_1 + MC_1 + TACLR + TAIE; //ACLK, up mode, enable interrupt
  TA1CCR0 = 6554;               // 5 Hz
  //--------------------------------------------------
  
  // -------------------ADC---------------------
  P8SEL1 |= BIT4;                         	// Use P8.4 (A7) for analog input
  P8SEL0 |= BIT4;
  // Configure ADC12
  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 = ADC12SHT0_2 | ADC12ON;      // Sampling time, S&H=16, ADC12 on
  ADC12CTL1 = ADC12SHP;                // Use sampling timer
  ADC12CTL2 |= ADC12RES_2;                // 12-bit conversion results
  ADC12MCTL0 |= ADC12INCH_7;             // A7 ADC input select; Vref=AVCC
  ADC12IER0 |= ADC12IE0;                // Enable ADC conv complete interrupt
  //-------------------------------------------------
  
  // -----------------------UART init-------------------------
  UCA1CTLW0 = UCSWRST;                 	// Put eUSCI in reset
  UCA1CTLW0 |= UCSSEL__SMCLK;    	// CLK = SMCLK
  UCA1BRW = 6;                              		// 9600 baud
  UCA1MCTLW = 0x22D1; 		// UCBRSx value = 0x22 (See UG)
  UCA1CTLW0 &= ~UCSWRST;                	// Initialize eUSCI
  UCA1IE |= UCRXIE; // user input
  
  // Configure GPIO
  P3SEL0 |= BIT4 | BIT5;                    	// USCI_A1  UART operation
  P3SEL1 &= ~(BIT4 | BIT5);
  P3DIR |= BIT4;  
  //---------------------------------------------------------
  
  // ---------PWM---------------------
  //-------------------------------------
  
  __bis_SR_register(LPM0_bits | GIE);	  	// LPM0
  
}

void startPWM(){
  TA0CTL = TASSEL_1 + MC_1 + TACLR; //ACLK, up mode
      TA0CCR0 = 32770;
      TA0CCTL2 = OUTMOD_2;
//      Display_Number(getDutyCycle(25)*10);
      TA0CCR2 = TA0CCR0*(1-getDutyCycle(TMP));//(1-getDutyCycle(28));//1;
      P1SEL0 |= BIT7;
      P1SEL1 |= BIT7;
      P1DIR |= BIT7; // TA0.2
      startConv();
}

double tmpAVG(){
  int i=0;
  double sum=0;
  for(;i<5;i++)
    sum += tmpArr[i];
  return sum/5;
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(){
  double current_temp=0;
  switch(__even_in_range(ADC12IV, ADC12IV_ADC12RDYIFG))   {
  case ADC12IV_NONE:        break;                          // Vector  0:  No interrupt
  case ADC12IV_ADC12IFG0:                                   // Vector 12:  ADC12MEM0 Interrupt
    ADC12CTL0 &= ~ADC12ENC;
    current_temp =  ADC12MEM0 *0.08;            //temp [Celsius] = vin * (vref/2^N) = lm35_output * (3.3/2^12) 
    tmpArr[counter] = current_temp;
    counter++;
    //numToTerm(current_temp);
    if(counter>5){
      counter = 0;
      isResultOk = 1;
      tempAVG = tmpAVG();
    }
    if(isResultOk){
     Display_Number(current_temp);
      numToTerm(tempAVG);
    }
    break;
  case ADC12IV_ADC12IFG1:   break;                      // Vector 14:  ADC12MEM1
  case ADC12IV_ADC12IFG2:   break;                      // Vector 16:  ADC12MEM2
  case ADC12IV_ADC12IFG3:   break;                      // Vector 18:  ADC12MEM3
  case ADC12IV_ADC12IFG4:   break;                      // Vector 20:  ADC12MEM4
  // continue here with IFG5 to IFG31
  default: break;
  }
  __bis_SR_register(LPM0_bits | GIE);	  	// LPM0, ADC12_ISR will force exit
}

#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer (void) {
  
  if (TA1CTL & TAIFG){
    P1OUT ^= BIT0; //Toggle red-LED
    ADC12CTL0 |= ADC12ENC | ADC12SC;    	// Start sampling/conversion
  }
    TA1CTL &=~ TAIFG;
}

#pragma vector=USCI_A1_VECTOR
__interrupt void rxISR() {
  static int minValue = 21;
  static int maxValue = 34;
  int x = 0;
  int ret;
  if  (UCA1IFG & UCRXIFG)  {
    ret = UCA1RXBUF;
    if (ret == ' ') { 
      if (numFromUser >= maxValue) {
        numFromUser = 34;
      }
      if (numFromUser <= minValue) {
        numFromUser = 21;
      }
      TMP = numFromUser;
      startPWM();
    }
    else {
      numFromUser = numFromUser * 10 + (ret - 48);
    }
    UCA1IFG &= ~UCRXIFG;
  }	
}

void startConv(){
  TA1CTL = TASSEL_1 + MC_1 + TACLR + TAIE;      //start TA1.0 count
//  isResultOk = 0;
}

void wait(int ms){		                        // Busy waiting...
  int i, j;
  for(i=ms; i>0; i--)		                        // countdown. 
    for(j=200; j>0; j--);  
}

double getDutyCycle(int TMP) {
  double z = (double)TMP-10;
  z= z/10;
  if (TMP <= 21) return 0;
  if (TMP >= 34) return 1;
  return z;
}

void numToTerm(double n) {
  int temp[] = {0, 0, 0};
  int numInt = (int) (n*10);
  if (n < 10) {
    temp[2] = numInt%10 +48;
    numInt = numInt/10;
    temp[0] = 0;                //high digit = 0
    temp[1] = numInt % 10 + 48;      //low digit
  }
  else if (n < 100) {
    temp[2] = numInt % 10 + 48;
    numInt = numInt/10;
    temp[1] = numInt % 10 + 48;      //low digit
    numInt = numInt/10;
    temp[0] = numInt % 10 + 48;      //high digit
  }
  while(!(UCA1IFG&UCTXIFG));
  UCA1TXBUF = temp[0];
  while(!(UCA1IFG&UCTXIFG));
  UCA1TXBUF =  temp[1];
  while(!(UCA1IFG&UCTXIFG));
  UCA1TXBUF =  46;              // "."
  while(!(UCA1IFG&UCTXIFG));
  UCA1TXBUF =  temp[2];
  while(!(UCA1IFG&UCTXIFG));
  UCA1TXBUF = '\n';
}
