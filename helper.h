#if HELPER == 1
/* Helper functions */

#include <uart.h>

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
#endif