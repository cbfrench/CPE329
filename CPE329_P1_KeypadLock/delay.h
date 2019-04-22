/*
 * delay.h
 *
 *  Created on: Apr 8, 2019
 *      Author: Joe Sandoval and Connor French
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "msp.h"

#define FREQ_1_5_MHz    ((uint32_t)0x00000000)
#define FREQ_3_MHz      ((uint32_t)0x00010000)
#define FREQ_6_MHz      ((uint32_t)0x00020000)
#define FREQ_12_MHz     ((uint32_t)0x00030000)
#define FREQ_24_MHz     ((uint32_t)0x00040000)
#define FREQ_48_MHz     ((uint32_t)0x00050000)

void set_DCO(uint32_t frequency);
void delay_us(int delay);
void delay_ms(int delay);

#endif /* DELAY_H_ */
