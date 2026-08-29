/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This adapter is derived from RTTHREAD_SEGGER_TOOL/adapter/drv_rtt.c and is
 * implemented against the RT-Thread 4.1.1 base character-device interface.
 */

#include <rthw.h>
#include <rtthread.h>

#include "SEGGER_RTT.h"

#if defined(RT_USING_DEVICE) && defined(RT_USING_CONSOLE)

#define SEGGER_RTT_DEVICE_NAME             "jlinkRtt"
#define SEGGER_RTT_CHANNEL_INDEX           0U
#define SEGGER_RTT_TERMINAL_INDEX          0U
#define SEGGER_RTT_RX_THREAD_STACK_SIZE    512U
#define SEGGER_RTT_RX_THREAD_PRIORITY      (RT_THREAD_PRIORITY_MAX - 2U)
#define SEGGER_RTT_RX_THREAD_TIMESLICE     5U
#define SEGGER_RTT_RX_POLL_INTERVAL_MS     5

_Static_assert(sizeof(SEGGER_RTT_DEVICE_NAME) <= RT_NAME_MAX,
               "SEGGER_RTT_DEVICE_NAME exceeds RT_NAME_MAX");
_Static_assert(RT_THREAD_PRIORITY_MAX > 2U,
               "RT_THREAD_PRIORITY_MAX is too small for the RTT worker");

static struct rt_device g_segger_rtt_device;
static struct rt_thread g_segger_rtt_rx_thread;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t g_segger_rtt_rx_stack[SEGGER_RTT_RX_THREAD_STACK_SIZE];
static rt_bool_t g_segger_rtt_registered;
static rt_bool_t g_segger_rtt_rx_thread_started;

static rt_err_t segger_rtt_device_init(rt_device_t dev)
{
    (void)dev;
    return RT_EOK;
}

static rt_err_t segger_rtt_device_open(rt_device_t dev, rt_uint16_t oflag)
{
    (void)dev;
    (void)oflag;
    return RT_EOK;
}

static rt_err_t segger_rtt_device_close(rt_device_t dev)
{
    (void)dev;
    return RT_EOK;
}

static rt_size_t segger_rtt_device_read(rt_device_t dev,
                                        rt_off_t pos,
                                        void *buffer,
                                        rt_size_t size)
{
    (void)dev;
    (void)pos;

    if ((buffer == RT_NULL) || (size == 0U))
    {
        return 0U;
    }

    return (rt_size_t)SEGGER_RTT_Read(SEGGER_RTT_CHANNEL_INDEX,
                                     buffer,
                                     (unsigned)size);
}

static rt_size_t segger_rtt_device_write(rt_device_t dev,
                                         rt_off_t pos,
                                         const void *buffer,
                                         rt_size_t size)
{
    const rt_uint8_t *input = (const rt_uint8_t *)buffer;
    rt_size_t count;
    static const rt_uint8_t carriage_return = '\r';

    (void)pos;
    if ((input == RT_NULL) || (size == 0U))
    {
        return 0U;
    }

    for (count = 0U; count < size; count++)
    {
        if (((dev->flag & RT_DEVICE_FLAG_STREAM) != 0U) &&
            (input[count] == '\n'))
        {
            if (SEGGER_RTT_Write(SEGGER_RTT_CHANNEL_INDEX,
                                 &carriage_return,
                                 1U) != 1U)
            {
                break;
            }
        }

        if (SEGGER_RTT_Write(SEGGER_RTT_CHANNEL_INDEX,
                             &input[count],
                             1U) != 1U)
        {
            break;
        }
    }

    return count;
}

static rt_err_t segger_rtt_device_control(rt_device_t dev, int cmd, void *args)
{
    if (cmd != RT_DEVICE_CTRL_CHAR_STREAM)
    {
        return -RT_ENOSYS;
    }

    if ((args != RT_NULL) && (*(const rt_bool_t *)args == RT_FALSE))
    {
        dev->flag &= (rt_uint16_t)~RT_DEVICE_FLAG_STREAM;
    }
    else
    {
        dev->flag |= RT_DEVICE_FLAG_STREAM;
    }

    return RT_EOK;
}

