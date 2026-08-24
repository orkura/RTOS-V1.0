/* Minimal application used to verify the selected BSP and FinSH console. */
#include <rtthread.h>

int main(void)
{
    rt_kprintf("Hello World\n");
    rt_kprintf("FinSH is ready. Try: help, ps, free, finsh_test arg1 arg2\n");
    return RT_EOK;
}
