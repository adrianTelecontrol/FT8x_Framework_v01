
#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>

#include "helpers.h"

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
        MAP_UARTCharPut(UART0_BASE, *pui8Buffer++);
    }
}