/**
 ****************************************************************************************************
 * @file        pwm.h
 * @brief       1路PWM驱动 (TIM8)
 *              CH1: PC9  TIM8_CH4  AF3  168MHz  PSC=167  -> 日本电机
 *              全部: ARR=999, 1KHz
 ****************************************************************************************************
 */

#ifndef __PWM_H
#define __PWM_H

#include "stm32f4xx.h"


/* 通道编号 */
#define PWM_CH1                         1

/* PWM参数 */
#define PWM_ARR                         999
#define PWM_DUTY_MAX                    PWM_ARR
#define PWM_DUTY_MIN                    0


void pwm_init(void);                                        /* 初始化PWM */
void pwm_set_duty(uint8_t ch, uint16_t duty);              /* 设置通道占空比 0~999 */
void pwm_set_percent(uint8_t ch, uint8_t percent);         /* 设置通道占空比 0~100% */

#endif
