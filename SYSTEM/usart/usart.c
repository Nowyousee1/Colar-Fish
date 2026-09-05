/**
 ****************************************************************************************************
 * @file        usart.c
 * @brief       串口初始化函数(一般是串口1)，支持printf
 ****************************************************************************************************
 */

#include "./SYSTEM/usart/usart.h"

#define SYS_SUPPORT_OS  0

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

/* NVIC 配置 */
static void nvic_init(uint8_t pprio, uint8_t sprio, uint8_t ch, uint8_t group)
{
    uint32_t temp;
    uint32_t temp1;

    /* 设置优先级分组 */
    temp1 = (~group) & 0x07U;
    temp1 <<= 8;
    temp  = SCB->AIRCR;
    temp &= 0x0000F8FFU;
    temp |= 0x05FA0000U;
    temp |= temp1;
    SCB->AIRCR = temp;

    /* 设置优先级 */
    temp  = ((uint32_t)pprio) << (4U - group);
    temp |= sprio & (0x0FU >> group);
    temp &= 0x0FU;
    NVIC->ISER[ch / 32U] |= 1U << (ch % 32U);
    NVIC->IP[ch] |= temp << 4U;
}


/* 使用 os 则包含 os 头文件 */
#if SYS_SUPPORT_OS
#include "os.h"
#endif


/* printf 重定向: 不使用半主机模式 */
#if 1
#if (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");
#else
#pragma import(__use_no_semihosting)
struct __FILE
{
    int handle;
};
#endif

int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

FILE __stdout;

int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);
    USART_UX->DR = (uint8_t)ch;
    return ch;
}
#endif


#if USART_EN_RX

uint8_t  g_usart_rx_buf[USART_REC_LEN];
uint16_t g_usart_rx_sta = 0;

void USART_UX_IRQHandler(void)
{
    uint8_t rxdata;
#if SYS_SUPPORT_OS
    OSIntEnter();
#endif

    if (USART_UX->SR & (1 << 5))
    {
        rxdata = USART_UX->DR;

        if ((g_usart_rx_sta & 0x8000) == 0)
        {
            if (g_usart_rx_sta & 0x4000)
            {
                if (rxdata != 0x0a)
                    g_usart_rx_sta = 0;
                else
                    g_usart_rx_sta |= 0x8000;
            }
            else
            {
                if (rxdata == 0x0d)
                    g_usart_rx_sta |= 0x4000;
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = rxdata;
                    g_usart_rx_sta++;
                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                        g_usart_rx_sta = 0;
                }
            }
        }
    }

#if SYS_SUPPORT_OS
    OSIntExit();
#endif
}
#endif


void usart_init(uint32_t sclk, uint32_t baudrate)
{
    uint32_t temp;

    USART_TX_GPIO_CLK_ENABLE();
    USART_RX_GPIO_CLK_ENABLE();
    USART_UX_CLK_ENABLE();

    /* PA9 TX: AF 模式, 推挽, 中速, 上拉 */
    GPIO_CFG(USART_TX_GPIO_PORT, 9, 2U, 0U, 1U, 1U);
    /* PA10 RX: AF 模式, 推挽, 中速, 上拉 */
    GPIO_CFG(USART_RX_GPIO_PORT, 10, 2U, 0U, 1U, 1U);

    GPIO_AF(GPIOA, 9, USART_TX_GPIO_AF);
    GPIO_AF(GPIOA, 10, USART_RX_GPIO_AF);

    temp = (sclk * 1000000U + baudrate / 2U) / baudrate;

    USART_UX->BRR  = temp;
    USART_UX->CR1  = 0;
    USART_UX->CR1 |= 0 << 12;   /* M = 0: 8 位字长 */
    USART_UX->CR1 |= 0 << 15;   /* OVER8 = 0: 16 倍过采样 */
    USART_UX->CR1 |= 1 << 3;    /* TE: 发送使能 */
#if USART_EN_RX
    USART_UX->CR1 |= 1 << 2;    /* RE: 接收使能 */
    USART_UX->CR1 |= 1 << 5;    /* RXNEIE: 接收中断使能 */
    nvic_init(3, 3, USART_UX_IRQn, 2);  /* 分组2, 抢占优先级3, 子优先级3 */
#endif
    USART_UX->CR1 |= 1 << 13;   /* UE: 串口使能 */
}
