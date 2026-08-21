#include "board.h"
#include "drv_rtc.h"

#include "rtc.h"

#define DRV_RTC_BACKUP_MAGIC       0x52544331UL
#define DRV_RTC_EPOCH_MIN          946684800UL
#define DRV_RTC_EPOCH_MAX          4102444799UL

struct stm32_rtc_device
{
    struct rt_device parent;
    RTC_HandleTypeDef *hrtc;
};

static struct stm32_rtc_device g_rtc_device;

static rt_bool_t stm32_rtc_is_leap_year(rt_uint16_t year)
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U))) ? RT_TRUE : RT_FALSE;
}

static rt_uint8_t stm32_rtc_days_in_month(rt_uint16_t year, rt_uint8_t month)
{
    static const rt_uint8_t days_per_month[] =
    {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month == 2U) && (stm32_rtc_is_leap_year(year) == RT_TRUE))
    {
        return 29U;
    }

    return days_per_month[month - 1U];
}

static rt_uint32_t stm32_rtc_days_before_year(rt_uint16_t year)
{
    rt_uint16_t current_year;
    rt_uint32_t days = 0U;

    for (current_year = 1970U; current_year < year; current_year++)
    {
        days += (stm32_rtc_is_leap_year(current_year) == RT_TRUE) ? 366U : 365U;
    }

    return days;
}

static rt_err_t stm32_rtc_calendar_to_epoch(const RTC_DateTypeDef *date,
                                             const RTC_TimeTypeDef *time,
                                             rt_uint32_t *epoch)
{
    rt_uint16_t year;
    rt_uint8_t month;
    rt_uint32_t days;

    if ((date == RT_NULL) || (time == RT_NULL) || (epoch == RT_NULL))
    {
        return -RT_EINVAL;
    }

    year = (rt_uint16_t)(2000U + date->Year);
    month = date->Month;
    if ((year < 2000U) || (year > 2099U) || (month < 1U) || (month > 12U) ||
        (date->Date < 1U) || (date->Date > stm32_rtc_days_in_month(year, month)) ||
        (time->Hours > 23U) || (time->Minutes > 59U) || (time->Seconds > 59U))
    {
        return -RT_EINVAL;
    }

    days = stm32_rtc_days_before_year(year);
    for (month = 1U; month < date->Month; month++)
    {
        days += stm32_rtc_days_in_month(year, month);
    }
    days += (rt_uint32_t)(date->Date - 1U);

    *epoch = (days * 86400UL) + ((rt_uint32_t)time->Hours * 3600UL) +
             ((rt_uint32_t)time->Minutes * 60UL) + time->Seconds;
    return RT_EOK;
}

static rt_err_t stm32_rtc_epoch_to_calendar(rt_uint32_t epoch,
                                             RTC_DateTypeDef *date,
                                             RTC_TimeTypeDef *time)
{
    rt_uint32_t days;
    rt_uint32_t seconds_of_day;
    rt_uint16_t year = 1970U;
    rt_uint8_t month = 1U;
    rt_uint16_t days_in_year;

    if ((date == RT_NULL) || (time == RT_NULL) ||
        (epoch < DRV_RTC_EPOCH_MIN) || (epoch > DRV_RTC_EPOCH_MAX))
    {
        return -RT_EINVAL;
    }

    days = epoch / 86400UL;
    seconds_of_day = epoch % 86400UL;
    while (year <= 2099U)
    {
        days_in_year = (stm32_rtc_is_leap_year(year) == RT_TRUE) ? 366U : 365U;
        if (days < days_in_year)
        {
            break;
        }
        days -= days_in_year;
        year++;
    }

    if (year > 2099U)
    {
        return -RT_EINVAL;
    }

    while (days >= stm32_rtc_days_in_month(year, month))
    {
        days -= stm32_rtc_days_in_month(year, month);
        month++;
    }

    date->Year = (rt_uint8_t)(year - 2000U);
    date->Month = month;
    date->Date = (rt_uint8_t)(days + 1U);
    date->WeekDay = (rt_uint8_t)(((epoch / 86400UL + 3UL) % 7UL) + 1UL);

    time->Hours = (rt_uint8_t)(seconds_of_day / 3600UL);
    seconds_of_day %= 3600UL;
    time->Minutes = (rt_uint8_t)(seconds_of_day / 60UL);
    time->Seconds = (rt_uint8_t)(seconds_of_day % 60UL);
    /* TimeFormat is ignored while CubeMX configures RTC for 24-hour mode. */
    time->TimeFormat = 0U;
    time->DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time->StoreOperation = RTC_STOREOPERATION_RESET;
    time->SubSeconds = 0U;
    time->SecondFraction = 0U;

    return RT_EOK;
}

