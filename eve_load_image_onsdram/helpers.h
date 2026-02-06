#ifndef _HELPERS_H
#define _HELPERS_H

/**
 * Helper functions
 */

#include <stdint.h>

extern uint32_t g_ui32SysClock;

/*
 * MS_2_CLK
*/
#define MS_2_CLK(ms) ((uint32_t)(( (g_ui32SysClock) / (3.0f) ) / ((1.0f) / ((ms) / (1000.0f)))))

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count);

#endif // _HELPERS_H


