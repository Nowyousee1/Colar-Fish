/**
 * @file    stepper.c
 * @brief   Stepper motor driver — TB6600 via TIM4 CH3 (PB8 PUL+) + PB9 (DIR+)
 *
 * Driver: TB6600, common-anode wiring.
 *   PB8 (TIM4_CH3) → PUL+    PUL- → GND
 *   PB9 (GPIO)      → DIR+   DIR- → GND
 *
 * TB6600 timing (from datasheet):
 *   - Minimum pulse width:   2.5µs → max frequency ≈ 200kHz
 *   - Direction setup time:  5µs before first pulse edge
 *   - Typical optocoupler on: 5mA ~ 16mA → @3.3V ~7.8mA (marginal but functional)
 *
 * Architecture:
 *   - PB8 AF2 → TIM4_CH3 alternate function push-pull for PWM step pulses
 *   - PB9 configured as GPIO push-pull output for direction
 *   - TIM4 PWM mode 1 generates continuous pulse train
 *   - Prescaler = 83 → timer clock = 84MHz / 84 = 1MHz → 1µs timebase
 *   - ARR sets the pulse period: T_us = (ARR + 1), f_Hz = 1MHz / (ARR + 1)
 *   - CCR3 = (ARR + 1) / 2 for 50% duty cycle square wave
 *
 * STM32F4 GPIO key differences from F1:
 *   - MODER (mode) replaces CRL/CRH
 *   - OTYPER (push-pull/open-drain) replaces CNF part of CRL/CRH
 *   - OSPEEDR and PUPDR are new
 *   - AFR[2] (AFRL[0] pins 0-7, AFRH[1] pins 8-15) for alternate function selection
 */

#include "./HARDWARE/stepper/stepper.h"
#include "./SYSTEM/delay/delay.h"

/*----------------------------------------------------------------------------
 * Static helpers
 *----------------------------------------------------------------------------*/

/*
 * PB8 mode switching between GPIO output and alternate function.
 * STM32F4 MODER register: 2 bits per pin
 *   MODER[17:16]: 01 = GPIO output, 10 = AF mode
 */

#define PB8_MODER_GPIO_OUT  0x1UL    /* MODER[17:16] = 01 */
#define PB8_MODER_AF        0x2UL    /* MODER[17:16] = 10 */
#define PB8_MODER_MASK      0x3UL    /* 2-bit mask */

static uint32_t g_current_freq = 1000;  /* current step frequency in Hz */

/* Switch PB8 between GPIO output and AF mode */
static void PB8_SetMode(uint32_t mode_2bit)
{
    uint32_t moder = GPIOB->MODER;
    moder &= ~(PB8_MODER_MASK << (8 * 2));   /* clear MODER[17:16] */
    moder |= (mode_2bit << (8 * 2));          /* set new mode */
    GPIOB->MODER = moder;
}

/*----------------------------------------------------------------------------
 * Public API
 *----------------------------------------------------------------------------*/

/**
 * @brief  Initialize stepper motor GPIO and TIM4 PWM.
 *
 * Clock tree:
 *   HSE(8MHz) → PLL(×336/M=8/P=2) → 168MHz SYSCLK
 *   AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
 *   TIM4CLK = 2 × APB1CLK = 84MHz (because APB1 prescaler /4 ≠ 1)
 */
