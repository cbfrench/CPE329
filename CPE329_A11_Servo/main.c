#include "msp.h"
#include "delay.h"
#include "keypad.h"

#define SERVO_PORT P6
#define SERVO_PIN BIT2
#define ONE_MS 5000000  //needs to calibrated

int key, key_one = -1, key_two = -1;
int angle = 90;
int angle_set = 0;
int outputting = 0;

void adjust_angle(){
    if(key_one < 2){
        angle = key_one * 100 + key_two * 10;
        angle_set = 1;
    }
    else{
        //invalid key combination
    }
}

void increment_angle(){
    angle += 10;
    if(angle > 180){
        angle = 180;
    }
}

void decrement_angle(){
    angle -= 10;
    if(angle < 0){
        angle = 0;
    }
}

void TA0_0_IRQHandler(void) {
    //base code for timer
    if (TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG) {
        TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
        if(angle_set){
            SERVO_PORT->OUT |= SERVO_PIN;
            TIMER_A0->CCR[0] += (int)(angle / 180 * ONE_MS) + ONE_MS;
            outputting = 1;
            angle_set = 0;
        }
        else{
            if(outputting){
                SERVO_PORT->OUT &= ~SERVO_PIN;
                outputting = 0;
            }
            TIMER_A0->CCR[0] += 500;
        }
    }
}

void init_timer(){
    //base code for initializing timer
    TIMER_A0->CTL |= TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_1;
    TIMER_A0->CCR[0] = 500;
    TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
}

void init_servo(){
    SERVO_PORT->SEL0 &= ~SERVO_PIN;
    SERVO_PORT->SEL1 &= ~SERVO_PIN;
    SERVO_PORT->DIR |= SERVO_PIN;
}

void reset_keys(){
    key_one = -1;
    key_two = -1;
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	set_DCO(FREQ_3_MHz);
	init_keypad();
	init_servo();
	init_timer();
	__enable_irq();

	while(1){
	    key = check_key_pressed();
	    if(key != -1){
	        if(key == '#'){
	            decrement_angle();
	            reset_keys();
	        }
	        else if(key == '*'){
	            increment_angle();
	            reset_keys();
	        }
	        else if(key_one == -1){
	            key_one = key;
	        }
	        else if(key_two == -1){
	            key_two = key;
	            adjust_angle();
	            reset_keys();
	        }
	    }
	}
}
