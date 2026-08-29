#ifndef BSP_STM32H743VIT_BOARD_H
#define BSP_STM32H743VIT_BOARD_H

#include <rtthread.h>

#include "stm32h7xx_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_UART2_RX_BUFFER_SIZE    256U
#define BSP_UART2_IRQ_PRIORITY        5U

/* Called by the CubeMX-owned SysTick_Handler(). */
void bsp_systick_handler(void);

#if (BSP_UART2_RX_BUFFER_SIZE < 2U)
#error "BSP_UART2_RX_BUFFER_SIZE must reserve one byte for ring-buffer state."
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32H743VIT_BOARD_H */
