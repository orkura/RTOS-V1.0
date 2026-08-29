/*
 * Generated from .config by tools/kconfig_to_rtconfig.py.
 * Run menuconfig Kconfig, then rerun this script after changing options.
 */

#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_ALIGN_SIZE 4
#define RT_NAME_MAX 32
#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 1024
#define RT_DEBUG_INIT 0
#define RT_USING_SEMAPHORE
#define RT_USING_MAILBOX
#define RT_USING_HEAP
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 1024
#define RT_CONSOLE_DEVICE_NAME "uart2"
#define RT_USING_DEVICE
#define RT_USING_FINSH
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define MSH_USING_BUILT_IN_COMMANDS

#endif /* __RTTHREAD_CFG_H__ */
