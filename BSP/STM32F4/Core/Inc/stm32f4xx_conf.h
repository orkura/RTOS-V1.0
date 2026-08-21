#ifndef STM32F4XX_CONF_H
#define STM32F4XX_CONF_H

/*
 * HAL integration boundary for the STM32F407 RT-Thread BSP.
 *
 * CubeMX normally routes this dependency through main.h.  The BSP has no
 * generated application entry, so generated peripheral files include this
 * header instead.
 */
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_CONF_H */
