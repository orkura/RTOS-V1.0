#include "board.h"
#include "drv_usart.h"

#include <rthw.h>

#include "usart.h"

#if defined(RT_USING_DEVICE) && defined(RT_USING_CONSOLE)

_Static_assert(sizeof(BSP_UART2_DEVICE_NAME) <= RT_NAME_MAX,
               "BSP_UART2_DEVICE_NAME exceeds RT_NAME_MAX");

struct stm32_uart_device
{
    struct rt_device parent;
    UART_HandleTypeDef *huart;
    rt_uint8_t rx_byte;
    rt_uint8_t rx_buffer[BSP_UART2_RX_BUFFER_SIZE];
    volatile rt_uint16_t rx_head;
    volatile rt_uint16_t rx_tail;
    volatile rt_uint32_t rx_drops;
    volatile rt_uint32_t rx_errors;
    rt_bool_t rx_enabled;
};

static struct stm32_uart_device g_uart2_device;

static rt_err_t stm32_uart2_arm_rx(struct stm32_uart_device *device)
{
    HAL_StatusTypeDef status;

    status = HAL_UART_Receive_IT(device->huart, &device->rx_byte, 1U);
    if (status != HAL_OK)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

static rt_err_t stm32_uart2_start_rx(struct stm32_uart_device *device)
{
    rt_err_t result;

    if (device->rx_enabled)
    {
        return RT_EOK;
    }

    result = stm32_uart2_arm_rx(device);
    if (result != RT_EOK)
    {
        return result;
    }

    device->rx_enabled = RT_TRUE;
    return RT_EOK;
}

static void stm32_uart2_push_rx(struct stm32_uart_device *device, rt_uint8_t byte)
{
    rt_uint16_t next = (rt_uint16_t)((device->rx_head + 1U) % BSP_UART2_RX_BUFFER_SIZE);

    if (next == device->rx_tail)
    {
        device->rx_drops++;
        return;
    }

    device->rx_buffer[device->rx_head] = byte;
    device->rx_head = next;
}

static rt_err_t stm32_uart2_init(rt_device_t dev)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;

    HAL_NVIC_SetPriority(USART2_IRQn, BSP_UART2_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    return stm32_uart2_start_rx(device);
}

static rt_err_t stm32_uart2_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;

    (void)oflag;
    return stm32_uart2_start_rx(device);
}

static rt_err_t stm32_uart2_close(rt_device_t dev)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    device->rx_enabled = RT_FALSE;
    rt_hw_interrupt_enable(level);
    (void)HAL_UART_AbortReceive_IT(device->huart);
    return RT_EOK;
}

static rt_size_t stm32_uart2_read(rt_device_t dev,
                                  rt_off_t pos,
                                  void *buffer,
                                  rt_size_t size)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;
    rt_uint8_t *output = (rt_uint8_t *)buffer;
    rt_size_t count = 0U;
    rt_base_t level;

    (void)pos;
    if ((output == RT_NULL) || (size == 0U))
    {
        return 0U;
    }

    level = rt_hw_interrupt_disable();
    while ((count < size) && (device->rx_tail != device->rx_head))
    {
        output[count++] = device->rx_buffer[device->rx_tail];
        device->rx_tail = (rt_uint16_t)((device->rx_tail + 1U) % BSP_UART2_RX_BUFFER_SIZE);
    }
    rt_hw_interrupt_enable(level);

    return count;
}

static rt_size_t stm32_uart2_write(rt_device_t dev,
                                   rt_off_t pos,
                                   const void *buffer,
                                   rt_size_t size)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;
    const rt_uint8_t *input = (const rt_uint8_t *)buffer;
    rt_size_t count;
    rt_uint8_t carriage_return = '\r';

    (void)pos;
    if ((input == RT_NULL) || (size == 0U))
    {
        return 0U;
    }

    for (count = 0U; count < size; count++)
    {
        if (((dev->flag & RT_DEVICE_FLAG_STREAM) != 0U) && (input[count] == '\n'))
        {
            if (HAL_UART_Transmit(device->huart, &carriage_return, 1U, 1000U) != HAL_OK)
            {
                break;
            }
        }

        if (HAL_UART_Transmit(device->huart, (uint8_t *)&input[count], 1U, 1000U) != HAL_OK)
        {
            break;
        }
    }

    return count;
}

