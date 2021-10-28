
// Definição de Pinos
// gpioReset = Reset do ESP8266
// gpioFull  = Flag de caixa cheia
// gpioEmpty = Flag de caixa vazia
// gpioTX    = Alimentação do emissor IR
// gpioRX    = Alimentacao do receptor de IR
//
#define gpioReset    GPIO.GP0 // GP1 = Pino 7
#define gpioFull     GPIO.GP1 // GP1 = Pino 6
#define gpioTX       GPIO.GP2 // GP2 = Pino 5
#define gpioBiasRX   GPIO.GP5 // GP5 = Pino 2

#define chkInterv 5000         // ms

// 4,88mv/bit

// Se a caixa estiver vazia, vai ser a maior tensão possivel = reflexo
#define adcThreshold 200

//
// Fim da definição de pinos

#define timer0Preload   6


unsigned short lastAction  = 0;
unsigned int adc_valueOFF = 0;        // TX OFF
unsigned int adc_valueON  = 0;        // TX ON
unsigned int adc_value = 0;

unsigned int millis = 0;
unsigned int lastRun = 0;

void interrupt() {
  if (INTCON.TMR0IF ==1) // timer 0 interrupt flag
  {
    asm CLRWDT;
    TMR0 = timer0Preload;           // reset the timer preset count
    INTCON.TMR0IF = 0;         // clear the flag
    INTCON.TMR0IE = 1;         // reenable the interrupt
    if (millis == 4294967295) { millis = 0; }
    else { millis++; }
  }

}


void main() {
     //TRISIO define direcao das IOs
     // 0 = Output
     // 1 = Input
              //76543210
     TRISIO = 0b00010000;     // Direcao das IOs, apenas AN3 In
     GPIO   = 0b00000000;     // Estado inical das IOs todos em low

     // OPTION_REG
     // bit 5 T0CS: Timer0 Clock Source Select bit (0 = Internal instruction cycle clock (FOSC/4)  / 1 = Transition on T0CKI pin)
     OPTION_REG.T0CS   = 0;
     // bit 4 T0SE: Timer0 Source Edge Select bit  (0 = Increment on low-to-high                   / 1 = Increment on high-to-low transition on T0CKI pin)
     OPTION_REG.T0SE   = 0;
     // bit 3 PSA: Prescaler Assignment bit        (0 = Prescaler is assigned to the Timer0 module / 1 = Prescaler is assigned to the WDT)
     OPTION_REG.PSA = 0;   // bit 3  Prescaler Assignment bit...0 = Prescaler is assigned to the Timer0
     OPTION_REG.PS2 = 0;   // bits 2-0  PS2:PS0: Prescaler Rate Select bits
     OPTION_REG.PS1 = 0;
     OPTION_REG.PS0 = 1;
     
     // Preload do Timer0 (p/ 1khz)
     TMR0 = timer0Preload;

     WPU    = 0;              // WPU: WEAK PULL-UP REGISTER (0 = all disabled)
     
     asm CLRWDT;
     
     // CMCON0: COMPARATOR CONFIGURATION REGISTER
     CMCON.CM2 = 1;          // bit 2-0 CM<2:0>: Comparator Mode bits (See Figure 8-5)
     CMCON.CM1 = 1;
     CMCON.CM0 = 1;          // 111 = CIN pins are configured as I/O, COUT pin is configured as I/O, Comparator output disabled, Comparator off.
     
     // REGISTER 4-3: ANSEL: ANALOG SELECT REGISTER
     ANSEL.ANS3 = 1;          // bit 3-0 ANS<3:0>: Analog Select bits
     ANSEL.ANS2 = 0;
     ANSEL.ANS1 = 0;          // 0 = Digital I/O. Pin is assigned to port or special function.
     ANSEL.ANS0 = 0;          // 1 = Analog input. Pin is assigned as analog input(1).

     // REGISTER 2-3: INTCON: INTERRUPT CONTROL REGISTER
     //INTCON = 0;
     INTCON.GIE = 1;          // bit 7 GIE: Global Interrupt Enable bit
     INTCON.T0IE = 1;         // bit 5 T0IE: Timer0 Overflow Interrupt Enable bit

     while (1) {
           asm CLRWDT;
           
           if ((millis - lastRun) >= chkInterv) {
              lastRun = millis;
              
              
              gpioBiasRX = 1;
              Delay_ms(1);
              adc_valueOFF = ADC_Read(3); // Read AN2
              while(ADCON0.GO_DONE);
              
              
              gpioTX = 1;
              Delay_ms(1);
              adc_valueON = ADC_Read(3); // Read AN2
              while(ADCON0.GO_DONE);
              gpioTX = 0;
              gpioBiasRX = 0;
              
              
              //adc_value = adc_valueON - adc_valueOFF;
              adc_value = adc_valueON;

              if ((adc_value > adcThreshold) && (lastAction == 1))  {
              //if ((adc_value > adcThreshold))  {
                 gpioFull = 0;       // Garante que a flag ta off
                 gpioReset  = 1;     // manda o Reset no ESP
                 delay_ms(1);        // tempinho pra garantir o reset
                 gpioReset  = 0;     // libera o reset
                 lastAction = 0;     // memoriza que a caixa ta vazia
              }
              else if ((adc_value < adcThreshold) && (lastAction == 0))  {
              //else if ((adc_value < adcThreshold))  {
                 gpioFull = 1;       // define a flag
                 gpioReset  = 1;     // manda o Reset no ESP
                 delay_ms(1);        // tempinho pra garantir o reset
                 gpioReset  = 0;     // libera o reset
                 delay_ms(1000);      // garante que o ESP pegou a flag
                 gpioFull = 0;       // limpa a flag
                 lastAction = 1;     // memoriza que a caixa ta cheia
              }
           }
     }
}