static void segger_rtt_rx_thread_entry(void *parameter)
{
    rt_size_t available;

    (void)parameter;

    while (1)
    {
        available = (rt_size_t)SEGGER_RTT_HasData(SEGGER_RTT_CHANNEL_INDEX);
        if ((available > 0U) &&
            ((g_segger_rtt_device.open_flag & RT_DEVICE_OFLAG_OPEN) != 0U) &&
            (g_segger_rtt_device.rx_indicate != RT_NULL))
        {
            (void)g_segger_rtt_device.rx_indicate(&g_segger_rtt_device,
                                                  available);
        }

        (void)rt_thread_mdelay(SEGGER_RTT_RX_POLL_INTERVAL_MS);
    }
}

int rt_hw_jlink_rtt_init(void)
{
    rt_err_t result;

    if (g_segger_rtt_registered)
    {
        return RT_EOK;
    }

    SEGGER_RTT_Init();
    SEGGER_RTT_SetTerminal(SEGGER_RTT_TERMINAL_INDEX);
    (void)SEGGER_RTT_ConfigUpBuffer(SEGGER_RTT_CHANNEL_INDEX,
                                    "RTTUP",
                                    RT_NULL,
                                    0U,
                                    SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    (void)SEGGER_RTT_ConfigDownBuffer(SEGGER_RTT_CHANNEL_INDEX,
                                      "RTTDOWN",
                                      RT_NULL,
                                      0U,
                                      SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    g_segger_rtt_device.type = RT_Device_Class_Char;
    g_segger_rtt_device.init = segger_rtt_device_init;
    g_segger_rtt_device.open = segger_rtt_device_open;
    g_segger_rtt_device.close = segger_rtt_device_close;
    g_segger_rtt_device.read = segger_rtt_device_read;
    g_segger_rtt_device.write = segger_rtt_device_write;
    g_segger_rtt_device.control = segger_rtt_device_control;

    result = rt_device_register(&g_segger_rtt_device,
                                SEGGER_RTT_DEVICE_NAME,
                                RT_DEVICE_FLAG_RDWR |
                                RT_DEVICE_FLAG_STREAM |
                                RT_DEVICE_FLAG_INT_RX);
    if (result != RT_EOK)
    {
        return result;
    }

    g_segger_rtt_registered = RT_TRUE;
    SEGGER_RTT_printf(SEGGER_RTT_CHANNEL_INDEX,
                      "\r\nSEGGER RTT control block: %p\r\n",
                      &_SEGGER_RTT);
    return RT_EOK;
}

static int segger_rtt_device_register(void)
{
    return rt_hw_jlink_rtt_init();
}
INIT_BOARD_EXPORT(segger_rtt_device_register);

static int segger_rtt_rx_worker_init(void)
{
    rt_err_t result;

    if (g_segger_rtt_rx_thread_started)
    {
        return RT_EOK;
    }

    result = rt_thread_init(&g_segger_rtt_rx_thread,
                            "rtt_rx",
                            segger_rtt_rx_thread_entry,
                            RT_NULL,
                            g_segger_rtt_rx_stack,
                            sizeof(g_segger_rtt_rx_stack),
                            SEGGER_RTT_RX_THREAD_PRIORITY,
                            SEGGER_RTT_RX_THREAD_TIMESLICE);
    if (result != RT_EOK)
    {
        return result;
    }

    result = rt_thread_startup(&g_segger_rtt_rx_thread);
    if (result == RT_EOK)
    {
        g_segger_rtt_rx_thread_started = RT_TRUE;
    }

    return result;
}
INIT_COMPONENT_EXPORT(segger_rtt_rx_worker_init);

#endif /* RT_USING_DEVICE && RT_USING_CONSOLE */
