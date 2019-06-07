#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "msp.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"

/**
 * main.c
 */
void main(void)
{
    float temp = 71.239999;
    float humidity = 22.419999;

    char fmt_temp[5];
    char fmt_humidity[5];

	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	set_DCO(FREQ_24_MHz);
	init_LCD();

	// Display Start-up Message
	write_string("Weather Station");
	change_line();
	write_string("Starting Up...");
	delay_ms(3000);

	while(1)
	{
	    // Convert floating point temp and humidity to strings
	    sprintf(fmt_temp, "%0.2f", temp);
	    sprintf(fmt_humidity, "%0.2f", humidity);

	    clear_LCD();
	    home_LCD();
	    clear_LCD();

	    // Write Temperature String
	    write_string("Temp: ");
	    write_string(fmt_temp);
	    write_string(" F");

	    change_to_line(2);

	    // Write Humidity String
	    write_string("Humidity: ");
	    write_string(fmt_humidity);
	    write_string("%");

	    // Refresh every 2 Seconds
	    delay_ms(2000);
	}

}
