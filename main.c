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

/*Local project includes */
#if HELPER == 1
#include "helper.h"
#endif

#include "hidBlProtocol.h"
#include "version.h"
#include "commands.h"

extern void SYS_UnlockReg(void);

unsigned char usbPkt[USB_BUFFER_SIZE];
bool usbDirty = FALSE;

extern uint32_t APP_INT_FLASH_START;
extern uint32_t APP_INT_FLASH_LENGTH;

const uint32_t *appIntFlashStart = (uint32_t*)&APP_INT_FLASH_START;
const uint32_t *appIntFlashLength = (uint32_t*)&APP_INT_FLASH_LENGTH;

extern uint32_t BOOT_MAGIC_ADDRESS;
volatile uint32_t * bootMagicAddress = &BOOT_MAGIC_ADDRESS;

void startApp(void);
void validAppCheck(void);
void printVersion(void);
void m032FlashBankSwitch(void);

/*---------------------------------------------------------------------------------------------------------
* Main Function
*
* Note that the Nuvoton version of OpenOCD is required: https://github.com/OpenNuvoton/OpenOCD-Nuvoton
*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t ledLastTicks;
    struct hidBlProtocolPacket_s pkt;

    GPIO_PIN_DIR(BL_LED_PORT, BL_LED_PIN, GPIO_DIR_OUT);
    GPIO_PIN_OUT(BL_LED_PORT, BL_LED_PIN, GPIO_OUT_HIGH);
    GPIO_PIN_DIR(BL_SW_PORT, BL_SW_PIN, GPIO_DIR_IN);

#if defined(__SAMR21) || defined(__SAMD21)
    oscSet(OSC_48DFLL);
#endif

    /*Nuvoton specific flash bank switching*/
#if defined(__NUVO_M032K)
    m032FlashBankSwitch();
#endif
    /******************************************************/

    /* Checks to see if a valid app is in flash and boots if so.
    * Also checks boot switch and magic word status
    */
    validAppCheck();

    /* If here, perform bootloader function rather than boot app */

#if HELPER == 1
    printVersion();

#if defined(__NUVO_M032K)
    uint32_t serialNum[3];
    for (uint32_t i = 0; i < 3; i++)
        serialNum[i] = intFlashReadUID(i);
    printStr("Serial num: ");
    printHex(serialNum[2], true);
    printHex(serialNum[1], false);
    printHex(serialNum[0], false);
    printStr("\r\n");
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
            commandsParser(&pkt, usbPkt);
            usbDirty = 0;
        }
#if defined(__SAMR21) || defined(__SAMD21)
        usbTask();
#endif
    } /*Main while loop */
}

#if defined(__NUVO_M032K)
void m032FlashBankSwitch(void)
{
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
}
#endif

void usbHidProcess(uint8_t *req)
{
    memcpy(usbPkt, req, USB_BUFFER_SIZE);
    usbDirty = true;
}

void printVersion(void)
{
    printStr("Version: ");
    printDec(VER_MAJ);
    printStr(".");
    printDec(VER_MIN);
    printStr(".");
    printDec(VER_FORK);
    printStr("\r\n");
}

void validAppCheck(void)
{
    uint32_t bootSw;
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
}

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