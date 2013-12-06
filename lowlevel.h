#ifndef __LOWLEVEL_H
#define	__LOWLEVEL_H

#include <stdint.h>

#define LED0        GPIO8 // PC8
#define LED1        GPIO9 // PC9
#define USER_BUTTON GPIO0

#define CC3000_IRQ  GPIO13 // PC13, 'interrupt pin' from cc3000 chip
#define CC3000_DOUT GPIO14 // PC14, data to CC3000
#define CC3000_CLK  GPIO15 // PC15, clk to CC3000
#define CC3000_DIN  GPIO0  // PF0, data from CC3000
#define CC3000_CS   GPIO1  // PF1, chip select to CC3000
#define CC3000_EN   GPIO0  // PC0, enable to CC3000

void ll_init();
void ll_enable_interrupts();

void led(uint8_t num, uint8_t state);

void panic(void);


#endif
