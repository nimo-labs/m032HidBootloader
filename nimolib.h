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

/*Main processor clock */
#define UP_CLK 48000000

/* Flash page size*/
#define INT_FLASH_PAGE_SIZE 2048

/*Uart config */
#define PRINTF_BUFF_SIZE 0
#define DEBUG_UART UART_CHAN0
#define PRINTF_UART PRINTF_USB_HID
#define UART_CHAN0 0
#define UART_CHAN0_SERCOM 0
#define UART_CHAN0_FIFO_LEN 10

/* USB HID */
#define USB_BUFFER_SIZE 64

