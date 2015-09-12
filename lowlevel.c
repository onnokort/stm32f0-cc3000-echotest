#include <stdint.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/f0/rcc.h>
#include <libopencm3/stm32/f0/gpio.h>
#include <libopencm3/stm32/f0/exti.h>
#include <libopencm3/stm32/f0/nvic.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <cc3000/hardware.h>
#include "lowlevel.h"

void panic(void) {
    int i;
    nvic_disable_irq(NVIC_EXTI0_1_IRQ);
    systick_interrupt_disable();
    while(1) {
        led(0, 0); led(1, 0);
        for (i=0;i<100000;i++) asm volatile("nop");
        led(0, 1); led(1, 1);
        for (i=0;i<100000;i++) asm volatile("nop");
    }
}

// triggered by pressing onboard button
void exti0_1_isr(void) {
    panic();
}

void ll_enable_interrupts() {
    // enable EXTI0
    nvic_enable_irq(NVIC_EXTI0_1_IRQ);

    // enable systick IRQ
    systick_interrupt_enable();
}

void ll_init() {
    rcc_clock_setup_in_hsi_out_48mhz();
    // Initialize used GPIOs and enable peripheral clocks
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_reset_pulse(RST_GPIOA);
    rcc_periph_reset_pulse(RST_GPIOC);
    rcc_periph_reset_pulse(RST_GPIOF);

    // Pin configuration
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE, USER_BUTTON);

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, LED0 | LED1);

    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE, CC3000_IRQ);

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, CC3000_DOUT|CC3000_CLK|CC3000_EN);

    gpio_mode_setup(GPIOF, GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE, CC3000_DIN);

    gpio_set(GPIOF, CC3000_CS);
    gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, CC3000_CS);

    // Enable EXTI0 IRQ on user pin
    exti_select_source(EXTI0, GPIOA);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI0);

    // clear counter so it starts right away
    STK_CVR=0;

    // Set processor clock for systick
    // this does it, the systick_set_clocksource doesn't seem to do the
    // right thing.
    STK_CSR|=4;

    // 8MHZ clock -> systick every 1ms
    systick_set_reload(8000);
    systick_counter_enable();
}

void led(uint8_t num, uint8_t val) {
    if (val)
        gpio_set(GPIOC, num ? LED1 : LED0);
    else
        gpio_clear(GPIOC, num ? LED1 : LED0);
}

// fill hardware.h prototypes


void cc3k_delay_us(unsigned val) {
    unsigned i;
    // FIXME: Extremely inaccurate. Time this better
    for (i=0;i<8*val; i++)
        asm volatile("nop");
}

void cc3k_delay_ms(unsigned val) {
    unsigned w=val+cc3k_regular_cnt;
    while (cc3k_regular_cnt<w);
}

char cc3k_get_irq(void) {
    return gpio_get(GPIOC, CC3000_IRQ)!=0;
}

void cc3k_set_cs(char val) {
    if (val)
        gpio_set(GPIOF, CC3000_CS);
    else
        gpio_clear(GPIOF, CC3000_CS);
}

void cc3k_set_clk(char val) {
    if (val)
        gpio_set(GPIOC, CC3000_CLK);
    else
        gpio_clear(GPIOC, CC3000_CLK);
}
    
void cc3k_set_dout(char val) {
    if (val)
        gpio_set(GPIOC, CC3000_DOUT);
    else
        gpio_clear(GPIOC, CC3000_DOUT);
}

char cc3k_get_din() {
    return gpio_get(GPIOF, CC3000_DIN)!=0;
}

void cc3k_set_enable(char val) {
    if (val)
        gpio_set(GPIOC, CC3000_EN);
    else
        gpio_clear(GPIOC, CC3000_EN);
}

void sys_tick_handler(void) {
    cc3k_regular();
}

void cc3k_global_irq_enable(char val) {
    if (val) {
        nvic_enable_irq(NVIC_EXTI0_1_IRQ);
        systick_interrupt_enable();
    } else {
        nvic_disable_irq(NVIC_EXTI0_1_IRQ);
        systick_interrupt_disable();
    }
}
