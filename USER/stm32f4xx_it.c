/**
 ****************************************************************************************************
 * @file        stm32f4xx_it.c
 * @brief       中断服务函数 (异常处理)
 * @note        SysTick_Handler   -> SYSTEM/delay/delay.c 提供 (用于delay延时)
 *              USART1_IRQHandler -> SYSTEM/usart/usart.c 提供 (用于printf串口)
 ****************************************************************************************************
 */

#include "./USER/stm32f4xx_it.h"


/**
 * @brief       NMI 不可屏蔽中断处理
 */
void NMI_Handler(void)
{
}


/**
 * @brief       HardFault 硬错误中断处理
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}


/**
 * @brief       MemManage 内存管理错误中断处理
 */
void MemManage_Handler(void)
{
    while (1)
    {
    }
}


/**
 * @brief       BusFault 总线错误中断处理
 */
void BusFault_Handler(void)
{
    while (1)
    {
    }
}


/**
 * @brief       UsageFault 用法错误中断处理
 */
void UsageFault_Handler(void)
{
    while (1)
    {
    }
}


/**
 * @brief       SVC 系统服务调用中断处理
 */
void SVC_Handler(void)
{
}


/**
 * @brief       DebugMon 调试监视中断处理
 */
void DebugMon_Handler(void)
{
}


/**
 * @brief       PendSV 可挂起系统服务中断处理
 */
void PendSV_Handler(void)
{
}
