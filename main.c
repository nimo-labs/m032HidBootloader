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

#if defined(__SAMR21) || defined(__SAMD21)
#include <sam.h>
#else
#include "NuMicro.h"
#endif
#include "string.h"

/*Nimolib configuration*/
#include "nimolib.h"

/*Nimolib books*/
#include <gpio.h>
#include <delay.h>
#include <uart.h>
#include <simpleHid.h>
#include <intFlash.h>
#include <sysCore.h>
#if defined(__SAMR21) || defined(__SAMD21)
#include <osc.h>
#endif

#if defined(EXT_FLASH)
#include <spi.h>
#include <spiDataFlash.h>
#endif

#define HELPER 1
#include "helper.h"


extern void SYS_UnlockReg(void);

/*Local project modules*/
#include "hidBlProtocol.h"
#include "version.h"

unsigned char usbPkt[USB_BUFFER_SIZE];
bool usbDirty = FALSE;

extern uint32_t APP_INT_FLASH_START;
extern uint32_t APP_INT_FLASH_LENGTH;

const uint32_t *appIntFlashStart = (uint32_t*)&APP_INT_FLASH_START;
const uint32_t *appIntFlashLength = (uint32_t*)&APP_INT_FLASH_LENGTH;

extern uint32_t BOOT_MAGIC_ADDRESS;
volatile uint32_t * bootMagicAddress = &BOOT_MAGIC_ADDRESS;

void startApp(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);
    uint32_t msp = *(uint32_t *)(appIntFlashStart);
    __disable_irq();

    /* Rebase the Stack Pointer */
    __set_MSP(msp);
    __set_PSP(msp);

    /* Rebase the vector table base address */
#if defined(__NUVO_M032K)
    intFlashSetVectorPageAddr(appIntFlashStart);
#elif defined(__SAMR21) || defined(__SAMD21)
    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)appIntFlashStart & SCB_VTOR_TBLOFF_Msk);
#endif

    /* Load the Reset Handler address of the application */

    /*The following two lines must not be combined */
    uint32_t resetVecAddr = (uint32_t*)&APP_INT_FLASH_START;
    resetVecAddr += 4;
    /**************************/
    application_code_entry = (void *)*(uint32_t *)(resetVecAddr);
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
    GPIO_PIN_DIR(BL_LED_PORT, BL_LED_PIN, GPIO_DIR_OUT);
    GPIO_PIN_OUT(BL_LED_PORT, BL_LED_PIN, GPIO_OUT_HIGH);
    GPIO_PIN_DIR(BL_SW_PORT, BL_SW_PIN, GPIO_DIR_IN);


#if defined(__SAMR21) || defined(__SAMD21)
    oscSet(OSC_48DFLL);
#endif

    /*Nuvoton specific flash bank switching*/
#if defined(__NUVO_M032K)
    SYS_UnlockReg();
    intFlashOpen();
    FMC_ENABLE_AP_UPDATE();
    /*Read config first, if not set then setup and issue uC reboot*/
    uint32_t flashDataWord = intFlashRead(FMC_CONFIG_BASE);
    if(0x02 != ((flashDataWord & 0xC0) >> 6))
    {
#if HELPER == 1
        uartInit(DEBUG_UART, UART_BAUD_115200);
        printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
        printStr("Updating config\r\n");
#endif
        delaySetup(DELAY_BASE_MILLI_SEC);
        FMC_ENABLE_CFG_UPDATE();
        uint32_t flashDataWord = 0xffffffbf;
        intFlashWrite(FMC_CONFIG_BASE, flashDataWord);

        // Perform chip reset to make new User Config take effect
        delayMs(1000);
        SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
    }
