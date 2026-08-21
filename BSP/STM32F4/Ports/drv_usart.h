#ifndef BSP_STM32F407_DRV_USART_H
#define BSP_STM32F407_DRV_USART_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_UART_CTRL_CLEAR_RX        (RT_DEVICE_CTRL_BASE(Char) + 0x20)
#define DRV_UART_CTRL_GET_RX_DROPS    (RT_DEVICE_CTRL_BASE(Char) + 0x21)
#define DRV_UART_CTRL_GET_RX_ERRORS   (RT_DEVICE_CTRL_BASE(Char) + 0x22)

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32F407_DRV_USART_H */
