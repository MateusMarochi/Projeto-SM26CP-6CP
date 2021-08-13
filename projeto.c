#include <msp430.h> 
#include <stdio.h>

void ini_P1_P2(void){
    // PORTA 1
    P1DIR = 
    P1REN = BIT6 + BIT7

    // PORTA 2
    P2DIR = BIT1 + BIT2 + BIT3 + BIT4 + BIT7;
	P2REN = BIT0 + BIT6;
	P2OUT = BIT0 + BIT6; 
}

void main(void) {
    ini_P1_P2();
}
