/**
 ****************************************************************************************************
 * @file        motor.h
 * @brief       1路电机控制模块 (通过PWM+继电器驱动)
 *              MOTOR_1: PWM_CH1 -> PC9 -> 日本电机
 ****************************************************************************************************
 */

#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f4xx.h"


/* 电机编号 */
#define MOTOR_1                         1

#define MOTOR_SPEED_MAX                 100
#define MOTOR_SPEED_MIN                 0


void motor_init(uint8_t motor_id);                          /* 初始化电机 */
void motor_start(uint8_t motor_id);                         /* 启动(全速) */
void motor_stop(uint8_t motor_id);                          /* 停止 */
void motor_set_speed(uint8_t motor_id, uint8_t percent);    /* 调速 0~100% */

#endif