static rt_err_t stm32_uart2_control(rt_device_t dev, int cmd, void *args)
{
    struct stm32_uart_device *device = (struct stm32_uart_device *)dev;
    rt_base_t level;

    switch (cmd)
    {
    case DRV_UART_CTRL_CLEAR_RX:
        level = rt_hw_interrupt_disable();
        device->rx_head = 0U;
        device->rx_tail = 0U;
        device->rx_drops = 0U;
        device->rx_errors = 0U;
        rt_hw_interrupt_enable(level);
        return RT_EOK;

    case DRV_UART_CTRL_GET_RX_DROPS:
        if (args == RT_NULL)
        {
            return -RT_EINVAL;
        }

        level = rt_hw_interrupt_disable();
        *(rt_uint32_t *)args = device->rx_drops;
        rt_hw_interrupt_enable(level);
        return RT_EOK;

    case DRV_UART_CTRL_GET_RX_ERRORS:
        if (args == RT_NULL)
        {
            return -RT_EINVAL;
        }

        level = rt_hw_interrupt_disable();
        *(rt_uint32_t *)args = device->rx_errors;
        rt_hw_interrupt_enable(level);
        return RT_EOK;

    case RT_DEVICE_CTRL_CHAR_STREAM:
        if ((args != RT_NULL) && (*(const rt_bool_t *)args == RT_FALSE))
        {
            dev->flag &= (rt_uint16_t)~RT_DEVICE_FLAG_STREAM;
        }
        else
        {
            dev->flag |= RT_DEVICE_FLAG_STREAM;
        }
        return RT_EOK;

    default:
        return -RT_ENOSYS;
    }
}

void USART2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_UART_IRQHandler(&huart2);
    rt_interrupt_leave();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2)
    {
        return;
    }

    stm32_uart2_push_rx(&g_uart2_device, g_uart2_device.rx_byte);

    if (g_uart2_device.parent.rx_indicate != RT_NULL)
    {
        (void)g_uart2_device.parent.rx_indicate(&g_uart2_device.parent, 1U);
    }

    if (g_uart2_device.rx_enabled)
    {
        if (stm32_uart2_arm_rx(&g_uart2_device) != RT_EOK)
        {
            g_uart2_device.rx_enabled = RT_FALSE;
            g_uart2_device.rx_errors++;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((huart == &huart2) && g_uart2_device.rx_enabled)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        g_uart2_device.rx_errors++;

        if (stm32_uart2_arm_rx(&g_uart2_device) != RT_EOK)
        {
            g_uart2_device.rx_enabled = RT_FALSE;
        }
    }
}

static int stm32_uart2_device_register(void)
{
    rt_err_t result;

    g_uart2_device.huart = &huart2;
    g_uart2_device.parent.type = RT_Device_Class_Char;
    g_uart2_device.parent.init = stm32_uart2_init;
    g_uart2_device.parent.open = stm32_uart2_open;
    g_uart2_device.parent.close = stm32_uart2_close;
    g_uart2_device.parent.read = stm32_uart2_read;
    g_uart2_device.parent.write = stm32_uart2_write;
    g_uart2_device.parent.control = stm32_uart2_control;

    result = rt_device_register(&g_uart2_device.parent,
                                BSP_UART2_DEVICE_NAME,
                                RT_DEVICE_FLAG_RDWR |
                                RT_DEVICE_FLAG_STREAM |
                                RT_DEVICE_FLAG_INT_RX);
    if (result != RT_EOK)
    {
        return result;
    }

    (void)rt_console_set_device(BSP_UART2_DEVICE_NAME);
    if (rt_console_get_device() != &g_uart2_device.parent)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_BOARD_EXPORT(stm32_uart2_device_register);

#endif /* RT_USING_DEVICE && RT_USING_CONSOLE */
