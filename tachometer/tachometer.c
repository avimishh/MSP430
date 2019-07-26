#include "msp430.h"
#include <stdio.h>
int requiredRPM = 0;
int abs(int f);
double getDutyCycle(int RPM);
int countToDisplay = 0;
int numFromUser = 0;
void sendZeroMessage();
void sendNum(int n);
void main(void) {
  WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5; // Clear locked IO Pins

  // sample clock 5Hz
  TA1CTL = TASSEL_1 + MC_1 + TACLR;
  TA1CCR0 = 6554;
  TA1CCTL2 = OUTMOD_2;
  TA1CCR2 = TA1CCR0 / 2; 
  P4SEL0 |= BIT7;
  P4SEL1 |= BIT7;
  P4DIR |= BIT7; // TA1.2
  
  
  // clock of signal from motor
  TB0CTL = TBSSEL_3 + MC_2 + TBCLR;
  TB0CCTL2 = CAP + CM_1 + CCIE;
  P3SEL0 |= BIT3;
  P3SEL1 |= BIT3;
  P3DIR &= ~BIT3; // TB0CLK
  
  // sample interrupt
  P3SEL0 |= ~BIT6;
  P3SEL1 |= BIT6;
  P3DIR &= ~BIT6; // TB0CC2A
  
  // UART init
  UCA1CTLW0 = UCSWRST;                 	// Put eUSCI in reset
  UCA1CTLW0 |= UCSSEL__SMCLK;    	// CLK = SMCLK
  UCA1BRW = 6;                              		// 9600 baud
  UCA1MCTLW = 0x22D1; 		// UCBRSx value = 0x22 (See UG)
  UCA1CTLW0 &= ~UCSWRST;                	// Initialize eUSCI
  UCA1IE |= UCRXIE; // user input
      
  
  // 100Hz
  TA0CTL = TASSEL_1 + MC_1 + TACLR; //ACLK, up mode, enable interrupt
  TA0CCR0 = 328; // 328
  TA0CCTL2 = OUTMOD_2;
  
  TA0CCR2 = TA0CCR0 * (1 - getDutyCycle(requiredRPM));
  P1SEL0 |= BIT7;
  P1SEL1 |= BIT7;
  P1DIR |= BIT7; // TA0.2

  _BIS_SR (LPM0_bits + GIE);
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void timerInter(void) {
  int f;
  unsigned int currentRPM;
  if (TB0CCTL2 & CCIFG) {
    TB0CTL |= TBCLR;
    f = TB0CCR2 * 5;
    f = abs(f);
  }
  countToDisplay++;
  if (countToDisplay == 24) {
    countToDisplay = 0;
    currentRPM = 1.875 * f;
    sendNum(currentRPM);
  }
  TB0CCTL2 &= ~CCIFG;
}

#pragma vector=USCI_A1_VECTOR
__interrupt void rxISR() {
  static int minValue = 700;
  static int maxValue = 1219;
  int ret;
  if  (UCA1IFG & UCRXIFG)  {
    ret = UCA1RXBUF;
    if (ret == ' ') { 
      if (numFromUser > maxValue) {
        numFromUser = maxValue;
      }
      if (numFromUser < minValue) {
        numFromUser = 0;
      }
      requiredRPM = numFromUser;
      if (requiredRPM == 0) {
        sendZeroMessage();
      }
      TA0CCR2 = TA0CCR0 * (1 - getDutyCycle(requiredRPM));
      numFromUser = 0;
    }
    else {
      numFromUser = numFromUser * 10 + (ret - 48);
    }
    UCA1IFG &= ~UCRXIFG;
  }	
}

int abs(int f) {
  return f > 0 ? f : -1 * f;
}



// minimum 700
double getDutyCycle(int RPM) {
  if (RPM == 0) return 0;
  return (0.0086 * RPM * RPM - 1.874 * RPM + 151.61) / 10630;
}

void sendNum(int n) {
  int temp[] = {0, 0, 0, 0}, original = n;
  temp[3] = n % 10 + 48;
  n = n / 10;
  temp[2] = n % 10 + 48;
  n = n / 10;
  temp[1] = n % 10 + 48;
  n = n / 10;
  temp[0] = n % 10 + 48;
  if (original < 10) {
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[3];
  }
  else if (original < 100) {
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[2];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[3];
  }
  else if (original < 1000) {
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[1];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[2];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[3];
  }
  else if (original < 10000) {
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[0];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[1];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[2];
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = temp[3];
  }
  while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = '\n';
}

void sendZeroMessage() {
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'S';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 't';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'o';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'p';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'p';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'e';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = 'd';
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = '\n';
}