void Stepper_Init(void)
{
    /*---- 1. Enable peripheral clocks ----*/
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   /* GPIOB clock (AHB1) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;    /* TIM4 clock (APB1) */

    /*---- 2. Configure PB9 as GPIO push-pull output (DIR+) ----*/
    /*
     * MODER[19:18] = 01 (GPIO output)
     * OTYPER bit 9 = 0 (push-pull, default)
     * OSPEEDR[19:18] = 11 (100MHz)
     * PUPDR[19:18] = 00 (no pull, default)
     */
    GPIOB->MODER   &= ~(0x3UL << (9 * 2));
    GPIOB->MODER   |=  (0x1UL << (9 * 2));  /* PB9: GPIO output */
    GPIOB->OTYPER  &= ~(1UL << 9);           /* PB9: push-pull */
    GPIOB->OSPEEDR |=  (0x3UL << (9 * 2));   /* PB9: 100MHz speed */
    GPIOB->ODR     &= ~(1UL << 9);           /* PB9 LOW = CW default */

    /*---- 3. Configure PB8 as alternate function (TIM4_CH3, AF2) ----*/
    /*
     * MODER[17:16] = 10 (AF mode)
     * OSPEEDR[17:16] = 11 (100MHz)
     * AFRH[3:0] = 0010 (AF2 = TIM4_CH3)
     * OTYPER bit 8 = 0 (push-pull)
     */
    GPIOB->MODER   &= ~(0x3UL << (8 * 2));
    GPIOB->MODER   |=  (0x2UL << (8 * 2));   /* PB8: AF mode */
    GPIOB->OSPEEDR |=  (0x3UL << (8 * 2));   /* PB8: 100MHz speed */
    GPIOB->OTYPER  &= ~(1UL << 8);           /* PB8: push-pull (AF) */

    /* Set AF2 (TIM4_CH3) on PB8: AFRH[3:0] */
    GPIOB->AFR[1]  &= ~(0xFUL << ((8 - 8) * 4));
    GPIOB->AFR[1]  |=  (0x2UL << ((8 - 8) * 4));  /* PB8 → AF2 */

    /*---- 4. Configure TIM4 PWM mode 1 on Channel 3 ----*/

    /* Set prescaler: PSC = 83 → TCK = 84MHz / 84 = 1MHz */
    TIM4->PSC = STEPPER_DEFAULT_PSC;

    /* Set auto-reload for default 1kHz: ARR = 1e6/1000 - 1 = 999 */
    TIM4->ARR = 999;

    /* Configure CH3 as PWM mode 1 output */
    /*
     * CCMR2 controls CH3 and CH4.
     * For CH3 output compare (CCMR2 bits [7:0]):
     *   CC3S[1:0] = 00 → output compare
     *   OC3M[2:0] = 110 → PWM mode 1
     *   OC3PE     = 1  → preload enable
     */
    TIM4->CCMR2 &= ~(TIM_CCMR2_CC3S_Msk | TIM_CCMR2_OC3M_Msk
                   | TIM_CCMR2_OC3PE_Msk | TIM_CCMR2_OC3FE_Msk);
    TIM4->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;  /* PWM mode 1 (110) */
    TIM4->CCMR2 |= TIM_CCMR2_OC3PE_Msk;         /* Preload enable */

    /* Set duty cycle to 50%: CCR3 = (ARR + 1) / 2 = 500 */
    TIM4->CCR3 = 500;

    /* Enable CH3 output (CC3E = 1) and auto-reload preload (ARPE = 1) */
    TIM4->CCER |= TIM_CCER_CC3E_Msk;
    TIM4->CR1  |= TIM_CR1_ARPE_Msk;

    /* Generate an update event to load PSC and ARR immediately */
    TIM4->EGR  |= TIM_EGR_UG_Msk;

    /*
     * Timer is configured but NOT started.
     * Call Stepper_Start() to begin pulsing.
     */

    g_current_freq = 1000;
}

/**
 * @brief  Set rotation direction.
 * @param  dir: DIR_CW (PB9=0) or DIR_CCW (PB9=1)
 */
void Stepper_SetDir(StepperDir_TypeDef dir)
{
    if (dir == DIR_CCW)
    {
        GPIOB->BSRR = (1UL << 9);   /* PB9 HIGH = CCW */
    }
    else
    {
        GPIOB->BSRR = (1UL << (9 + 16));  /* PB9 LOW = CW (reset via upper 16 bits) */
    }
}

/**
 * @brief  Start pulse output (enable TIM4 counter).
 *         PUL+ (PB8) begins outputting step pulses immediately.
 */
void Stepper_Start(void)
{
    TIM4->CNT = 0;
    TIM4->CR1 |= TIM_CR1_CEN_Msk;
}

/**
 * @brief  Stop pulse output (disable TIM4 counter).
 *         PUL+ (PB8) stops immediately.
 */
void Stepper_Stop(void)
{
    TIM4->CR1 &= ~TIM_CR1_CEN_Msk;
    TIM4->CNT  = 0;
}

uint32_t Stepper_GetSpeed(void)
{
    return g_current_freq;
}

