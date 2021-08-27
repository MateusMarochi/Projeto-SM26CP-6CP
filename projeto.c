#include <msp430.h> 
#include <stdio.h>

 /* -----> Equipe 6 (21_01_G5) <-------
 * Gabriel Junges Baratto
 * Gabriela Pietra Pereira
 * Mateus Marochi 
 * Leonardo Oliveira Nogueira
 * Vinicius Rodrigues Frandoloso
 */

void ini_P1_P2(void);
void ini_uCon(void);
void ini_Timer0(void);
void ini_Timer1(void);
void ini_ADC10(void);
void desliga_micro(void);
void liga_micro(void);

unsigned int ADC10_vetor[8], media_vector1[16], media_vector2[16], media_samps=0, media_movel1=0, media_movel2=0, set_point1=75, set_point2=75, soma = 0;
unsigned char i=0, indice_mv=0, estagio=0, inicializacao=1,ligado=0, deb_ch_on=0, deb_enc_on=0, setor=1;

// Inicialização dos multiplicadores de tempo base (Timer0):
unsigned int temp_amost = 0;
unsigned char temp_deb_ch = 0, temp_deb_enc = 0;

void main(void)
{
    ini_P1_P2();
    ini_uCon();
    ini_Timer0();
    ini_Timer1();
    ini_ADC10();

    /* Loop da main, continuamente ajustando a luminosidade dos setores à referência:
     */
    do{
        //ajuste do setor 1:

        if ((set_point1 - 15) > media_movel1 && TA1CCR1+99 <= 3333) //se a media movel 1 estiver abaixo do desejado
        {
            TA1CCR1 = TA1CCR1 + 99; //diminui-se a r.c. do PWM 1 (e assim a alimentação do led 1)
        }

        else if ((set_point1 + 15) < media_movel1 && TA1CCR1-99 >= 0) //se não, se a media movel 1 estiver acima do desejado
        {
            TA1CCR1 = TA1CCR1 - 99; //diminui-se a r.c. do PWM 1 (e assim a alimentação do led 1)
        }

        //ajuste do setor 2:

        if ((set_point2 - 15) > media_movel2 && TA1CCR2+99 <= 3333) //se a media movel 2 estiver abaixo do desejado
        {
            TA1CCR2 = TA1CCR2 + 99; //diminui-se a r.c. do PWM 2 (e assim a alimentação do led 2)
        }

        else if ((set_point2 + 15) < media_movel2 && TA1CCR2-99 >= 0) //se não, se a media movel 2 estiver acima do desejado
        {
            TA1CCR2 = TA1CCR2 - 99; //diminui-se a r.c. do PWM 2 (e assim a alimentação do led 2)
        }

    }while(1);

}

//RTI ADC
#pragma vector=ADC10_VECTOR
__interrupt void RTI_ADC10(void){
    ADC10CTL0 &= ~ENC;

    //calculo da média de 8 amostras
    soma=0;
    for(i=0;i<8;i++)
    {
        soma = soma + ADC10_vetor[i];
    }
    media_samps = soma >> 3; // O resultado é um numero entre 0-1023
    // convertendo a média para um valor em lumens
    media_samps = media_samps * 500 / 1023; // Converte o resultado para lumens
    
    switch(estagio)
    {
        case 0: // Primeiro estágio, amostragem de A2

            media_vector1[indice_mv]=media_samps;
           
            soma=0;

            // Na inicialização, é utilizada a media atual (media_samps) no lugar da média móvel:
            if(inicializacao==0){ // Se não estiver na inicialização
                //calculo da nova media movel 1
                for(i=0;i<16;i++)
                {
                    soma = soma + media_vector1[i];
                }
                media_movel1 = soma >> 4;
            }
            else{ // Se estiver inicializando
                media_movel1 = media_samps;
            }

            //no final da amostragem de A2, é feita uma amostragem da entrada A4:
            estagio++;
            ADC10CTL1 &= ~INCH1;
            ADC10CTL1 |= INCH2; 
        
            ADC10SA = &ADC10_vetor[0];
            ADC10CTL0 |= ENC;

            break;

        case 1: // Segundo estágio, amostragem de A4

            media_vector2[indice_mv]=media_samps;

            //atualizando indice dos vetores de médias:
            if(indice_mv<=14)
                indice_mv++;
            else
                indice_mv=0;

            soma=0;

            // Na inicialização, é utilizada a media atual (media_samps) no lugar da média móvel:
            if(inicializacao==0){ // Se não estiver na inicialização
                //calculo da nova media movel 2
                for(i=0;i<16;i++)
                {
                    soma = soma + media_vector2[i];
                }
                media_movel2 = soma >> 4;
            }
            else{ // Se estiver inicializando
                media_movel2 = media_samps;
                if(indice_mv==0){ // Se o vetor média foi preenchido, saimos do modo de inicialização
                    inicializacao=0;
                }
            }
            break;

    }
}


