#ifndef STM32H7XX_CONF_H
#define STM32H7XX_CONF_H

/*
 * HAL integration boundary for the STM32H743VIT6 RT-Thread BSP.
 *
 * CubeMX normally routes this dependency through main.h. The BSP has no
 * generated application entry, so generated peripheral files include this
 * header instead.
 */
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32H7XX_CONF_H */
