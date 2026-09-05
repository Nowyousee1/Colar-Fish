/**
 ****************************************************************************************************
 * @file        relay.h
 * @brief       继电器控制模块
 *              RELAY_1: PWM模式 -> PC9 日本电机
 *              RELAY_2: GPIO模式 -> PC7 威恒电机 (保留, 未启用)
 *              RELAY_4: GPIO模式 -> PG5 水泵
 *              RELAY_3: GPIO模式 -> PG3 电磁阀
 *              RELAY_K15(6): GPIO模式 -> PD7, 时序沿用原 PB4(IN1) 正转窗口
 *              RELAY_K16(7): GPIO模式 -> PD4, 时序沿用原 PB3(IN2) 反转窗口
 ****************************************************************************************************
 */

#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f4xx.h"


/* 继电器编号 */
#define RELAY_1                         1
#define RELAY_2                         2
#define RELAY_3                         3
#define RELAY_4                         4
#define RELAY_K15                       6   /* PD7 */
#define RELAY_K16                       7   /* PD4 */

#define RELAY_ON                        1
#define RELAY_OFF                       0


void relay_init(uint8_t relay_id);                          /* 初始化指定继电器 */
void relay_on(uint8_t relay_id);                            /* PWM继电器吸合 (仅RELAY_1) */
void relay_off(uint8_t relay_id);                           /* PWM继电器断开 (仅RELAY_1) */
void relay_set(uint8_t relay_id, uint8_t state);            /* 设置状态 */
uint8_t relay_get_state(uint8_t relay_id);                  /* 获取状态 */
void relay_toggle(uint8_t relay_id);                        /* 翻转状态 */

/* GPIO模式 (威恒电机/水泵/气泵等开关量控制) */
void relay_gpio_init(uint8_t relay_id);                     /* 初始化继电器为GPIO模式 */
void relay_gpio_on(uint8_t relay_id);                       /* GPIO高电平, 继电器吸合 */
void relay_gpio_off(uint8_t relay_id);                      /* GPIO低电平, 继电器断开 */

#endif