// RTI da PORTA 1
#pragma vector=PORT1_VECTOR
__interrupt void RTI_da_Porta_1(void){
    
    // Passo 1: desabilita int. chaves
    P1IE &= ~BIT6 + ~BIT7;
    // Passo 2: inicializa o debounce das chaves
    deb_ch_on = 1;
}

// RTI da PORTA 2
#pragma vector=PORT2_VECTOR
__interrupt void RTI_da_Porta_2(void){
    
    // Passo 1: desabilita int. encoder
    P2IE &= ~BIT0;
    // Passo 2: inicializa o debounce do encoder
    deb_enc_on = 1;
}


//RTI Timer 0
#pragma vector=TIMER0_A0_VECTOR
__interrupt void RTI_TA0 (void){

    TA0CCTL0 &= ~CCIE;


    /*  Tratamento debouncer encoder:
     *  2 - Verificar se P2.0 esta em nivel baixo
     *      - Verificar setor selecionado (P1.7)
     *      - Verificar sentido de giro (P2.6)
     */
    if(deb_enc_on==1){ // Se estiver ativo o debounce do encoder

        if(temp_deb_enc>=30){

            temp_deb_enc = 0;
            deb_enc_on = 0;
            
            if( (~P2IN) & BIT0 ){ // Verifica se entrada de Sa esta em nivel baixo
                                // para validar tecla.
                if(setor==1) // Verifica qual setor está selecionado
                { // Setor 1   
                    if((~P2IN) & BIT6){  // Verifica sentido do passo
                        if(set_point1 >= 350){  // Sentido horario
                            set_point1 = 350;
                        }else{
                            set_point1 = set_point1 + 25;
                        }
                    }else{
                        if(set_point1 <= 75){  // Sentido ANTI-horario
                            set_point1 = 75;
                        }else{
                            set_point1 = set_point1 - 25;
                        }
                    }
                }
                else
                { // Setor 2
                    if((~P2IN) & BIT6){  // Verifica sentido do passo
                        if(set_point2 >= 350){  // Sentido horario
                            set_point2 = 350;
                        }else{
                            set_point2 = set_point2 + 25;
                        }
                    }else{
                        if(set_point2 <= 75){  // Sentido ANTI-horario
                            set_point2 = 75;
                        }else{
                            set_point2 = set_point2 - 25;
                        }
                    }
                }

                P2IFG &= ~(BIT0);
                P2IE |= BIT0;
            }
        }
        else
            temp_deb_enc++;

    }
    
    if(deb_ch_on==1){ // Se estiver ativo o debounce das chaves

        if(temp_deb_ch>=120){

            temp_deb_ch = 0;
            deb_ch_on = 0;

            /*  Finalização debounce chave on_off:
             *
             *  Verificar se a chave (ch_on_off) continua pressionada
             *      - Desligar/Ligar as conversões e as saídas do micro
             */
            if((~P1IN) & BIT6){ // Se a chave ch_on_off continuar pressionada

                if(ligado==1)
                    desliga_micro();
                else
                    liga_micro();

                P1IFG &= ~BIT6; // Limpando a flag.
                P1IE |= BIT6; // Habilita int. do BIT6 da P1

            }
            
            /*  Finalização debounce chave s_sel:
             *
             *  Verificar se a chave (ch_s_sel) continua pressionada
             *      - Alterar o setor selecionado
             */
            if((~P1IN) & BIT7){ // Se a chave ch_s_sel continuar pressionada

                if(setor==1){ //Se estiver no setor 1
                    setor=2; //Alterna-se para o setor 2
                    P2OUT |= BIT7; // liga led_setor2
                    P2OUT &= ~BIT3; // desliga led_setor1
                }
                else{ //E vice-versa:
                    setor=1;
                    P2OUT |= BIT3; // liga led_setor1
                    P2OUT &= ~BIT7; // desliga led_setor2
                }
                
                P1IFG &= ~BIT7; // Limpando a flag.
                P1IE |= BIT7; // Habilita int. do BIT7 da P1

            }
        }
        else
            temp_deb_ch++;
    }

    // Gatilho de conversão do ADC (a cada 150 ms):

    if(temp_amost>=1500)
    {
        temp_amost=0; // Reseta o contador do amostrador
        estagio=0;

        ADC10CTL1 |= INCH1; // seleciona entrada analogica A2
        ADC10CTL1 &= ~INCH2;

	    ADC10CTL0 |= ENC + ADC10SC; // Fornece o disparo de conversão por software.
    }
    else{ // Se não chegou a 1500 ainda, incrementa
        temp_amost++;
    }
    TA0CCTL0 |= CCIE; // Habilita a int. Timer 0
    TA0CCTL0 &= ~CCIFG; // Desliga a flag Timer 0
}


