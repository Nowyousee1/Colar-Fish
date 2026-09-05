/**
 ****************************************************************************************************
 * @file        motor.c
 * @brief       1路电机控制
 *              MOTOR_1 -> PWM_CH1 (PC9) 日本电机
 ****************************************************************************************************
 */

#include "./HARDWARE/motor/motor.h"
#include "./HARDWARE/pwm/pwm.h"


/**
 * @brief       初始化电机
 * @param       motor_id: MOTOR_1
 * @note        首次调用时初始化PWM硬件
 */
void motor_init(uint8_t motor_id)
{
    static uint8_t pwm_initialized = 0;

    if (motor_id != MOTOR_1) return;

    if (!pwm_initialized)
    {
        pwm_init();
        pwm_initialized = 1;
    }

    motor_stop(motor_id);
}


/**
 * @brief       启动电机(全速)
 * @param       motor_id: MOTOR_1
 */
void motor_start(uint8_t motor_id)
{
    if (motor_id != MOTOR_1) return;
    pwm_set_duty(motor_id, PWM_DUTY_MAX);
}


/**
 * @brief       停止电机
 * @param       motor_id: MOTOR_1
 */
void motor_stop(uint8_t motor_id)
{
    if (motor_id != MOTOR_1) return;
    pwm_set_duty(motor_id, PWM_DUTY_MIN);
}


/**
 * @brief       设置电机转速
 * @param       motor_id: MOTOR_1
 * @param       percent:  0~100 (0=停止, 100=全速)
 */
void motor_set_speed(uint8_t motor_id, uint8_t percent)
{
    if (motor_id != MOTOR_1) return;
    pwm_set_percent(motor_id, percent);
}
