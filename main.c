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
#include "version.h"

unsigned char usbPkt[USB_BUFFER_SIZE];
bool usbDirty = FALSE;

#define BL_APPLICATION_ENTRY 0x3000
#define APP_START_RESET_VEC_ADDRESS (BL_APPLICATION_ENTRY + (uint32_t)0x00000004)

//#define BOOT_MAGIC_VALUE extern (*((volatile uint32_t *) &BOOT_MAGIC_ADDRESS))

extern uint32_t BOOT_MAGIC_ADDRESS;

/* Helper functions */
void printStr(char *str)
{
    unsigned char i = 0;
    char *strOrig = str;
    // while(*str)
    // {
    //     i++;
    //     *str++;
    // }
    // usbSend(EP_INPUT, strOrig, i);
    while(*str)
        uartTx(DEBUG_UART, *str++);
}

void printHex(uint32_t val)
{
    uint8_t hex[] = "0123456789ABCDEF";
    printStr("0x");
    uartTx(DEBUG_UART, hex[(val>>28) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>24) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>20) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>16) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>12) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>8) & 0xf]);
    uartTx(DEBUG_UART, hex[(val>>4) & 0xf]);
    uartTx(DEBUG_UART, hex[(val) & 0xf]);
}

void printDec(uint8_t val)
{
    uint8_t reg[3];
    if(val > 99)
    {
        reg[0] = (val / 100) + 0x30;
        val -= 100;
    }
    else
    {
        reg[0] = 0;
    }
    if(val > 9)
    {
        reg[1] = (val / 10) + 0x30;
        val -= 10;
    }
    else
    {
        reg[1] = 0;
    }
    if(val > 0)
    {
        reg[2] = val  + 0x30;
        val -= 10;
    }
    else
    {
        reg[2] = 0;
    }

    for(uint8_t i=0; i < 3; i++)
    {
        if(reg[i] > 0)
            uartTx(DEBUG_UART, reg[i]);
        else if(2 == i)
            uartTx(DEBUG_UART, 0x30);
    }
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
*
* Note that the Nuvoton version of OpenOCD is required: https://github.com/OpenNuvoton/OpenOCD-Nuvoton
*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t ledLastTicks;
    struct hidBlProtocolPacket_s pkt;
    uint32_t bootSw;

    GPIO_PIN_DIR(GPIO_PORTC, 14, GPIO_DIR_OUT);
    GPIO_PIN_OUT(GPIO_PORTC, 14, GPIO_OUT_HIGH);

    GPIO_PIN_DIR(GPIO_PORTB, 14, GPIO_DIR_IN);

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
    bootSw = GPIO_PIN_READ(GPIO_PORTB,14);
    volatile uint32_t * bootMagicAddress = &BOOT_MAGIC_ADDRESS;

    if((1 == bootSw) && (0x0000DEAD != *bootMagicAddress))
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
    *bootMagicAddress = 0xFFFFFFFF;

    // printStr("Version: ");
    // printDec(VER_MAJ);
    // printStr(".");
    // printDec(VER_MIN);
    // printStr("\r\n");

    // printStr("Serial number: ");
    // printHex(SYS->PDID);

    //usbInit();
    delaySetup(DELAY_BASE_MILLI_SEC);
    usbInit();
    ledLastTicks = delayGetTicks();

    while(1)
    {
        if(delayMillis(ledLastTicks, 500))
        {
            ledLastTicks = delayGetTicks();
            GPIO_PIN_TGL(GPIO_PORTC, 14);
        }

        if(usbDirty)
        {
            /*Handle USB Messages*/
            hidBlProtocolDeSerialisePacket(&pkt, usbPkt);
            uint32_t dataWord;

            if(HID_BL_PROTOCOL_WRITE_INT_FLASH == pkt.packetType)
            {
                // printStr("Address: ");
                // printHex(pkt.address);
                // printStr("\r\n");
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
                        // printHex(dataWord);
                        // printStr(" ");
                    }
                    // printStr("\r\n");
                    hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                    hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                    usbSend( usbPkt, USB_BUFFER_SIZE);
                }
                else
                {
                    hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
                    hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                    usbSend( usbPkt, USB_BUFFER_SIZE);
                }
            }
            else if(HID_BL_PROTOCOL_ERASE_INT_FLASH == pkt.packetType)
            {
                for(uint32_t i=BL_APPLICATION_ENTRY; i < 0x40000; i+=INT_FLASH_PAGE_SIZE)
                {
                    intFlashErase(i);
                }
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_RUN_INT == pkt.packetType) /* Jump to application*/
            {
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
                /*Reset to run application*/
                SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
            }
            else if(HID_BL_PROTOCOL_GET_MFR_ID == pkt.packetType)
            {
                uint32_t mfrId = intFlashReadCID();
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_MFR_ID, &mfrId, sizeof(mfrId));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_GET_PART_ID == pkt.packetType)
            {
                uint32_t partId = intFlashReadPID();
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_PART_ID, &partId, sizeof(partId));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_GET_BL_VER == pkt.packetType)
            {
                //          printStr("Get ver\r\n");
                uint16_t version = (VER_MAJ << 8) | VER_MIN;
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_BL_VER, &version, sizeof(version));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else /*Send NAK due to unknown command */
            {
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
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