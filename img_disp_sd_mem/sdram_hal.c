/**
 */

#include <stdlib.h>
#include <stdint.h>

#include <utils/uartstdio.h>

#include "sdram_hal.h"



bool SDRAM_Test(const uint32_t SDRAM_APP_START_ADDRESS, const uint32_t SDRAM_ADDRESS_OFFSET)
{
    uint32_t *pui32SDRAM = (uint32_t*) (SDRAM_APP_START_ADDRESS
            + SDRAM_ADDRESS_OFFSET);
    uint32_t ui32ErrorCount = 0;
    const uint32_t ui32TestSize = 0x04000000 / sizeof(uint32_t);    //2e7;
    uint32_t i = 0;

    // Setup data
    #define TRANSFER_SIZE   256 // words

    UARTprintf("Starting SDRAM Access Test...\n");
    //UARTprintf("Initial address: %u MB\n", SDRAM_ADDRESS_OFFSET / 1000000);

    // Malloc test
    uint32_t *ptr;
    uint32_t n = 1000000;
    ptr = (uint32_t*) malloc(sizeof(uint32_t) * n);

    uint32_t j = 0;
    for (j = 0; j < n; j++)
    {
        ptr[j] = j;
    }

    // Verification
    for (i = 0; i < n; i++)
    {
        if (ptr[i] != i)
        {
            //UARTprintf("Correct! Malloc verification in address:\n");
            break;
        }
        // else
        // {
        //     UARTprintf("Error! Data corrupted in malloc:\n");
        // }
    }
    if(i != j)
        UARTprintf("Corrupted data in malloc verification! \n");
    else
        UARTprintf("Correct data verification in malloc! \n");

    free(ptr);
    UARTprintf("Ended malloc test\n");

    for (i = 0; i < ui32TestSize; i++)
    {
        pui32SDRAM[i] = (uint32_t) (SDRAM_APP_START_ADDRESS + i);
    }

    // --- Read & Verify ---
    for (i = 0; i < ui32TestSize; i++)
    {
        uint32_t ui32Read = pui32SDRAM[i];
        uint32_t ui32Expected = (uint32_t) (SDRAM_APP_START_ADDRESS + i);

        if (ui32Read != ui32Expected)
        {
            ui32ErrorCount++;
            if (ui32ErrorCount < 5)
            { // Print first few errors only
                UARTprintf("Error at offset\n");
            }
        }
    }
    if (ui32ErrorCount == 0)
    {
        UARTprintf("SDRAM Test PASSED. Memory accessible.\n");
    }
    else
    {
        UARTprintf("SDRAM Test FAILED.\n");

        return false;
    }


    return true;
}
