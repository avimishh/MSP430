#include "msp430.h"
#define A1 LCDM10
#define A2 LCDM6
#define A3 LCDM4
#define A4 LCDM19
#define A5 LCDM15
#define A6 LCDM8
const unsigned char lcd_num[10] = {
  0xFC,        // 0
  0x60,        // 1
  0xDB,        // 2
  0xF3,        // 3
  0x67,        // 4
  0xB7,        // 5
  0xBF,        // 6
  0xE4,        // 7
  0xFF,        // 8
  0xF7        // 9
};
void LCDinit (void)
{
  PJSEL0 = BIT4 | BIT5;                   // For LFXT
  
  // Initialize LCD segments 0 - 21; 26 - 43
  LCDCPCTL0 = 0xFFFF;
  LCDCPCTL1 = 0xFC3F;
  LCDCPCTL2 = 0x0FFF;
  
  // Disable the GPIO power-on default high-impedance mode
  // to activate previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;
  
  // Configure LFXT 32kHz crystal
  CSCTL0_H = CSKEY >> 8;                  // Unlock CS registers
  CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
  do
  {
    CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
    SFRIFG1 &= ~OFIFG;
  }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag
  CSCTL0_H = 0;                           // Lock CS registers
  
  // Initialize LCD_C
  // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
  LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
  
  // VLCD generated internally,
  // V2-V4 generated internally, v5 to ground
  // Set VLCD voltage to 2.60v
  // Enable charge pump and select internal reference for it
  LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
  
  LCDCCPCTL = LCDCPCLKSYNC;               // Clock synchronization enabled
  
  LCDCMEMCTL = LCDCLRM;                   // Clear LCD memory
  LCDCCTL0 |= LCDON;
  
}

void LCD_All_On(void){
  
  int i;
  char *ptr = 0;
  ptr += 0x0A20;		//Create a pointer ptr to LCD memory starts at 0x0A20
  for (i = 0;i<21;i++){
    *ptr = 0xFF;
    ptr = ptr+1;
  }
}

void LCD_All_Off(void){
  
  int i;
  char *ptr = 0;
  ptr += 0x0A20;		//Create a pointer ptr to LCD memory starts at 0x0A20
  for (i = 0;i<21;i++){
    *ptr = 0x00;
    ptr = ptr+1;
  }
}

void Display_digit(int p,int d){
  char *ptr = 0;
  ptr += 0x0A20;
  int A_[6] = {0x09,0x05,0x03,0x12,0x0E,0x07};
  
  ptr += A_[p-1];
  *ptr = lcd_num[d];
}

void Display_Number (double n){
  int num;
  LCD_All_Off();
  int i=6;
  if(n<0){
    LCDM11=0x04;
    n *=(-1);
  }
  else
    LCDM11=0x00;
  
  
  num=(int)(n*10);
  LCDM16 |= BIT0;
  while(num>0&&i>0){
    Display_digit(i--,num%10);
    num =num/10;
  }
}

void Display_Number2 (long long n){
  int i=6;
  if(n<0){
    LCDM11=0x04;
    n *=(-1);
  }
  else
    LCDM11=0x00;
  
  while(n>0&&i>0){
      Display_digit(i--,n%10);
      n =n/10;
  }
}

void Display_ERROR(){
  LCD_All_Off();
  //!
  LCDM3=0x01;
  //E
  A1 = 0x9F;
  //R
  A2 = 0x8E;
  LCDM7 = 0x22;
  //R
  A3 = 0x8E;
  LCDM5 = 0x22;
  //O
  A4 = 0xFC;
  //R
  A5 = 0x8E;
  LCDM16 = 0x22;
}

