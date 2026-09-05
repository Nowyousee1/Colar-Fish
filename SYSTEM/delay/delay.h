/**
 ****************************************************************************************************
 * @file        delay.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-11-26
 * @brief       使用SysTick的普通计数模式进行延时管理(支持ucosii)
 *              提供delay_init初始化函数和 delay_us微秒延时函数。
 * @license     Copyright (c) 2022-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"


void delay_init(uint16_t sysclk);   /* 初始化延时函数 */
void delay_ms(uint16_t nms);        /* 延时nms */
void delay_us(uint32_t nus);        /* 延时nus */

#endif
