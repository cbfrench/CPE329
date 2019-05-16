/*
 * dac.h
 */

#ifndef DAC_H_
#define DAC_H_


void init_SPI(void);
void set_voltage(uint16_t val);
void TA0_0_IRQHandler(void);
void TA0_N_IRQHandler(void);
void init_timers();

#endif /* DAC_H_ */
