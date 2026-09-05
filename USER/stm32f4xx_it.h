/**
 ****************************************************************************************************
 * @file        stm32f4xx_it.h
 * @brief       中断服务函数头文件
 * @note        SysTick_Handler 由 SYSTEM/delay/delay.c 提供
 *              USART1_IRQHandler 由 SYSTEM/usart/usart.c 提供
 ****************************************************************************************************
 */

#ifndef __STM32F4XX_IT_H
#define __STM32F4XX_IT_H

#include "stm32f4xx.h"


void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);

#endif
