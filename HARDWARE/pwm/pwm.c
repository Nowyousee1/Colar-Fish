/**
 ****************************************************************************************************
 * @file        pwm.c
 * @brief       1路PWM驱动 (TIM8)
 *              CH1(PC9, TIM8_CH4)
 *              PWM模式1, 1KHz, 初始占空比0%
 ****************************************************************************************************
 */

#include "./HARDWARE/pwm/pwm.h"

/* 引脚与模式常量 (本文件所需) */
#define P_PC9                   9
#define M_AF                    2U
#define O_PP                    0U
#define S_HIGH                  3U
#define P_UP                    1U

/* GPIO 单引脚配置 */
#define GPIO_CFG(port, pin, mode, otype, speed, pupd)  do {           \
    (port)->MODER   &= ~(3U << ((pin) * 2));                           \
    (port)->MODER   |= ((mode) << ((pin) * 2));                        \
    if ((mode) == 1U || (mode) == 2U) {                                \
        (port)->OSPEEDR &= ~(3U << ((pin) * 2));                       \
        (port)->OSPEEDR |= ((speed) << ((pin) * 2));                   \
        (port)->OTYPER  &= ~(1U << (pin));                             \
        (port)->OTYPER  |= ((otype) << (pin));                         \
    }                                                                  \
    (port)->PUPDR   &= ~(3U << ((pin) * 2));                           \
    (port)->PUPDR   |= ((pupd) << ((pin) * 2));                        \
} while(0)

/* AF 复用功能 */
#define GPIO_AF(port, pin, af)  do {                                   \
    (port)->AFR[(pin) >> 3] &= ~(0xFU << (((pin) & 0x7U) * 4));        \
    (port)->AFR[(pin) >> 3] |= ((uint32_t)(af) << (((pin) & 0x7U) * 4)); \
} while(0)


/* 通道配置表 */
typedef struct
{
    TIM_TypeDef     *tim;
    uint8_t          tim_ch;
    uint16_t         psc;
    GPIO_TypeDef    *port;
    uint16_t         pin;
    uint8_t          af;
    uint32_t         tim_clk_bit;
} pwm_cfg_t;


static const pwm_cfg_t g_pwm_cfg[2] =
{
    {0, 0, 0, 0, 0, 0, 0},

    /* CH1: PC9, TIM8_CH4, AF3, 168MHz, PSC=167 -> 日本电机 */
    {TIM8, 4, 167, GPIOC, P_PC9, 3, 1UL << 1},
};


static void pwm_config_channel(TIM_TypeDef *tim, uint8_t tim_ch)
{
    volatile uint32_t *ccmr;
    uint32_t ccmr_shift, ccer_clr, ccer_set;

    if (tim_ch <= 2)
    {
        ccmr = &tim->CCMR1;
        ccmr_shift = (tim_ch == 1) ? 4 : 12;
        ccer_clr   = (tim_ch == 1) ? (1UL << 1) : (1UL << 5);
        ccer_set   = (tim_ch == 1) ? (1UL << 0) : (1UL << 4);
    }
    else
    {
        ccmr = &tim->CCMR2;
        ccmr_shift = (tim_ch == 3) ? 4 : 12;
        ccer_clr   = (tim_ch == 3) ? (1UL << 9)  : (1UL << 13);
        ccer_set   = (tim_ch == 3) ? (1UL << 8)  : (1UL << 12);
    }

    *ccmr &= ~(0x7UL << ccmr_shift);
    *ccmr |= (6UL << ccmr_shift) | (1UL << (ccmr_shift - 1));
    tim->CCER &= ~ccer_clr;
    tim->CCER |= ccer_set;
}


void pwm_init(void)
{
    uint8_t i;
    const pwm_cfg_t *cfg;

    /* 使能 GPIO 时钟: GPIOC (CH1) */
    RCC->AHB1ENR |= 1UL << 2;

    /* TIM8 在 APB2 */
    RCC->APB2ENR |= g_pwm_cfg[PWM_CH1].tim_clk_bit;

    for (i = PWM_CH1; i <= PWM_CH1; i++)
    {
        cfg = &g_pwm_cfg[i];

        /* GPIO: AF 模式, 推挽, 高速, 上拉 */
        GPIO_CFG(cfg->port, cfg->pin, M_AF, O_PP, S_HIGH, P_UP);
        GPIO_AF(cfg->port, cfg->pin, cfg->af);

        /* 时基: ARR=999 */
        cfg->tim->ARR = PWM_ARR;
        cfg->tim->PSC = cfg->psc;

        /* 高级定时器 (TIM8) 需要使能 MOE */
        if (cfg->tim == TIM8)
            cfg->tim->BDTR |= (1UL << 15);

        /* 启动定时器 */
        cfg->tim->CR1 |= (1UL << 7);   /* ARPE */
        cfg->tim->EGR |= (1UL << 0);   /* UG */
        cfg->tim->CR1 |= (1UL << 0);   /* CEN */

        /* PWM 通道配置 + 初始占空比 0% */
        pwm_config_channel(cfg->tim, cfg->tim_ch);
        pwm_set_duty(i, 0);
    }
}


void pwm_set_duty(uint8_t ch, uint16_t duty)
{
    const pwm_cfg_t *cfg;

    if (ch != PWM_CH1) return;
    if (duty > PWM_ARR) duty = PWM_ARR;

    cfg = &g_pwm_cfg[ch];

    switch (cfg->tim_ch)
    {
        case 1: cfg->tim->CCR1 = duty; break;
        case 2: cfg->tim->CCR2 = duty; break;
        case 3: cfg->tim->CCR3 = duty; break;
        case 4: cfg->tim->CCR4 = duty; break;
        default: break;
    }
}


void pwm_set_percent(uint8_t ch, uint8_t percent)
{
    uint16_t duty;

    if (percent > 100) percent = 100;
    duty = (uint16_t)((uint32_t)PWM_ARR * percent / 100);
    pwm_set_duty(ch, duty);
}
