/*
 * This file is part of the m032HidBootloader distribution (https://github.com/nimo-labs/m032HidBootloader).
 * Copyright (c) 2021 Nimolabs Ltd. www.nimo.uk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "hal.h"

/*Main processor clock */
#define UP_CLK 48000000

#define HELPER 1

/*Uart config */
#define PRINTF_BUFF_SIZE 0
#define DEBUG_UART UART_CHAN0

#if defined(__NUVO_M032K)
#define UART_0_USE_PF2_3
#define UART_CHAN0 0
#define UART_CHAN0_SERCOM 0
#define UART_CHAN0_FIFO_LEN 10
#elif defined(__SAMR21) || defined(__SAMD21)
#define UART_CHAN0 0
#define UART_CHAN0_SERCOM 0
#define UART_CHAN0_IRQ void irq_handler_sercom0(void)
#define UART_CHAN0_FIFO_LEN 10
#define UART_CHAN0_PORT SAM_GPIO_PORTA
#define UART_CHAN0_RX_PIN 5
#define UART_CHAN0_TX_PIN 4
#define UART_CHAN0_RX_PAD 1
#define UART_CHAN0_TX_PAD 0 /*1 is pad 2*/
#define UART_CHAN0_PERHIPH_RX_MUX SAM_GPIO_PMUX_D
#define UART_CHAN0_PERHIPH_TX_MUX SAM_GPIO_PMUX_D
#endif

/* USB HID */
#define USB_VID        0x0416
#define USB_PID        0x5020
#define USB_BUFFER_SIZE 64

#define USB_VID 0x0416
#define USB_PID 0x5020

#if defined(__SAMR21) || defined(__SAMD21)
#define PERHIP_CLK_GEN 0
#endif
