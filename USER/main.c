/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       自动化流程控制
 *              RELAY_1 (PC9, TIM8_CH4) -> 日本电机       PWM 100%
 *              RELAY_K15 (PD7, GPIO)   -> K15继电器      原直流电机正转时序
 *              RELAY_K16 (PD4, GPIO)   -> K16继电器      原直流电机反转时序
 *              RELAY_4 (PG5, GPIO)     -> 水泵           GPIO
 *              RELAY_3 (PG3, GPIO)     -> 电磁阀         GPIO
 *              Board:  ALIENTEK Explorer STM32F407 V3
 *              MCU:    STM32F407ZGT6, 168MHz
 ****************************************************************************************************
 */

#include "./USER/main.h"

/* ------------------------------------------------------------------ */
/* GPIO 单引脚寄存器操作辅助宏 (替代 sys_gpio_set)                       */
/* ------------------------------------------------------------------ */
#define GPIO_CFG(port, pin, mode, otype, speed, pupd)  do {           \
    (port)->MODER   &= ~(3U << ((pin) * 2));                           \
    (port)->MODER   |= ((mode) << ((pin) * 2));                        \
    if ((mode) == MODE_OUT || (mode) == MODE_AF) {                     \
        (port)->OSPEEDR &= ~(3U << ((pin) * 2));                       \
        (port)->OSPEEDR |= ((speed) << ((pin) * 2));                   \
        (port)->OTYPER  &= ~(1U << (pin));                             \
        (port)->OTYPER  |= ((otype) << (pin));                         \
    }                                                                  \
    (port)->PUPDR   &= ~(3U << ((pin) * 2));                           \
    (port)->PUPDR   |= ((pupd) << ((pin) * 2));                        \
} while(0)

#define GPIO_SET(port, pin)     do { (port)->BSRR = (1U << (pin));           } while(0)
#define GPIO_CLR(port, pin)     do { (port)->BSRR = (1U << ((pin) + 16));    } while(0)

/* AF 复用功能配置 */
#define GPIO_AF(port, pin, af)  do {                                        \
    (port)->AFR[(pin) >> 3] &= ~(0xFU << (((pin) & 0x7U) * 4));             \
    (port)->AFR[(pin) >> 3] |= ((uint32_t)(af) << (((pin) & 0x7U) * 4));    \
} while(0)

/* LED: PF9 */
#define LED_PORT    GPIOF
#define LED_PIN     9

/* 步进电机: 6400Hz = 32 rev/sec = 1920 RPM (1.8° 步距角, 200 脉冲/转) */
#define STEP_FREQ_HZ  6400UL


/**
 * @brief       系统时钟初始化 (HSE 8MHz -> PLL -> 168MHz)
 *              替代原 sys_stm32_clock_init()
 */
static void clock_init(void)
{
    uint32_t retry;

    /* 复位 RCC */
    RCC->CR      = 0x00000001;     /* 保持 HSI ON */
    RCC->CFGR    = 0x00000000;
    RCC->PLLCFGR = 0x00000000;
    RCC->CIR     = 0x00000000;

    /* 使能 HSE */
    RCC->CR |= 1UL << 16;
    retry = 0;
    while (((RCC->CR & (1UL << 17)) == 0) && (retry < 0x7FFF)) retry++;

    /* 电源: over-drive 模式, 支持 168MHz */
    RCC->APB1ENR |= 1UL << 28;
    PWR->CR |= 3UL << 14;

    /* PLL 配置: M=8, N=336, P=2, Q=7, 时钟源=HSE */
    RCC->PLLCFGR  = (PLL_M & 0x3FU);
    RCC->PLLCFGR |= (PLL_N << 6);
    RCC->PLLCFGR |= (((PLL_P >> 1) - 1) << 16);
    RCC->PLLCFGR |= (PLL_Q << 24);
    RCC->PLLCFGR |= 1UL << 22;

    /* AHB=168M, APB1=42M(÷4), APB2=84M(÷2) */
    RCC->CFGR |= 0UL << 4;          /* HPRE  = 0: AHB 不分频 */
    RCC->CFGR |= 5UL << 10;         /* PPRE1 = 5: APB1 4分频 */
    RCC->CFGR |= 4UL << 13;         /* PPRE2 = 4: APB2 2分频 */

    /* 使能 PLL */
    RCC->CR |= 1UL << 24;
    retry = 0;
    while ((RCC->CR & (1UL << 25)) == 0) retry++;

    /* Flash: 预取 + I-Cache + D-Cache + 5 等待周期 (168MHz 要求) */
    FLASH->ACR |= (1UL << 8) | (1UL << 9) | (1UL << 10) | (5UL << 0);

    /* 切换系统时钟到 PLL */
    RCC->CFGR |= 2UL << 0;
    while (((RCC->CFGR & 0xCU) >> 2) != 2U) {}

    /* 中断向量表: FLASH 基地址 */
    SCB->VTOR = FLASH_BASE;
}


void system_init(void)
{
    clock_init();
    delay_init(168);
    usart_init(84, 115200);

    /* GPIO 直驱先初始化, 避免引脚浮空导致继电器误闪烁 */
    relay_gpio_init(RELAY_4);   /* PG5 水泵 */
    relay_gpio_init(RELAY_3);   /* PG3 电磁阀 */
    relay_gpio_init(RELAY_K15); /* PD7 K15继电器 */
    relay_gpio_init(RELAY_K16); /* PD4 K16继电器 */

    /* PWM 电机初始化 */
    relay_init(RELAY_1);   /* PC9 日本电机 */

    /* 步进电机初始化，流程中按需启停 (PB8 PUL+ / PB9 DIR+, TB6600) */
    Stepper_Init();
    Stepper_SetSpeed(STEP_FREQ_HZ);
}


