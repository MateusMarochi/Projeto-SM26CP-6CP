#include <msp430.h> 
#include <stdio.h>

void ini_P1_P2(void);
void ini_uCon(void);
void ini_Timer0(void);
void ini_Timer1(void);
void ini_ADC10(void);

unsigned int ADC10_vetor[8], media_vector1[16], media_vector2[16], media_samps=0, media_movel1=0, media_movel2;
unsigned char i=0, indice_mv=0, estagio=0;

void main(void)
{
    ini_P1_P2();
    ini_uCon();
    ini_Timer0();
    ini_Timer1();

    while(1){
        
    }

}

void ini_P1_P2(void){
    // PORTA 1
    P1DIR = ~(BIT2 + BIT4 + BIT6 + BIT7); // = BIT0 + BIT1 + BIT3 + BIT5
    P1REN = BIT6 + BIT7;
    P1OUT = BIT6 + BIT7; //habilitando pull-up ch_on_off e ch_s_sel
    P1IES = BIT6 + BIT7; //habilitando detecção de borda de descida ch_on_off e ch_s_sel
    P1IFG = 0; //limpando a flag

    // PORTA 2
    /*
    Entradas: 0, 5 e 6 (0 e 6 com pull up)
    Saídas: 1, 2, 3, 4, 7 (todas em nível baixo)
    Mudança de função dos pinos 2.1, 2.4, 2.6 e 2.7
    */
    P2DIR = BIT1 + BIT2 + BIT3 + BIT4 + BIT7;
    P2REN = BIT0 + BIT6;
    P2OUT = BIT0 + BIT6;
    P2SEL = BIT1 + BIT4;
    P2IFG = 0;

}

void ini_uCon(void){
    //Configuração do sistema de clock.
    WDTCTL = WDTPW | WDTHOLD; // parando watchdog timer

    /* Config. do BCS           [[EDITAR]]
     *     VLO - Nao utilizado
     *     LFXT1 - xtal 32k
     *     DCO ~ 8 MHz
     *
     *     MCLK  -> 8 MHz
     *     SMCLK ->  4 MHz
     *     ACLK  -> 32768 Hz
     */

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
     *      - Modo contagem: UP (inicialmente ficará no modo PARADO)
     *      - Int. do contador: desabilitada
     *
     * MODULO 0: (responsável pelo deb_encoder)
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

void ini_Timer1(void){ //********** AJEITAR PARA 2 PWM!!
    /* Timer0 para gerar PWM para controle dos LEDs
     *   - Frequencia/periodo: 600 Hz
     *   - RC.: 0% inicial
     *
     * CONTADOR:
     *      - Clock: SMCLK ~ 8MHZ
     *          - Fdiv: 4
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
    TA1CTL = TASSEL1 + ID1 + MC0;
    TA1CCTL1 = OUTMOD0 + OUTMOD1 + OUTMOD2 + OUT;
    TA1CCR0 = 3332;
    TA1CCR1 = 0; //PWM1
    TA2CCR2 = 0; //PWM2
}

void ini_ADC10 (void){ //*******************Mandar email pro professor ver como faz pra 2 entrada analógica
    
    /* Inicializacao do ADC10
     *
     *   - Clock: ADC10OSC = 6,13 MHz
     *   - Taxa conv. max.:  ~ 200 ksps (3,7 MHz a 6,3 MHz)
     *   - Tensao de REF:  Vcc = 2,5 V
     *   - Entrada: A2 e A4. Inicia fazendo amostragem de A2 e em sequência de A4.
     *   - Tempo de amostragem: 150 ms (Gatilho por Timer 1)
     *   - Ts (Sample Time): Pot. 10 kohms
     *        -> (Rs + Ri).Ci.ln(2^11) = (10k + 2k).27pf.ln(2^11) = 2,5 us
     *            - t_clk = 1/6,3M = 158,73n
     *            - n_ciclos = 2,5us/158,73n = 15,75 ----> 16 x ADC10CLK
     *   - Media de 8 amostras repetidas
     *       -> DTC para transf. no Modo 1 Bloco
     *
     *
     */

    ADC10CTL0 = SREF0 + ADC10SHT0 + ADC10SHT1 + REF2_5V + REFON + ADC10ON + ADC10IE;
    ADC10CTL1 = INCH1 + SHS0; 
    ADC10DTC0 = 0;
    ADC10DTC1 = 8;
    ADC10SA = &ADC10_vetor[0];
    ADC10AE0 = BIT2 + BIT4;
    ADC10CTL0 |= ENC;

}

#pragma vector=ADC10_VECTOR
__interrupt void RTI_ADC10(void){
    ADC10CTL0 &= ~ENC

    //calculo da média de 8 amostras
    soma=0;
    for(i=0;i<8;i++)
    {
        soma = soma + ADC10_vetor[i];
    }
    media_samps = soma >> 3;
    
    switch(estagio)
    {
        case 0:

            media_vector1[indice_mv]=media_samps;
           
            soma=0;

            //calculo da nova media movel 1
            for(i=0;i<16;i++)
            {
                soma = soma + media_vector1[i];
            }
            media_movel1 = soma >> 4;

            //no final da amostragem de A2, é feita uma amostragem da entrada A4:
            estagio++;
            ADC10CTL1 &= ~INCH1;
            ADC10CTL1 |= INCH2; 
        
            ADC10SA = &ADC10_vetor[0];
            ADC10CTL0 |= ENC;

            break;

        case 1:

            media_vector2[indice_mv]=media_samps;

            //atualizando indice dos vetores de médias:
            if(indice_mv<=14)
                indice_mv++;
            else
                indice_mv=0;

            soma=0;

            //calculo da nova media movel 2
            for(i=0;i<16;i++)
            {
                soma = soma + media_vector2[i];
            }
            media_movel2 = soma >> 4;

            break;

    }
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void RTI_TA0 (void){
	
	ADC10CTL0 |= ENC + ADC10SC; // Fornece o disparo de conversão por software.

}