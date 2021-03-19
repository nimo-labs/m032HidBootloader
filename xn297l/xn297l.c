/*
* Copyright 2021 NimoLabs Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* File: xn297l.c
* Description: XN297L / 2.4GHz transceiver driver
*/
/**
 * The driver requires the following I/O defines to be present in the projects nimolib.h
 * \code
 * #define  BB_SPI_CHAN0
 *
 * XN297L_SPI_CHAN
 * XN297L_SS_PORT
 * XN297L_SS_PIN
 *
 * \endcode
 *
 *Basic setup of the chip involves the following:
 *\code
 * at86rf23xInit();
 * at86rf23xSetChannel(radioChan);
 * at86rf23xSetPanId(1);
 * at86rf23xSetAddr(1);
 *\endcode
 */

#include <nimolib.h>
#include <gpio.h>
#include <spi.h>
#include <delay.h>
#include <printf.h>

#include "xn297l.h"

static uint8_t xn297lReadReg(uint8_t reg);
static uint8_t xn297lWriteReg(uint8_t reg, uint8_t value);

void xn297lInit(void)
{
    printf("at86rf23xInit()...");

    /*Set CS pin as output and high*/
    GPIO_PIN_DIR(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_DIR_OUT);
    GPIO_PIN_OUT(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_OUT_HIGH);

    /*Force normal burst mode*/
    xn297lWriteReg(XN297L_REG_EN_AA, 0x00);
    xn297lWriteReg(XN297L_REG_SETUP_RETR, 0x00);
    xn297lWriteReg(XN297L_REG_DYNPD, 0x00);
    uint8_t featureReg=xn297lReadReg(XN297L_REG_FEATURE);
    xn297lWriteReg(XN297L_REG_FEATURE, featureReg & 0x07);


    /*Set address len to 3 bytes */
    xn297lWriteReg(XN297L_REG_SETUP_AW, 0x01);


    printf("Config reg: 0x%.2X\r\n", xn297lReadReg(XN297L_REG_CONFIG));
    printf("Status reg: 0x%.2X\r\n", xn297lReadReg(XN297L_REG_STATUS));

    printf("Done.\r\n");
}

void xn297lEnRxMode(void)
{
    uint8_t configReg=xn297lReadReg(XN297L_REG_CONFIG);

    configReg |= (1 << 7 /*EN_PM*/) | (1 << 3 /*PWR_UP*/);
    xn297lWriteReg(XN297L_REG_CONFIG, configReg);
}

void xn297lReadStatus(void)
{
    printf("Status reg: 0x%.2X\r\n", xn297lReadReg(XN297L_REG_STATUS));
}

void xn297lSetChannel(uint8_t rfChannel)
{
    uint8_t rfSetupReg=xn297lReadReg(XN297L_REG_RFSETUP);

    rfSetupReg &= ~(0x3F << 0); /*Clear channel*/
    rfSetupReg |= (rfChannel << 0); /*Set channel*/
    xn297lWriteReg(XN297L_REG_RFSETUP, rfSetupReg);
}

void xn297lSetDataRate(uint8_t dataRate)
{
    uint8_t rfSetupReg=xn297lReadReg(XN297L_REG_RFSETUP);

    rfSetupReg &= ~(0x03 << 6); /*Clear channel*/
    rfSetupReg |= (dataRate << 6); /*Set channel*/
    xn297lWriteReg(XN297L_REG_RFSETUP, rfSetupReg);
}


uint8_t xn297lWriteReg(uint8_t reg, uint8_t value)
{
    uint8_t regData=0;

    GPIO_PIN_OUT(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_OUT_LOW);
    spiTxByte(XN297L_SPI_CHAN, XN297L_CMD_WRITE_REG | reg);
    spiTxByte(XN297L_SPI_CHAN, value);
    GPIO_PIN_OUT(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_OUT_HIGH);

    return regData;
}

uint8_t xn297lReadReg(uint8_t reg)
{
    uint8_t regData=0;

    GPIO_PIN_OUT(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_OUT_LOW);
    spiTxByte(XN297L_SPI_CHAN, XN297L_CMD_READ_REG | reg);
    regData = spiRxByte(XN297L_SPI_CHAN);
    GPIO_PIN_OUT(XN297L_SS_PORT, XN297L_SS_PIN, GPIO_OUT_HIGH);

    return regData;
}