#endif
    /******************************************************/
    /*Check for valid App*/
    bootSw = GPIO_PIN_READ(BL_SW_PORT, BL_SW_PIN);

    uint32_t msp = *(uint32_t *)(appIntFlashStart);

    if((0 == bootSw) && (0x0000DEAD != *bootMagicAddress))
    {

        if (0xffffffff != msp)
        {
            startApp();
        }
        else
        {
#if HELPER == 1
            uartInit(DEBUG_UART, UART_BAUD_115200);
            printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
            printStr("No application found\r\n");
            printHex(msp);
#endif
        }
    }
    else
    {
#if HELPER == 1
        uartInit(DEBUG_UART, UART_BAUD_115200);
        printStr("\r\n\r\nmicroNIMO Bootloader\r\n");
        printStr("Bootloader mode requested\r\n");
#endif
    }

#if HELPER == 1
    printStr("Version: ");
    printDec(VER_MAJ);
    printStr(".");
    printDec(VER_MIN);
    printStr("\r\n");

#if defined(__NUVO_M032K)
    printStr("Serial number: ");
    printHex(SYS->PDID);
#endif
#endif

    delaySetup(DELAY_BASE_MILLI_SEC);

#if defined(EXT_FLASH)
    spiInit(SPI_CHAN0);
    spiDataFlashInit(0);
#endif

    usbInit();
    ledLastTicks = delayGetTicks();

    while(1)
    {
        if(delayMillis(ledLastTicks, 500))
        {
            ledLastTicks = delayGetTicks();
            GPIO_PIN_TGL(BL_LED_PORT, BL_LED_PIN);
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
                if(pkt.address >= appIntFlashStart)
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
                for(uint32_t i=appIntFlashStart; i < 0x40000; i+=INT_FLASH_PAGE_SIZE)
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
                sysCoreuCReset();
            }
            else if(HID_BL_PROTOCOL_GET_MFR_ID == pkt.packetType)
            {
                uint32_t mfrId = intFlashReadCID();
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_MFR_ID, (unsigned char*)&mfrId, sizeof(mfrId));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_GET_PART_ID == pkt.packetType)
            {
                uint32_t partId = intFlashReadPID();
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_PART_ID, (unsigned char*)&partId, sizeof(partId));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_GET_BL_VER == pkt.packetType)
            {
                //          printStr("Get ver\r\n");
                uint16_t version = (VER_MAJ << 8) | VER_MIN;
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_SEND_BL_VER, (unsigned char*)&version, sizeof(version));
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
#if defined(EXT_FLASH)
            else if(HID_BL_PROTOCOL_ERASE_EXT_FLASH == pkt.packetType)
            {
                spiDataFlashChipErase(0);
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            else if(HID_BL_PROTOCOL_WRITE_EXT_FLASH == pkt.packetType)
            {
                printStr("Address: ");
                printHex(pkt.address);
                printStr("\r\n");
                /*Make sure we don't overwrite ourself!*/
                if(pkt.address >= 0x400)
                {
                    for(int i=0; i < pkt.dataLen; i++)
                    {
                        //dataWord = (pkt.data[i+3] << 24)|(pkt.data[i+2] << 16)|(pkt.data[i+1] << 8)|(pkt.data[i]);
                        //spiDataFlashPageWrite(0, pkt.address - BL_APPLICATION_ENTRY, pkt.data[i], 1);
                        //intFlashWrite(pkt.address+(i), dataWord);
                        printHex(dataWord);
                        printStr(" ");
                    }
                    printStr("\r\n");
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
            else if(HID_BL_PROTOCOL_COPY_EXT_TO_INT == pkt.packetType)
            {
                for(volatile uint32_t i= 0; i < appIntFlashLength; i+=4)
                {
                    uint32_t dataWord = (pkt.data[i+3] << 24)|(pkt.data[i+2] << 16)|(pkt.data[i+1] << 8)|(pkt.data[i]);
                    intFlashWrite(pkt.address+(i+appIntFlashStart), dataWord);
                }

                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
#endif
            else /*Send NAK due to unknown command */
            {
                hidBlProtocolEncodePacket(&pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
                hidBlProtocolSerialisePacket(&pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            usbDirty = 0;
        }
#if defined(__SAMR21) || defined(__SAMD21)
        usbTask();
#endif
    } /*Maine while loop */
}

void usbHidProcess(uint8_t *req)
{
    memcpy(usbPkt, req, USB_BUFFER_SIZE);
    usbDirty = true;
}