static rt_bool_t stm32_rtc_has_valid_time(struct stm32_rtc_device *device)
{
    HAL_PWR_EnableBkUpAccess();
    return (HAL_RTCEx_BKUPRead(device->hrtc, RTC_BKP_DR0) == DRV_RTC_BACKUP_MAGIC) ? RT_TRUE : RT_FALSE;
}

static rt_err_t stm32_rtc_init(rt_device_t dev)
{
    (void)dev;
    return RT_EOK;
}

static rt_err_t stm32_rtc_open(rt_device_t dev, rt_uint16_t oflag)
{
    (void)dev;
    (void)oflag;
    return RT_EOK;
}

static rt_err_t stm32_rtc_close(rt_device_t dev)
{
    (void)dev;
    return RT_EOK;
}

static rt_size_t stm32_rtc_read(rt_device_t dev,
                                 rt_off_t pos,
                                 void *buffer,
                                 rt_size_t size)
{
    (void)dev;
    (void)pos;
    (void)buffer;
    (void)size;
    return 0U;
}

static rt_size_t stm32_rtc_write(rt_device_t dev,
                                  rt_off_t pos,
                                  const void *buffer,
                                  rt_size_t size)
{
    (void)dev;
    (void)pos;
    (void)buffer;
    (void)size;
    return 0U;
}

static rt_err_t stm32_rtc_get_epoch(struct stm32_rtc_device *device, rt_uint32_t *epoch)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    if (stm32_rtc_has_valid_time(device) == RT_FALSE)
    {
        return -RT_EEMPTY;
    }

    if ((HAL_RTC_GetTime(device->hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_GetDate(device->hrtc, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
        return -RT_EIO;
    }

    return stm32_rtc_calendar_to_epoch(&date, &time, epoch);
}

static rt_err_t stm32_rtc_set_epoch(struct stm32_rtc_device *device, rt_uint32_t epoch)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    if (stm32_rtc_epoch_to_calendar(epoch, &date, &time) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    HAL_PWR_EnableBkUpAccess();
    if ((HAL_RTC_SetTime(device->hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_SetDate(device->hrtc, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
        return -RT_EIO;
    }

    HAL_RTCEx_BKUPWrite(device->hrtc, RTC_BKP_DR0, DRV_RTC_BACKUP_MAGIC);
    return RT_EOK;
}

static rt_err_t stm32_rtc_control(rt_device_t dev, int cmd, void *args)
{
    struct stm32_rtc_device *device = (struct stm32_rtc_device *)dev;

    if (args == RT_NULL)
    {
        return -RT_EINVAL;
    }

    switch (cmd)
    {
    case DRV_RTC_CTRL_GET_EPOCH:
        return stm32_rtc_get_epoch(device, (rt_uint32_t *)args);

    case DRV_RTC_CTRL_SET_EPOCH:
        return stm32_rtc_set_epoch(device, *(const rt_uint32_t *)args);

    default:
        return -RT_ENOSYS;
    }
}

static int stm32_rtc_device_register(void)
{
    g_rtc_device.hrtc = &hrtc;
    g_rtc_device.parent.type = RT_Device_Class_RTC;
    g_rtc_device.parent.init = stm32_rtc_init;
    g_rtc_device.parent.open = stm32_rtc_open;
    g_rtc_device.parent.close = stm32_rtc_close;
    g_rtc_device.parent.read = stm32_rtc_read;
    g_rtc_device.parent.write = stm32_rtc_write;
    g_rtc_device.parent.control = stm32_rtc_control;

    return rt_device_register(&g_rtc_device.parent,
                              BSP_RTC_DEVICE_NAME,
                              RT_DEVICE_FLAG_RDWR);
}
INIT_BOARD_EXPORT(stm32_rtc_device_register);
