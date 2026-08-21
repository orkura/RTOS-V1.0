/* Board startup for the STM32F407ZGT6 BSP. */

#include "board.h"

#include "gpio.h"
#include "rtc.h"
#include "usart.h"

static void bsp_clock_config(void)
{
    RCC_OscInitTypeDef osc_config = {0};
    RCC_ClkInitTypeDef clock_config = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc_config.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    osc_config.HSEState = RCC_HSE_ON;
    osc_config.LSEState = RCC_LSE_ON;
    osc_config.PLL.PLLState = RCC_PLL_ON;
    osc_config.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc_config.PLL.PLLM = 8U;
    osc_config.PLL.PLLN = 336U;
    osc_config.PLL.PLLP = RCC_PLLP_DIV2;
    osc_config.PLL.PLLQ = 4U;
    if (HAL_RCC_OscConfig(&osc_config) != HAL_OK)
    {
        Error_Handler();
    }

    clock_config.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                             RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock_config.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock_config.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock_config.APB1CLKDivider = RCC_HCLK_DIV4;
    clock_config.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clock_config, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

static void bsp_systick_init(void)
{
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_SYSTICK_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0x0FU, 0U);
}

void bsp_systick_handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
static rt_uint8_t g_rt_heap[BSP_RT_HEAP_SIZE];

RT_WEAK void *rt_heap_begin_get(void)
{
    return g_rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return g_rt_heap + sizeof(g_rt_heap);
}
#endif

void rt_hw_board_init(void)
{
    HAL_Init();
    bsp_clock_config();
    SystemCoreClockUpdate();
    bsp_systick_init();

    /* CubeMX owns peripheral and pin configuration. */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_RTC_Init();

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif

#ifdef RT_USING_COMPONENTS_INIT
    /* Invoke functions registered with INIT_BOARD_EXPORT(). */
    rt_components_board_init();
#endif
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
