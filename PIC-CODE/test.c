#pragma config FNOSC = PRI          // Oscillator Source Selection (Primary Oscillator XT, HS, EC)
#pragma config POSCMOD = HS         // Primary Oscillator Select (HS Oscillator mode selected)
#pragma config JTAGEN = OFF         // JTAG Enable bit (JTAG is disabled)
#pragma config WINDIS = OFF         // Watchdog Timer Window (Windowed Watchdog Timer enabled; FWDTEN must be 1)
#pragma config FWDTEN = OFF         // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config ICS = PGD2

#include "xc.h"
#include <stdint.h>
#define FCY 8000000UL       //Instruction cycle frequency, Hz - required for __delayXXX() to work
#include "libpic30.h"       //__delayXXX() functions macros defined here

volatile uint8_t nec_state, bit_n = 0;
volatile uint8_t nec_ok = 0;
volatile uint16_t timer_value;
volatile uint32_t nec_code;
volatile uint8_t TMR1stop;

//void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void){
    //IFS0bits.T1IF =0;       // reset the interrupt flag
   // TMR1 = 0;
   // Timer1_Stop();
//}

void reset_timer() {
    TMR1 = 0;
}

void Timer1_init() {
    T1CONbits.TON = 0; // Start/Stop bit - Stop Timer
    T1CONbits.TCKPS = 0x03; // Timer prescaler 1:256
    T1CONbits.TCS = 0x00; // Using FOSC/2 
    PR1 = 15624; // Load the Timer period value into PR1 register
    IPC0bits.T1IP = 1; // Set interrupt priority
    IFS0bits.T1IF = 0; // Reset interrupt flag
    //IEC0bits.T1IE = 1; // Turn on the timer1 interrupt    
}

void Timer1_Start() {
    T1CONbits.TON = 1; // Start Timer
}

void Timer1_Stop() {
    T1CONbits.TON = 0;
}
void delay() {
    Timer1_Start();
    while(!IFS0bits.T1IF) {}
    IFS0bits.T1IF = 0;
    Timer1_Stop();
}

void disableISR() {
    IOCNDbits.IOCND0 = 0; //Pin : RD0
    IOCPDbits.IOCPD0 = 0; //Pin : RD0
    IOCFDbits.IOCFD0 = 0; //Pin : RD0
    PADCONbits.IOCON = 0; //Config for PORTD
    //RD0_SetInterruptHandler(&RD0_CallBack);

    /****************************************************************************
     * Interrupt On Change: Interrupt Enable
     ***************************************************************************/
    IEC1bits.IOCIE = 0; //Enable IOCI interrupt
    IFS1bits.IOCIF = 0; //Clear IOCI interrupt flag
}

void enableISR() {
    IOCNDbits.IOCND0 = 1; //Pin : RD0
    IOCPDbits.IOCPD0 = 1; //Pin : RD0
    IOCFDbits.IOCFD0 = 0; //Pin : RD0
    PADCONbits.IOCON = 1; //Config for PORTD
    //RD0_SetInterruptHandler(&RD0_CallBack);
    /****************************************************************************
     * Interrupt On Change: Interrupt Enable
     ***************************************************************************/
    IEC1bits.IOCIE = 1; //Enable IOCI interrupt
    IFS1bits.IOCIF = 0; //Clear IOCI interrupt flag
    
}

void ADC_init()
{
    AD1CON2bits.PVCFG =0;
    AD1CON3bits.ADCS = 0xFF;        // A/D Conversion Clock Select bits
    AD1CON1bits.SSRC = 0x00;        // Sample Clock Source Select bits
    AD1CON3bits.SAMC = 0x10;        // Auto-Sample Time Select bits 
    AD1CON1bits.FORM = 0x00;        // Data Output Format bits (00 = Absolute decimal result, unsigned, right justified)
    AD1CON2bits.SMPI = 0x00;        // Interrupt Sample/DMA Increment Rate Select bits
    AD1CON1bits.ADON = 0x01;        // A/D Operating Mode bit (0 = A/D Converter is off)
}

uint16_t ADC_Read()
{
    uint16_t i;
    ANSBbits.ANSB4=0;
    AD1CON1bits.DONE = 0;
    AD1CHS = 0x400;               // Select ADC channel 
    AD1CON1bits.SAMP = 1;           // ADC Sample-and-Hold amplifiers are sampling
    for(i = 0; i < 1000; i++){      // Waiting for sampling
        Nop();         
    }
    AD1CON1bits.SAMP = 0;           // ADC Sample-and-Hold amplifiers are sampling
    for(i = 0; i < 1000; i++){      // Waiting for sampling
        Nop();
    }
    while(!AD1CON1bits.DONE);       // Waiting until the Convertion is complete 
    return ADC1BUF0;                // return ADC value 
}

void space(int time)
{
    T1CON = 0; // Start/Stop bit - Stop Timer
    //T1CONbits.TCKPS = 0x0; // Timer prescaler 1:256
    //T1CONbits.TCS = 0x00; // Using FOSC/2 
    PR1 = time; // Load the Timer period value into PR1 register
    IPC0bits.T1IP = 1; // Set interrupt priority
    IFS0bits.T1IF = 0; // Reset interrupt flag
    IEC0bits.T1IE = 1; // Enable Output Compare 1 interrupts
    TMR1stop = 0;
    PORTDbits.RD2 = 0;
    T1CON = 0x8000;
    while(TMR1stop == 0);
}

