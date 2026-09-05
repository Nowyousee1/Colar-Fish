/**
 ****************************************************************************************************
 * @file        usart.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-12-30
 * @brief       串口初始化函数(一般是串口1)，支持printf
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "stm32f4xx.h"


/* 串口引脚配置 (USART1: PA9=TX, PA10=RX, AF7) */
#define USART_TX_GPIO_PORT                  GPIOA
#define USART_TX_GPIO_AF                    7
#define USART_TX_GPIO_CLK_ENABLE()          do{ RCC->AHB1ENR |= 1 << 0; }while(0)

#define USART_RX_GPIO_PORT                  GPIOA
#define USART_RX_GPIO_AF                    7
#define USART_RX_GPIO_CLK_ENABLE()          do{ RCC->AHB1ENR |= 1 << 0; }while(0)

#define USART_UX                            USART1
#define USART_UX_IRQn                       USART1_IRQn
#define USART_UX_IRQHandler                 USART1_IRQHandler
#define USART_UX_CLK_ENABLE()               do{ RCC->APB2ENR |= 1 << 4; }while(0)


#define USART_REC_LEN               200
#define USART_EN_RX                 1


extern uint8_t  g_usart_rx_buf[USART_REC_LEN];
extern uint16_t g_usart_rx_sta;

void usart_init(uint32_t sclk, uint32_t baudrate);

#endif
