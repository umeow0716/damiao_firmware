#ifndef DAMIAO_INTERRUPTS_H
#define DAMIAO_INTERRUPTS_H

/* Semantic interrupt entry points used by host tests and the vector wrappers. */
void position_sensor_timer_irq(void);
void position_sensor_dma_irq(void);
void adc_foc_control_irq(void);
void mcan1_receive_irq(void);
void debug_uart_receive_irq(void);

/* Names consumed directly by startup_hc32f448.S. */
void IRQ000_Handler(void);
void IRQ001_Handler(void);
void IRQ002_Handler(void);
void IRQ003_Handler(void);
void IRQ004_Handler(void);

#endif
