/**
 ****************************************************************************************************
 * @file        relay.c
 * @brief       继电器控制
 *              RELAY_1 -> PWM_CH1 -> PC9 日本电机
 *              RELAY_2 -> GPIO直驱 -> PC7 威恒电机 (保留, 未启用)
 *              RELAY_4 -> GPIO直驱 -> PG5 水泵
 *              RELAY_3 -> GPIO直驱 -> PG3 电磁阀
 *              RELAY_K15 -> GPIO直驱 -> PD7 (原直流电机正转时序)
 *              RELAY_K16 -> GPIO直驱 -> PD4 (原直流电机反转时序)
 ****************************************************************************************************
 */

#include "./HARDWARE/relay/relay.h"
#include "./HARDWARE/motor/motor.h"

/* 引脚定义 (本文件所需) */
#define P_PC9   9
#define P_PC7   7
#define P_PG3   3
#define P_PG5   5
#define P_PD7   7
#define P_PD4   4

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

#define GPIO_SET(port, pin)     do { (port)->BSRR = (1U << (pin));           } while(0)
#define GPIO_CLR(port, pin)     do { (port)->BSRR = (1U << ((pin) + 16));    } while(0)


static uint8_t g_relay_state[8] =
{
    0, RELAY_OFF, RELAY_OFF, RELAY_OFF, RELAY_OFF,
    RELAY_OFF, RELAY_OFF, RELAY_OFF
};


void relay_init(uint8_t relay_id)
{
    if (relay_id != RELAY_1) return;

    motor_init(MOTOR_1);
    relay_on(RELAY_1);
}


void relay_on(uint8_t relay_id)
{
    if (relay_id == RELAY_1)
        motor_start(MOTOR_1);
    else
        return;

    g_relay_state[relay_id] = RELAY_ON;
}


void relay_off(uint8_t relay_id)
{
    if (relay_id == RELAY_1)
        motor_stop(MOTOR_1);
    else
        return;

    g_relay_state[relay_id] = RELAY_OFF;
}


void relay_set(uint8_t relay_id, uint8_t state)
{
    if (state == RELAY_ON) relay_on(relay_id);
    else relay_off(relay_id);
}


uint8_t relay_get_state(uint8_t relay_id)
{
    if (relay_id < RELAY_1 || relay_id > RELAY_K16) return RELAY_OFF;
    return g_relay_state[relay_id];
}


void relay_toggle(uint8_t relay_id)
{
    if (g_relay_state[relay_id] == RELAY_ON)
        relay_off(relay_id);
    else
        relay_on(relay_id);
}


/* GPIO 模式配置表 */
typedef struct
{
    GPIO_TypeDef *port;
    uint8_t       pin;            /* 引脚号 0~15 */
    uint32_t      clk_bit;
} relay_gpio_cfg_t;

static const relay_gpio_cfg_t g_relay_gpio_cfg[8] =
{
    {0,     0,     0},
    {GPIOC, P_PC9, 1UL << 2},   /* RELAY_1: PC9 (PWM 路径, 仅供参考) */
    {GPIOC, P_PC7, 1UL << 2},   /* RELAY_2: PC7 威恒电机 (保留, 未启用) */
    {GPIOG, P_PG3, 1UL << 6},   /* RELAY_3: PG3 电磁阀 */
    {GPIOG, P_PG5, 1UL << 6},   /* RELAY_4: PG5 水泵 */
    {0,     0,     0},          /* 原 RELAY_5/PB1 已删除 */
    {GPIOD, P_PD7, 1UL << 3},   /* RELAY_K15: PD7 (原PB4正转时序) */
    {GPIOD, P_PD4, 1UL << 3},   /* RELAY_K16: PD4 (原PB3反转时序) */
};


void relay_gpio_init(uint8_t relay_id)
{
    const relay_gpio_cfg_t *cfg;

    if (relay_id == RELAY_1) return;
    if (relay_id < RELAY_1 || relay_id > RELAY_K16) return;

    cfg = &g_relay_gpio_cfg[relay_id];

    RCC->AHB1ENR |= cfg->clk_bit;
    GPIO_CFG(cfg->port, cfg->pin, 1U, 0U, 0U, 0U);  /* OUT, PP, LOW, NONE */
    GPIO_CLR(cfg->port, cfg->pin);
    g_relay_state[relay_id] = RELAY_OFF;
}


void relay_gpio_on(uint8_t relay_id)
{
    const relay_gpio_cfg_t *cfg;

    if (relay_id == RELAY_1) return;
    if (relay_id < RELAY_1 || relay_id > RELAY_K16) return;

    cfg = &g_relay_gpio_cfg[relay_id];
    GPIO_SET(cfg->port, cfg->pin);
    g_relay_state[relay_id] = RELAY_ON;
}


void relay_gpio_off(uint8_t relay_id)
{
    const relay_gpio_cfg_t *cfg;

    if (relay_id == RELAY_1) return;
    if (relay_id < RELAY_1 || relay_id > RELAY_K16) return;

    cfg = &g_relay_gpio_cfg[relay_id];
    GPIO_CLR(cfg->port, cfg->pin);
    g_relay_state[relay_id] = RELAY_OFF;
}
