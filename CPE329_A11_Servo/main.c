#include "msp.h"
#include "delay.h"
#include "keypad.h"

#define SERVO_PORT P6
#define SERVO_PIN BIT1
#define ONE_MS 750 //needs to calibrated

int key, key_one = -1, key_two = -1;
int angle = 90;
int angle_set = 0;
int outputting = 0;

void reset_keys();

void adjust_angle(){
    if(key_two > -1){
        int tmpAngle = (key_one * 100) + (key_two * 10);

        if(tmpAngle >=0 && tmpAngle <= 180){
            angle = tmpAngle;

            printf("Angle: %d \n", angle);
            angle_set = 1;
        }
    }
    else{
        //invalid key combination
    }

    reset_keys();
}

void increment_angle(){
    angle += 10;
    if(angle > 180){
        angle = 180;
    }

    printf("Angle: %d \n", angle);
    reset_keys();

    angle_set = 1;
}

void decrement_angle(){
    angle -= 10;
    if(angle < 0){
        angle = 0;
    }

    printf("Angle: %d \n", angle);
    reset_keys();

    angle_set = 1;
}

void TA0_0_IRQHandler(void) {
    static int computedPeriod = 0;

    //base code for timer
    if (TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG) {
        TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
        if(angle_set){
            if(!outputting){
                // If not already outputting, being sending sevo pulse
                SERVO_PORT->OUT |= SERVO_PIN;
                computedPeriod = (int)(angle / 180.0 * ONE_MS) + ONE_MS;
                TIMER_A0->CCR[0] += computedPeriod;
                outputting = 1;
            }
            else{
                // Ensure Duty Cycle Remains Low for remainder of 20 ms cycle
                SERVO_PORT->OUT &= ~SERVO_PIN;
                outputting = 0;
                angle_set = 0;

                TIMER_A0->CCR[0] += (20*ONE_MS - computedPeriod) ;
            }
        }
    }
}

void init_timer(){
    //base code for initializing timer
    TIMER_A0->CTL |= TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID__4;
    TIMER_A0->CCR[0] = 500;
    TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
}

void init_servo(){
    // Initalize Servo Pins
    SERVO_PORT->SEL0 &= ~SERVO_PIN;
    SERVO_PORT->SEL1 &= ~SERVO_PIN;
    SERVO_PORT->DIR |= SERVO_PIN;
}

void reset_keys(){
    // Reset Keys one and two to default -1
    key_one = -1;
    key_two = -1;

    outputting = 0;
}

void main(void){
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	set_DCO(FREQ_3_MHz);
	init_keypad();
	init_servo();
	init_timer();
	__enable_irq();

	while(1){
	    key = check_key_pressed();      // Check if a Key is being pressed
	    if(key != -1){
	        if(key == 48/*'#'*/){       // If the # key is pressed, decrement
	            decrement_angle();
	            reset_keys();
	        }
	        else if(key == 46/*'*'*/){  // If the * key is pressed, increment
	            increment_angle();
	            reset_keys();
	        }
	        else if(key_one == -1){     // Store first numeric key
	            key_one = key;
	        }
	        else if(key_two == -1){     // Store second numeric key, then change angle
	            key_two = key;
	            adjust_angle();
	            reset_keys();
	        }
	    }

	    delay_ms(200);
	}
}
