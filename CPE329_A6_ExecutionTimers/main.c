#include "msp.h"
#include <math.h>

float TestFunction(float num);

int main(void) {
    float mainVar;
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    P1->SEL1 &= ~BIT0; //set P1.0 as simple I/O
    P1->SEL0 &= ~BIT0;
    P1->DIR |= BIT0; //set P1.0 as output
    P2->SEL1 &= ~BIT0; //set P2.0 as simple I/O
    P2->SEL0 &= ~BIT0;
    P2->DIR |= BIT0; //set P2.0 as output pins
    P2->OUT |= BIT0; // turn on Blue LED
    mainVar = TestFunction(15); // test function for timing
    P2->OUT &= ~BIT0; // turn off Blue LED
    while(1) // infinite loop to do nothing
        mainVar++; // increment mainVar to eliminate not used warning
}
float TestFunction(float num) {
    float testVar;
    P1->OUT |= BIT0; // turn RED LED on
    //testVar = num;
    P1->OUT &= ~BIT0; // turn RED LED off
    return testVar;
}