void PulseOut(int time)
{
    PORTDbits.RD2 = 0;
    T1CONbits.TON = 0; // Start/Stop bit - Stop Timer
    //T1CONbits.TCKPS = 0x0; // Timer prescaler 1:256
    //T1CONbits.TCS = 0x00; // Using FOSC/2 
    PR1 = time; // Load the Timer period value into PR1 register
    IPC0bits.T1IP = 1; // Set interrupt priority
    IFS0bits.T1IF = 0; // Reset interrupt flag
    IEC0bits.T1IE = 1; // Enable Output Compare 1 interrupts
    TMR1stop = 0;
    
    /*OC3CON1 = 0x0000; // Turn off Output Compare 1 Module
    OC3R = 0x0069; // Initialize Compare Register1 with 0x0026
    OC3RS = 0x0069; // Initialize Secondary Compare Register1 with 0x0026
    OC3CON1 = 0x6; // Load new compare mode to OC1CON
    OC3CON2 = 0xC;*/
    PR2 = 0x67; // Initialize PR2 with 0x004C
    //T2CONbits.TCS = 0x00;
    IPC1bits.T2IP = 1; // Setup Output Compare 1 interrupt for
    IFS0bits.T2IF = 0; // Clear Output Compare 1 interrupt flag
    IEC0bits.T2IE = 1; // Enable Output Compare 1 interrupts
    //T2CONbits.TCKPS = 0x03; // Timer prescaler 1:256
    T2CONbits.TCS = 0x00; // Using FOSC/2 
    T1CON = 0x8000;
    T2CON = 0x8000; // Start Timer2 with assumed settings
    while(TMR1stop == 0);
}

void __attribute__ ((interrupt,no_auto_psv)) _T2Interrupt(void)
{
    PORTDbits.RD2 = ~PORTDbits.RD2;
    IFS0bits.T2IF = 0;
}

void __attribute__ ((interrupt,no_auto_psv)) _T1Interrupt(void)
{
    //PORTAbits.RA0 = ~PORTAbits.RA0;
    TMR1stop = 1;
    IFS0bits.T1IF = 0;
    IEC0bits.T2IE = 0;
    //OC3CON1 = 0x0000;
    T2CON = 0;
    T1CON = 0;
    TMR1 = 0;
    TMR2 = 0;
}

void sendVI(uint16_t V, uint16_t I)
{
    int i = 0;
    uint32_t VI = V;
    VI = (VI << 16) | I;
    while(i < 36)
    {
        if(i == 0) PulseOut(0x8C9F);
        else if (i == 1) space(0x464F);
        else if( i == 2) PulseOut(0x9C3F);
        else if (i == 35) PulseOut(0x8D5);
        else if ((VI & 0x1) == 0)
        {
            PulseOut(0x8D5);
            space(0x8D5);
            VI = VI >> 1;
        }
        else 
        {
            PulseOut(0x8D5);
            space(0x11AA);
            VI = VI >> 1;
        }
        i++;
    }
    //__delay_ms(500);
}

void sendADC()
{
    disableISR();
    uint16_t ADC = ADC_Read();
    uint16_t Vout = (ADC*4.8876); //(ADC/1023)*5000
    uint16_t Iout = (Vout-2500)/185;
    sendVI(Vout,Iout);
   __delay_ms(1000);
}

int main() { // Main function  

    TRISA = 0; // Set PortA as output - LED enable
    TRISD = 0xffFB; // set portD as input
    RPINR0bits.INT1R = 0;
    ANSA = 0; // Disable Analog of PortA 
    ANSD = 0;
    Timer1_init();
    enableISR();
    ADC_init();
    int power;
    PORTDbits.RD6 = 1;
    PORTDbits.RD2 = 0;
    while (1) { // Endless Loop
        nec_state = 0;
        while (nec_ok == 0) {
            PORTAbits.RA0 = power;
            //if(nec_code == 0x12345678) sendADC();
            if(!PORTDbits.RD6) {
                sendADC();
                enableISR();
            }
        }
        bit_n = 0;
        nec_state = 5;
        switch(nec_code){
            case 0x12345678: {
                sendADC();
                enableISR();
                break;
            }
            case 0x0fd5486b:{
                power = ~power;
                break;
            }
            default: ;
        }
        nec_code = 0;
        nec_ok = 0;
        timer_value = 0;
        __delay_ms(500);
    }
    return 0;
}

void __attribute__((interrupt, auto_psv)) _IOCInterrupt(void) {
    if (IFS1bits.IOCIF == 1 && nec_state < 5) {
            if (nec_state != 0) {
                timer_value = TMR1;
                Timer1_init();
                reset_timer();
                Timer1_Start();
            } // end if
            switch (nec_state) {
                case 0:
                    Timer1_init();
                    Timer1_Start();
                    nec_state = 1;
                    bit_n = 0;
                    break;
                case 1:
                    if (timer_value < 0x8a || timer_value > 0x8c) {
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    } else {
                        nec_state = 2;
                    }
                    break;
                case 2:
                    if (timer_value < 0x45 || timer_value > 0x47) {
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    } else nec_state = 3;
                    break;
                case 3:
                    if (timer_value < 0x7 || timer_value > 0x9) {
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    } else nec_state = 4;
                    break;
                case 4:
                    if (timer_value > 0x20 || timer_value < 0x7) {
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    } else {
                        if (timer_value > 0xf) {
                            nec_code |= (uint32_t) 1 << (31-bit_n);
                        } else {
                            nec_code &= ~((uint32_t) 1 << (31-bit_n));
                        }
                        bit_n++;
                        if (bit_n > 31) {
                            nec_ok = 1;
                        } else {
                            nec_state = 3;
                        }
                    }
                    break;
            } // end switch 
        // Clear the flag
    }
    IFS1bits.IOCIF = 0;
    IOCFDbits.IOCFD0 = 0;
    IFS0bits.INT0IF = 0;
}