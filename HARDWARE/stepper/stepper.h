/**
 * @file    stepper.h
 * @brief   Stepper Motor Driver — TIM4 PWM (PB8 PUL+) + GPIO (PB9 DIR+)
 *
 * Target Hardware:
 *   - Driver:    TB6600 (common-anode wiring)
 *   - MCU:       STM32F407ZGT6 @ 168MHz, HSE 8MHz
 *   - Motor:     1.8° step angle, 200 pulses/rev (full step, no microstepping)
 *
 * Wiring (common-anode):
 *   PB8 (TIM4_CH3) → PUL+    (step pulse, PWM output)
 *   PB9 (GPIO)     → DIR+    (direction, high/low)
 *   PUL-           → GND     (common-anode return)
 *   DIR-           → GND     (common-anode return)
 *
 * TB6600 optocoupler current @ 3.3V: ~7.8mA (threshold for reliable triggering).
 * If signals are unstable, parallel a 270Ω resistor from each MCU pin to GND
 * to increase current through the optocoupler LED.
 *
 * Timer clock: TIM4CLK = 2 × APB1CLK = 84MHz (APB1=42MHz, PPRE1≠1)
 * Default prescaler: 83 → timer counter clock = 1MHz → 1µs resolution
 * ARR = (1000000 / freq_Hz) - 1
 */

#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f4xx.h"

/* Direction definitions */
typedef enum
{
    DIR_CW  = 0,    /* Clockwise / forward */
    DIR_CCW = 1     /* Counter-clockwise / reverse */
} StepperDir_TypeDef;

/* Default timer prescaler: PSC = 83 → TCK = 84MHz / (83+1) = 1MHz */
#define STEPPER_DEFAULT_PSC     (83)

/* TB6600 hardware limit: max 200kHz step frequency */
#define TB6600_MAX_FREQ_HZ      (200000UL)

/* Minimum ARR to ensure valid PWM waveform (ARR >= 2 for 50% duty) */
#define STEPPER_MIN_ARR         (2)

/* Default motor: 200 pulses/rev (1.8° full step, microstepping = 1) */
#define STEPPER_STEPS_PER_REV   (200U)

/*
 * Frequency range at default PSC=83 (timer clock = 1MHz):
 *   Max (TB6600 cap):  200kHz   → ARR = 4
 *   Min (ARR=65535):   15.3Hz   → ARR = 65535
 */

void Stepper_Init(void);
void Stepper_SetDir(StepperDir_TypeDef dir);
void Stepper_Start(void);
void Stepper_Stop(void);
uint32_t Stepper_GetSpeed(void);
void Stepper_SetSpeed(uint32_t freq_hz);
void Stepper_SetSpeedRPM(uint16_t rpm, uint16_t step_per_rev);
void Stepper_RotateSteps(uint32_t steps, uint32_t freq_hz, StepperDir_TypeDef dir);

#endif /* __STEPPER_H */