/**
 * @brief  Set step pulse frequency by adjusting TIM4 ARR.
 *
 * Timer timebase: TCK = 84MHz / (PSC+1) = 84MHz / 84 = 1MHz = 1µs
 * PWM period:     T_pwm = (ARR + 1) × 1µs
 * PWM frequency:  f_Hz  = 1MHz / (ARR + 1)
 * Therefore:      ARR   = 1MHz / f_Hz - 1
 *
 * TB6600 hardware limit: max 200kHz (5µs minimum pulse period).
 * This function clamps frequency to [16Hz, 200kHz].
 *
 * @param  freq_hz: desired step frequency in Hz
 */
void Stepper_SetSpeed(uint32_t freq_hz)
{
    uint32_t arr;

    if (freq_hz == 0) return;

    /* Clamp to TB6600 hardware limit (200kHz) */
    if (freq_hz > TB6600_MAX_FREQ_HZ) freq_hz = TB6600_MAX_FREQ_HZ;

    /* Compute ARR: ARR = (1MHz / freq) - 1 */
    arr = (1000000UL / freq_hz) - 1;

    /* Clamp to 16-bit ARR range */
    if (arr < STEPPER_MIN_ARR)  arr = STEPPER_MIN_ARR;
    if (arr > 65535)            arr = 65535;

    /* Update ARR and CCR3 (50% duty) */
    TIM4->ARR  = (uint16_t)arr;
    TIM4->CCR3 = (uint16_t)((arr + 1) / 2);

    /* Generate update event to immediately load new values */
    TIM4->EGR |= TIM_EGR_UG_Msk;

    g_current_freq = 1000000UL / (arr + 1);
}

/**
 * @brief  Set motor speed in RPM.
 *
 * Converts RPM to step frequency:
 *   steps_per_second = (rpm × steps_per_rev) / 60
 *
 * Example: 1.8° motor (200 steps/rev), 16× microstepping:
 *   step_per_rev = 200 × 16 = 3200
 *   At 60 RPM: freq = 60 × 3200 / 60 = 3200 Hz
 *
 * @param  rpm:          target speed in revolutions per minute
 * @param  step_per_rev: total microsteps per full revolution
 */
void Stepper_SetSpeedRPM(uint16_t rpm, uint16_t step_per_rev)
{
    uint32_t freq_hz;

    if (step_per_rev == 0) return;

    freq_hz = ((uint32_t)rpm * step_per_rev) / 60;

    Stepper_SetSpeed(freq_hz);
}

/**
 * @brief  Rotate exactly N steps (blocking).
 *
 * Temporarily disables PWM, switches PB8 to GPIO mode, and bit-bangs
 * step pulses. This guarantees an exact step count.
 *
 * After completion, the previous PWM configuration is restored but the
 * timer is left in the STOPPED state. Call Stepper_Start() to resume
 * continuous PWM operation.
 *
 * @param  steps:    number of steps to execute
 * @param  freq_hz:  pulse frequency in Hz
 * @param  dir:      direction (DIR_CW or DIR_CCW)
 */
void Stepper_RotateSteps(uint32_t steps, uint32_t freq_hz, StepperDir_TypeDef dir)
{
    uint32_t i;
    uint32_t half_period_us;

    if (steps == 0 || freq_hz == 0) return;

    /* Set direction first, then wait ≥5µs (TB6600 setup time) */
    Stepper_SetDir(dir);
    delay_us(5);

    /* Compute half-period in microseconds for 50% duty */
    half_period_us = 500000UL / freq_hz;   /* (1/f)/2 × 1e6 = 5e5/f */
    if (half_period_us < 2) half_period_us = 2;

    /* Disable timer output and switch PB8 to GPIO mode */
    TIM4->CR1  &= ~TIM_CR1_CEN_Msk;
    TIM4->CCER &= ~TIM_CCER_CC3E_Msk;
    PB8_SetMode(PB8_MODER_GPIO_OUT);

    /* Bit-bang N step pulses */
    for (i = 0; i < steps; i++)
    {
        GPIOB->BSRR = (1UL << 8);          /* PB8 HIGH */
        delay_us(half_period_us);
        GPIOB->BSRR = (1UL << (8 + 16));   /* PB8 LOW (reset via upper 16 bits) */
        delay_us(half_period_us);
    }

    /* Restore PB8 to AF mode and re-enable timer output */
    PB8_SetMode(PB8_MODER_AF);
    TIM4->CCER |= TIM_CCER_CC3E_Msk;
    /*
     * Timer remains stopped after this function.
     * User calls Stepper_Start() to resume continuous PWM.
     */
}
