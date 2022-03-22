#pragma config FNOSC = PRI          // Oscillator Source Selection (Primary Oscillator XT, HS, EC)
#pragma config POSCMOD = HS         // Primary Oscillator Select (HS Oscillator mode selected)
#pragma config JTAGEN = OFF         // JTAG Enable bit (JTAG is disabled)
#pragma config WINDIS = OFF         // Watchdog Timer Window (Windowed Watchdog Timer enabled; FWDTEN must be 1)
#pragma config FWDTEN = OFF         // Watchdog Timer Enable (Watchdog Timer is disabled)

#include "xc.h"
#include <stdint.h>
#define FCY 4000000UL       //Instruction cycle frequency, Hz - required for __delayXXX() to work
#include "libpic30.h"       //__delayXXX() functions macros defined here

uint8_t nec_state, bit_n, nec_ok;
uint16_t timer_value;
uint32_t nec_code;


volatile uint8_t Timer1_flag = 0;
/*void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void){
    IFS0bits.T1IF =0;       // reset the interrupt flag
    LATA = ~LATA;
}*/

void reset_timer(){
    TMR1 = 0;
}

void Timer1_init(){
    T1CONbits.TON = 0;          // Start/Stop bit - Stop Timer
    T1CONbits.TCKPS = 0x03;     // Timer prescaler 1:256
    T1CONbits.TCS = 0x00;       // Using FOSC/2 
    PR1 = 15624;                // Load the Timer period value into PR1 register
    IPC0bits.T1IP =1;           // Set interrupt priority
    IFS0bits.T1IF =0;           // Reset interrupt flag
    IEC0bits.T1IE =1;           // Turn on the timer1 interrupt    
}

void Timer1_Start(){
    T1CONbits.TON =1;           // Start Timer
}
void Timer1_Stop(){
    T1CONbits.TON =0;
}



int main(void) {                // Main function  
    
    TRISA = 0;                  // Set PortA as output - LED enable
    TRISD = 0xffff;             // set portD as input
    RPINR0bits.INT1R = 0;
#ifdef __PIC24FJ1024GB610__
    ANSA = 0;                   // Disable Analog of PortA 
    ANSD = 0;
#endif
    IOCNDbits.IOCND0 = 1;    //Pin : RD0
    IOCPDbits.IOCPD0 = 1;    //Pin : RD0
    IOCFDbits.IOCFD0 = 0;    //Pin : RD0
    PADCONbits.IOCON = 1;    //Config for PORTD
    //RD0_SetInterruptHandler(&RD0_CallBack);
    
    /****************************************************************************
     * Interrupt On Change: Interrupt Enable
     ***************************************************************************/
    IFS1bits.IOCIF = 0; //Clear IOCI interrupt flag
    IEC1bits.IOCIE = 1; //Enable IOCI interrupt
    Timer1_init();
    while(1){                   // Endless Loop
        while(nec_ok == 0){
           
        };
        nec_ok = 0;
        nec_state = 0;
        bit_n=0;
        LATA = nec_code>>16;
        __delay_ms(1000);
    }
    return 0;
}

void __attribute__ ((interrupt,no_auto_psv)) _IOCInterrupt ( void )
{
    if(IFS1bits.IOCIF == 1)
    {
        if(IOCFDbits.IOCFD0 == 1 && (PORTDbits.RD0||!PORTDbits.RD0))
        {
            if(nec_state!=0){
                timer_value = TMR1;
                Timer1_Stop();
                reset_timer();
                Timer1_Start();
            } // end if
            switch(nec_state){
                case 0:
                    Timer1_init();
                    Timer1_Start();
                    nec_state = 1;
                    bit_n = 0;
                    break;
                case 1:
                    if(timer_value < 0x8a || timer_value > 0x8c){
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    }
                    else {
                        nec_state = 2;
                    }
                    break;
                case 2:
                    if(timer_value < 0x45 || timer_value > 0x47){
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    }
                    else nec_state = 3;
                    break;
                case 3:
                    if(timer_value < 0x7|| timer_value > 0x9){
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    }
                    else nec_state = 4;
                    break;
                case 4:
                    if(timer_value > 0x20 || timer_value < 0x7){
                        Timer1_Stop();
                        reset_timer();
                        nec_state = 0;
                    }
                    else {
                        if(timer_value > 0xf){
                            nec_code |=   (uint32_t)1<<(31 - bit_n); 
                        }
                        else{
                            nec_code &= ~((uint32_t)1<<(31 - bit_n));
                        }
                        bit_n++;
                        if(bit_n > 31) {
                            nec_ok = 1;
                            nec_state = 0;
                        }
                        else {
                            nec_state = 3;
                        }
                        
                    }
                    break;
            } // end switch 
            IOCFDbits.IOCFD0 = 0;  //Clear flag for Pin - RD0
        }
        // Clear the flag
        IFS1bits.IOCIF = 0;
    }
}



