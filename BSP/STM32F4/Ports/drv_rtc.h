#ifndef BSP_STM32F407_DRV_RTC_H
#define BSP_STM32F407_DRV_RTC_H

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_RTC_CTRL_GET_EPOCH        (RT_DEVICE_CTRL_BASE(RTC) + 1)
#define DRV_RTC_CTRL_SET_EPOCH        (RT_DEVICE_CTRL_BASE(RTC) + 2)

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32F407_DRV_RTC_H */