void ini_P1_P2(void){
    // PORTA 1
    P1DIR = ~(BIT2 + BIT4 + BIT6 + BIT7); // = BIT0 + BIT1 + BIT3 + BIT5
    P1REN = BIT6 + BIT7;
    P1OUT = BIT6 + BIT7; // habilitando pull-up ch_on_off e ch_s_sel
    P1IES = BIT6 + BIT7; // habilitando detecção de borda de descida ch_on_off e ch_s_sel
    P1IFG = 0; // limpando a flag
    P1IE = BIT6 + BIT7; // habilitando interrupções P1.6 e P1.7

    // PORTA 2
    /*
    Entradas: 0, 5 e 6 (0 e 6 com pull up)
    Saídas: 1, 2, 3, 4, 7 (todas em nível baixo)
    Mudança de função dos pinos 2.1, 2.4, 2.6 e 2.7
    */
    P2DIR = BIT1 + BIT2 + BIT3 + BIT4 + BIT7;
    P2REN = BIT0 + BIT6;
    P2OUT = BIT0 + BIT2 + BIT3 + BIT6;
    P2SEL = BIT1 + BIT4;
    P2IFG = 0;
    P2IE = BIT0; // habilitando interrupções P2.0

}

void ini_uCon(void){
    //Configuração do sistema de clock.
    WDTCTL = WDTPW | WDTHOLD; // parando watchdog timer

    /* Config. do BCS
     *     VLO - Nao utilizado
     *     LFXT1 - xtal 32k
     *     DCO ~ 16 MHz
     *
     *     MCLK  -> 16 MHz
     *     SMCLK ->  8 MHz
     *     ACLK  -> 32768 Hz
     */

    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 = DIVS0;
    BCSCTL3 = XCAP0 + XCAP1;
    while(BCSCTL3 & LFXT1OF);
    __enable_interrupt();
}

void ini_Timer0(void){
    /* Inicializacao do Timer 0 para geração da base de tempo utilizada
     * pelo debouncer das chaves e do encoder, e pelo disparo
     * de conversão para o ADC10 (tempo de amostragem)
     *
     * CONTADOR:
     *      - Clock: SMCLK ~ 8MHz
     *          - Fdiv: 1
     *      - Modo contagem: UP (ligado nessa função já)
     *      - Int. do contador: desabilitada
     * 
     * MODULO 0:
     *      - Tempo base 100 us -> TA0CCR0 = 8e+6 * 100e-6 - 1 = 799
     * ----------------------------------------------
     * Responsável por:
     * 
     * FUNÇÃO 1: (responsável pelo deb_encoder)
     *      - Tempo debouncer encoder deb_enc_time = 3 ms
     *      - Fator de mult. -> 3 ms / 100 us = 30x
     *
     * FUNCAO 2: (debounce das chaves)
     *      - Tempo debouncer chaves deb_ch_time = 12 ms
     *      - Fator de mult. -> 12 ms / 100 us = 120x
     *
     * FUNCAO 3: (disparo de conversão)
     *      - Tempo amostragem t_samps = 150 ms
     *      - Fator de mult. -> 150 ms / 100 us = 1500x
     */
// Clock base Timer A0:
    TA0CTL = TASSEL1 + MC0;
// Modulo 0 (tempo base de 100 us)
    TA0CCTL0 = CCIE;
    TA0CCR0 = 799;
}

void ini_Timer1(void){
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
     * 
     * 
     * CÁLCULO PASSO PWM:
     *      - delta_pwm = 3% de 3333 => passo = 0,03 * 3333 - 1 = 99
     */
    TA1CTL = TASSEL1 + ID1 + MC0;
    TA1CCTL1 = OUTMOD0 + OUTMOD1 + OUTMOD2 + OUT;
    TA1CCR0 = 3332;
    TA1CCR1 = 0; //PWM1
    TA1CCR2 = 0; //PWM2
}

void ini_ADC10 (void){
    
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
    ADC10CTL1 = INCH1 + SHS0; //inicialmente a entrada A2 é selecionada
    ADC10DTC0 = 0;
    ADC10DTC1 = 8; //modo 8 amostras consecutivas
    ADC10SA = &ADC10_vetor[0];
    ADC10AE0 = BIT2 + BIT4;
}

void desliga_micro(void){

    P2OUT &= ~(BIT2 + BIT3 + BIT7); // desliga todos os leds
    TA1CCR0 = 0;
    ADC10CTL0 &= ~(ADC10ON + ADC10IE);
    ADC10CTL0 &= ~ENC;
    ligado = 0;

}

void liga_micro(void){

    P2OUT |= BIT2 + BIT3; // Tecla press. -> liga Leds on_off e setor_1

    setor=1; // seleção do setor 1
    inicializacao=1; // modo de inicialização do vetor de médias
    estagio=0; // reset do estágio do ADC
    
    ADC10CTL0 |= ADC10ON + ADC10IE;
    ADC10CTL0 |= ENC; //habilita a conversão

    indice_mv=0;

    set_point1 = 75; // Resetando set_point's para o valor padrão de 75
    set_point2 = 75;

    TA1CCR0 = 3332; // Inicia o contador do PWM
    TA1CCR1 = 0; //PWM1
    TA1CCR2 = 0; //PWM2

    ligado=1;

}