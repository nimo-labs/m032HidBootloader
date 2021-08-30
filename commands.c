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

/*Nimolib books*/
#include <simpleHid.h>
#include <intFlash.h>
#include <sysCore.h>
#include <delay.h>
#if defined(EXT_FLASH)
#include <spi.h>
#include <spiDataFlash.h>
#endif

#include <string.h>

#include "hidBlProtocol.h"
#include "version.h"
#include "helper.h"

extern const uint32_t *appIntFlashStart;
extern const uint32_t *appIntFlashLength;

void commandsParser(struct hidBlProtocolPacket_s *pkt, unsigned char * usbPkt)
{
    uint32_t dataWord = 0;
    if(HID_BL_PROTOCOL_WRITE_INT_FLASH == pkt->packetType)
    {
        // printStr("Address: ");
        // printHex(pkt.address);
        // printStr("\r\n");
        /*Make sure we don't erase ourself!*/
        if(pkt->address >= (uint32_t)appIntFlashStart)
        {
            if(0==(pkt->address % INT_FLASH_PAGE_SIZE))
            {
                if(-1 == intFlashErase(pkt->address))
                    intFlashErase(pkt->address);
            }

            for(int i=0; i < pkt->dataLen; i+=4)
            {
                dataWord = (pkt->data[i+3] << 24)|(pkt->data[i+2] << 16)|(pkt->data[i+1] << 8)|(pkt->data[i]);
                intFlashWrite(pkt->address+(i), dataWord);
                // printHex(dataWord);
                // printStr(" ");
            }
            // printStr("\r\n");
            hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
            hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
            usbSend( usbPkt, USB_BUFFER_SIZE);
        }
        else
        {
            hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
            hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
            usbSend( usbPkt, USB_BUFFER_SIZE);
        }
    }
    else if(HID_BL_PROTOCOL_ERASE_INT_FLASH == pkt->packetType)
    {
        for(uint32_t i=(uint32_t)appIntFlashStart; i < 0x40000; i+=INT_FLASH_PAGE_SIZE)
        {
            intFlashErase(i);
        }
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
    else if(HID_BL_PROTOCOL_RUN_INT == pkt->packetType) /* Jump to application*/
    {
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
        /*Reset to run application*/
        sysCoreuCReset();
    }
    else if(HID_BL_PROTOCOL_GET_MFR_ID == pkt->packetType)
    {
        uint32_t mfrId = intFlashReadCID();
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_SEND_MFR_ID, (unsigned char*)&mfrId, sizeof(mfrId));
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
    else if(HID_BL_PROTOCOL_GET_PART_ID == pkt->packetType)
    {
        uint32_t partId = intFlashReadPID();
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_SEND_PART_ID, (unsigned char*)&partId, sizeof(partId));
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
    else if(HID_BL_PROTOCOL_GET_BL_VER == pkt->packetType)
    {
        //          printStr("Get ver\r\n");
        uint16_t version = (VER_MAJ << 8) | VER_MIN;
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_SEND_BL_VER, (unsigned char*)&version, sizeof(version));
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
#if defined(EXT_FLASH)
    else if(HID_BL_PROTOCOL_ERASE_EXT_FLASH == pkt->packetType)
    {
        if(0 == (EXT_FLASH_APP_LOC % 0x1000))
        {
            unsigned long millis = delayGetTicks();
            for(unsigned int i=EXT_FLASH_APP_LOC; i < SPI_DATAFLASH_SIZE; i+=0x1000)
            {
                spiDataFlash4kErase(0, i);
                if(delayMillis(millis, 1000))
                {
                    millis = delayGetTicks();
                    //  printStr("Send wait\r\n");
                    hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_USB_WAIT, NULL, 0);
                    hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
                    usbSend( usbPkt, USB_BUFFER_SIZE);
                }
            }

            hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
            hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
            usbSend( usbPkt, USB_BUFFER_SIZE);
        }
        else
        {
            hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
            hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
            usbSend( usbPkt, USB_BUFFER_SIZE);
        }
    }
    else if(HID_BL_PROTOCOL_WRITE_EXT_FLASH == pkt->packetType)
    {
        // printStr("Address: ");
        // printHex(pkt->address);
        // printStr("\r\n");
        for(int i=0; i < pkt->dataLen; i++)
        {
            spiDataFlashPageWrite(0, pkt->address+EXT_FLASH_APP_LOC+i, &pkt->data[i], 1);
            // printHex(pkt->data[i]);
            // printStr(" ");
        }
        //printStr("\r\n");
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
    else if(HID_BL_PROTOCOL_COPY_EXT_TO_INT == pkt->packetType)
    {
        // printStr("Copying ext to int\r\n");
        uint8_t buf[16];
        unsigned long millis = delayGetTicks();
        for(volatile uint32_t i= 0; i < (uint32_t)appIntFlashStart; i+=4)
        {
            if(0==((uint32_t)(i+0x3000) % INT_FLASH_PAGE_SIZE))
            {
                if(-1 == intFlashErase((uint32_t)(i+(uint32_t)appIntFlashStart)))
                    intFlashErase((uint32_t)(i+(uint32_t)appIntFlashStart));
            }
            if(delayMillis(millis, 1000))
            {
                millis = delayGetTicks();
                //  printStr("Send wait\r\n");
                hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_USB_WAIT, NULL, 0);
                hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
                usbSend( usbPkt, USB_BUFFER_SIZE);
            }
            spiDataFlashReadData(0, EXT_FLASH_APP_LOC+i, buf, 4);
            uint32_t dataWord = (buf[3] << 24)|(buf[2] << 16)|(buf[1] << 8)|(buf[0]);
            intFlashWrite((uint32_t)(i+(uint32_t)appIntFlashStart), dataWord);
        }
        //printStr("Done\r\n");
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
    else if(HID_BL_PROTOCOL_READ_EXT_FLASH == pkt->packetType)
    {
        uint8_t buf[16];
        // printStr("Reading ext flash\r\n");
        spiDataFlashReadData(0, EXT_FLASH_APP_LOC, buf, 16);
        //printStr("Done\r\n");
        // for(int i=0; i < 16; i++)
        // {
        //     printHex(buf[i]);
        //     printStr(" ");
        // }
        // printStr("\r\n");
        delayMs(3000);
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_ACK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
        // printStr("Sent\r\n");
    }
#endif
    else /*Send NAK due to unknown command */
    {
        hidBlProtocolEncodePacket(pkt, 0, HID_BL_PROTOCOL_NAK, NULL, 0);
        hidBlProtocolSerialisePacket(pkt, usbPkt, USB_BUFFER_SIZE);
        usbSend( usbPkt, USB_BUFFER_SIZE);
    }
}