int main(void)
{
    system_init();

    /* LED0: PF9, 推挽输出, 低速, 无上下拉 */
    RCC->AHB1ENR |= 1UL << 5;
    GPIO_CFG(LED_PORT, LED_PIN, MODE_OUT, OTYPE_PP, SPEED_LOW, PUPD_NONE);
    GPIO_CLR(LED_PORT, LED_PIN);

    printf("\r\n");
    printf("========================================\r\n");
    printf(" Automation Sequence Control\r\n");
    printf(" MCU:   STM32F407ZGT6 @ 168MHz\r\n");
    printf(" CH1:   PC9 -> 日本电机 (PWM 100%%)\r\n");
    printf(" CH2:   PD7 -> K15继电器 (直流正转时序)\r\n");
    printf(" CH2:   PD4 -> K16继电器 (直流反转时序)\r\n");

    printf(" CH4:   PG5 -> 水泵 (GPIO)\r\n");
    printf(" CH3:   PG3 -> 电磁阀 (GPIO)\r\n");
    printf(" CH6:   PB8/PB9 -> 步进电机 (TB6600, 6400Hz, CW+CCW)\r\n");
    printf("========================================\r\n\r\n");

    /* 确保所有设备初始为关闭状态 */
    relay_off(RELAY_1);
    relay_gpio_off(RELAY_4);
    relay_gpio_off(RELAY_3);
    relay_gpio_off(RELAY_K15);
    relay_gpio_off(RELAY_K16);
    printf(">> [Init] All relays OFF\r\n\r\n");

    /* ================================================================
     * 自动化流程 (29s 周期, 无限循环, 周期间隔 2s)
     * ================================================================ */
    while (1)
    {

    /* ================================================================
     * Step 1: 日本电机转动 3s
     * ================================================================ */
    printf(">> Step 1/5: 日本电机 ON (3s)\r\n");
    relay_on(RELAY_1);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(3000);
    relay_off(RELAY_1);
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * Step 2: 直流电机正转 2s + 步进电机 CW
     * ================================================================ */
    printf(">> Step 2/5: K15继电器 ON (直流正转时序) + 步进电机 CW (2s)\r\n");
    relay_gpio_on(RELAY_K15);
    Stepper_SetDir(DIR_CW);
    Stepper_Start();
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(2000);
    relay_gpio_off(RELAY_K15);
    Stepper_Stop();
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * Step 3: 1) 日本电机+直流电机反转 2s -> 2) 步进电机 CCW 2s -> 3) 日本电机 3s
     * ================================================================ */

    /* Step 3-1: 日本电机 + K16继电器(直流反转时序) 同时 2s */
    printf(">> Step 3/5-1: 日本电机 + K16继电器 ON (直流反转时序) (2s)\r\n");
    relay_on(RELAY_1);
    relay_gpio_on(RELAY_K16);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(2000);
    relay_off(RELAY_1);
    relay_gpio_off(RELAY_K16);
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* Step 3-2: 步进电机 CCW 2s */
    printf(">> Step 3/5-2: 步进电机 CCW (2s)\r\n");
    Stepper_SetDir(DIR_CCW);
    Stepper_Start();
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(2000);
    Stepper_Stop();
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* Step 3-3: 日本电机 3s */
    printf(">> Step 3/5-3: 日本电机 ON (3s)\r\n");
    relay_on(RELAY_1);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(3000);
    relay_off(RELAY_1);
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * Step 4: 水泵 3s
     * ================================================================ */
    printf(">> Step 4/5: 水泵 ON (3s)\r\n");
    relay_gpio_on(RELAY_4);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(3000);
    relay_gpio_off(RELAY_4);
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * [间隔] 日本电机 3s
     * ================================================================ */
    printf(">> [间隔] 日本电机 ON (3s)\r\n");
    relay_on(RELAY_1);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(3000);
    relay_off(RELAY_1);
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * Step 5: 气泵 7s, 电磁阀 @4s 通 4s
     * ================================================================ */
    printf(">> Step 5/5: 电磁阀 @4s ON, 通 4s (原气泵路已删)\r\n");
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(4000);                  /* 原气泵(K1/PB1)时段, 仅保留延时骨架 */
    printf(">> Step 5/5: 电磁阀 ON @4s (4s)\r\n");
    relay_gpio_on(RELAY_3);          /* 电磁阀 ON @4s */
    delay_ms(4000);                  /* 电磁阀满 4s */
    relay_gpio_off(RELAY_3);         /* 电磁阀 OFF @8s */
    GPIO_CLR(LED_PORT, LED_PIN);
    delay_ms(500);

    /* ================================================================
     * [间隔] 日本电机 2s
     * ================================================================ */
    printf(">> [间隔] 日本电机 ON (2s)\r\n");
    relay_on(RELAY_1);
    GPIO_SET(LED_PORT, LED_PIN);
    delay_ms(2000);
    relay_off(RELAY_1);
    GPIO_CLR(LED_PORT, LED_PIN);

    /* 周期结束, 延时 2s 后进入下一周期 */
    printf("\r\n>> [Cycle End] Delay 2s before next cycle...\r\n\r\n");
    delay_ms(2000);

    } /* while(1) — 无限循环 */
}
