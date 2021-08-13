#include <msp430.h> 
#include <stdio.h>

void ini_P1_P2(void){
    // PORTA 1
    P1DIR = ~(BIT2 + BIT4 + BIT6 + BIT7); // = BIT0 + BIT1 + BIT3 + BIT5
    P1REN = BIT6 + BIT7;
    P1OUT = BIT6 + BIT7; //habilitando pull-up bits 6 e 7
    P1ES = BIT6 + BIT7; //*** Ver com o professor se há a necessidade de setar as duas.
    P1IFG = 0;
    //ADC10AE = BIT2 + BIT4;

    // PORTA 2
    /*
    Entradas: 0, 5 e 6 (0 e 6 com pull up)
    Saídas: 1, 2, 3, 4, 7 (todas em nível baixo)
    Mudança de função dos pinos 2.1, 2.4, 2.6 e 2.7
    */
    P2DIR = BIT1 + BIT2 + BIT3 + BIT4 + BIT7;
	P2REN = BIT0 + BIT6;
	P2OUT = BIT0 + BIT6;
    P2SEL = BIT1 + BIT4 + BIT6 + BIT7;
    P2IFG = 0;

}

void main(void) {
    ini_P1_P2();

}

void ini_uCon(void){
    //Configuração do sistema de clock.
    WDTCTL = WDTPW | WDTHOLD;

    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    BCSCTL2 = DIVS0 + DIVS1;
    BCSCTL3 = XCAP0 + XCAP1;
    while(BCSCTL3 & LFXT1OF);
    __enable_interrupt();
}
