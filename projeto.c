#include <msp430.h> 
#include <stdio.h>

void ini_P1_P2(void);
void ini_uCon(void);
void ini_Timer0(void);
void ini_Timer1(void);


void main(void)
{
    ini_P1_P2();
    ini_uCon();
    ini_Timer0();
    ini_Timer1();
}

void ini_P1_P2(void){
    // PORTA 1
    P1DIR = ~(BIT2 + BIT4 + BIT6 + BIT7); // = BIT0 + BIT1 + BIT3 + BIT5
    P1REN = BIT6 + BIT7;
    P1OUT = BIT6 + BIT7; //habilitando pull-up bits 6 e 7
    P1IES = BIT6 + BIT7; //*** Ver com o professor se há a necessidade de setar as duas.
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
    P2OUT ^= BIT1 + BIT4 + BIT2 + BIT3 + BIT7;
    P2SEL = BIT1 + BIT4;
    P2IFG = 0;

}

void ini_uCon(void){
    //Configuração do sistema de clock.
    WDTCTL = WDTPW | WDTHOLD; // parando watchdog timer

    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    BCSCTL2 = DIVS0 + DIVS1;
    BCSCTL3 = XCAP0 + XCAP1;
    while(BCSCTL3 & LFXT1OF);
    __enable_interrupt();
}

void ini_Timer0(void){
    /* Inicializacao do TImer 0 para o debouncer das chaves e do encoder, e disparo
     * de conversão para o ADC10 (tempo de amostragem)
     *
     * CONTADOR:
     *      - Clock: ACLK ~ 32768Hz
     *          - Fdiv: 1
     *      - Modo contagem: UP (inicialmente ficara no modo PARADO)
     *      - Int. do contador: desabilitada
     *
     * MODULO 0: (resposnável pelo deb_encoder)
     *      - Funcao: comparacao (nativa)
     *      - Int.: Habilitada
     *      Tempo debouncer encoder deb_enc_time = 3 ms
     *      - Valor para TA0CCR0: 32768 * 3e-3 - 1 = 97
     *
     * MODULO 1: (debounce das chaves)
     *      - Funcao: comparacao (nativa)
     *      - Int.: Habilitada
     *      Tempo debouncer chaves deb_ch_time = 12 ms
     *      - Valor para TA0CCR1: 32768 * 12e-3 - 1  = 392
     *
     * MODULO 2: (disparo de conversão)
     *      - Funcao: comparacao (nativa)
     *      - Int.: Habilitada
     *      Tempo amostragem t_samps = 150 ms
     *      - Valor para TA0CCR2: 4914
     */
// Clock base Timer A0:
    TA0CTL = TASSEL0;
// Modulo 0
    TA0CCTL0 = CCIE;
    TA0CCR0 = 97;
// Modulo 1
    TA0CCTL1 = CCIE;
    TA0CCR1 = 392;
// Modulo 2
    TA0CCTL2 = CCIE;
    TA0CCR2 = 4914;
}

void ini_Timer1(void){
    /* Timer0 para gerar PWM para controle dos LEDs
     *   - Frequencia/periodo: 600 Hz
     *   - RC.: 0% inicial
     *
     * CONTADOR:
     *      - Clock: SMCLK ~ 2MHZ
     *          - Fdiv: 1
     *      - Int.: Desabilitada
     *      - Modo: UP
     *
     * MODULO 0: -> ref. de periodo/freq. do PWM
     *      - Funcao: comparacao
     *      - Int.: desabilitada
     *      - TA0CCR0 = (2e6 / 600) -1 = 3332
     *
     * MODULO 1: -> Gerar o sinal
     *      - Funcao: comparacao
     *      - Int.: desabilitada
     *      - Modo de saida: 7 (reset/set)
     *      - TA0CCR1 = 0;
     */
    TA1CTL = TASSEL1 + MC0;
    TA1CCTL1 = OUTMOD0 + OUTMOD1 + OUTMOD2 + OUT;
    TA1CCR0 = 3332;
    TA1CCR1 = 0;
}
