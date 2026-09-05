/**
 ****************************************************************************************************
 * @file        main.h
 * @brief       主程序头文件
 ****************************************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"

/* GPIO 引脚位掩码 (用于 BSRR) */
#define PIN_0                   (1U << 0)
#define PIN_1                   (1U << 1)
#define PIN_2                   (1U << 2)
#define PIN_3                   (1U << 3)
#define PIN_4                   (1U << 4)
#define PIN_5                   (1U << 5)
#define PIN_6                   (1U << 6)
#define PIN_7                   (1U << 7)
#define PIN_8                   (1U << 8)
#define PIN_9                   (1U << 9)
#define PIN_10                  (1U << 10)
#define PIN_11                  (1U << 11)
#define PIN_12                  (1U << 12)
#define PIN_13                  (1U << 13)
#define PIN_14                  (1U << 14)
#define PIN_15                  (1U << 15)

/* GPIO MODER 模式 */
#define MODE_IN                 0U
#define MODE_OUT                1U
#define MODE_AF                 2U
#define MODE_AN                 3U

/* GPIO OTYPER 输出类型 */
#define OTYPE_PP                0U
#define OTYPE_OD                1U

/* GPIO OSPEEDR 速度 */
#define SPEED_LOW               0U
#define SPEED_MID               1U
#define SPEED_FAST              2U
#define SPEED_HIGH              3U

/* GPIO PUPDR 上下拉 */
#define PUPD_NONE               0U
#define PUPD_UP                 1U
#define PUPD_DOWN               2U

/* 时钟参数 */
#define PLL_M                   8
#define PLL_N                   336
#define PLL_P                   2
#define PLL_Q                   7

#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./HARDWARE/pwm/pwm.h"
#include "./HARDWARE/motor/motor.h"
#include "./HARDWARE/relay/relay.h"
#include "./HARDWARE/stepper/stepper.h"

void system_init(void);     /* 系统初始化 */

#endif
