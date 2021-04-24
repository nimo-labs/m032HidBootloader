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

/*Standard libs*/
#include "stdbool.h"
#include "NuMicro.h"
#include "string.h"

/*Nimolib configuration*/
#include "nimolib.h"

/*Nimolib books*/
#include <gpio.h>
#include <delay.h>
#include <uart.h>
#include <simpleHid.h>
#include <intFlash.h>


extern void SYS_UnlockReg(void);

/*Local project modules*/
#include "hidBlProtocol.h"

unsigned char usbPkt[USB_BUFFER_SIZE];
bool usbDirty = FALSE;

#define BL_APPLICATION_ENTRY 0x3000
#define APP_START_RESET_VEC_ADDRESS (BL_APPLICATION_ENTRY + (uint32_t)0x00000004)

/* Helper functions */
void printStr(char *str)
{
    while(*str)
        uartTx(DEBUG_UART, *str++);
}

void startApp(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);
    uint32_t msp = *(uint32_t *)(BL_APPLICATION_ENTRY);
    __disable_irq();

    /* Rebase the Stack Pointer */
    __set_MSP(msp);
    __set_PSP(msp);

    /* Rebase the vector table base address */
    intFlashSetVectorPageAddr(BL_APPLICATION_ENTRY);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void *)*(uint32_t *)(APP_START_RESET_VEC_ADDRESS);
    /* Jump to user Reset Handler in the application */
    __enable_irq();
    application_code_entry();
}

/*---------------------------------------------------------------------------------------------------------
* Main Function
* Turns on the red LED of the NuMaker-M032LD dev board
*
* Note that the Nuvoton version of OpenOCD is required: https://github.com/OpenNuvoton/OpenOCD-Nuvoton
*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t ledLastTicks;
    struct hidBlProtocolPacket_s pkt;
    uint8_t bootSw;

    GPIO_PIN_DIR(PORTC, 14, GPIO_DIR_OUT);
    GPIO_PIN_OUT(PORTC, 14, GPIO_OUT_HIGH);

    GPIO_PIN_DIR(PORTC, 5, GPIO_DIR_OUT);
    GPIO_PIN_OUT(PORTC, 5, GPIO_OUT_LOW);

    GPIO_PIN_DIR(PORTB, 14, GPIO_DIR_IN);

    SYS_UnlockReg();
    intFlashOpen();
    FMC_ENABLE_AP_UPDATE();

    /*Read config first, if not set then setup and issue uC reboot*/
    uint32_t flashDataWord = intFlashRead(FMC_CONFIG_BASE);
    if(0x02 != ((flashDataWord & 0xC0) >> 6))
    {
        uartInit(DEBUG_UART, UART_BAUD_115200);
        printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
        printStr("Updating config\r\n");
        delaySetup(DELAY_BASE_MILLI_SEC);
        FMC_ENABLE_CFG_UPDATE();
        uint32_t flashDataWord = 0xffffffbf;
        intFlashWrite(FMC_CONFIG_BASE, flashDataWord);

        // Perform chip reset to make new User Config take effect
        delayMs(1000);
        SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
    }

    /*Check for valid App*/
    bootSw = GPIO_PIN_READ(PORTB,14);
    if(0 == bootSw)
    {
        uint32_t msp = *(uint32_t *)(BL_APPLICATION_ENTRY);
        if (0xffffffff != msp)
        {
            startApp();
        }
        else
        {
            uartInit(DEBUG_UART, UART_BAUD_115200);
            printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
            printStr("No application found\r\n");
        }
    }
    else
    {
        uartInit(DEBUG_UART, UART_BAUD_115200);
        printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
        printStr("Bootloader mode requested\r\n");
    }

    delaySetup(DELAY_BASE_MILLI_SEC);
    usbInit();

    ledLastTicks = delayGetTicks();

    while(1)
    {
        if(delayMillis(ledLastTicks, 500))
        {
            ledLastTicks = delayGetTicks();
            GPIO_PIN_TGL(PORTC, 14);
        }

        if(usbDirty)
        {
            /*Handle USB Messages*/
            hidBlProtocolDeSerialisePacket(&pkt, usbPkt);
            uint32_t dataWord;

            if(HID_BL_PROTOCOL_WRITE_INT_FLASH == pkt.packetType)
            {
                /*Make sure we don't erase ourself!*/
                if(pkt.address >= BL_APPLICATION_ENTRY)
                {
                    if(0==(pkt.address % INT_FLASH_PAGE_SIZE))
                    {
                        if(-1 == intFlashErase(pkt.address))
                            intFlashErase(pkt.address);
                    }
                    for(int i=0; i < pkt.dataLen; i+=4)
                    {
                        dataWord = (pkt.data[i+3] << 24)|(pkt.data[i+2] << 16)|(pkt.data[i+1] << 8)|(pkt.data[i]);
                        intFlashWrite(pkt.address+(i), dataWord);
                    }
                    hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                    hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                    usbSend(EP_INPUT, usbPkt, USB_BUFFER_SIZE);
                }
            }
            else if(HID_BL_PROTOCOL_ERASE_INT_FLASH == pkt.packetType) /* Jump to application*/
            {
                for(uint32_t i=BL_APPLICATION_ENTRY; i < 0x40000; i+=INT_FLASH_PAGE_SIZE)
                {
                    intFlashErase(i);
                }
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend(EP_INPUT, usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_RUN_INT == pkt.packetType) /* Jump to application*/
            {
                /*Reset to run application*/
                SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
            }
            usbDirty = 0;
        }
    }
}

void usbHidProcess(uint8_t *req)
{
    memcpy(usbPkt, req, USB_BUFFER_SIZE);
    usbDirty = true;
}