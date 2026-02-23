/**
 */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "tiva_log.h"

#include "sdram_hal.h"

static const char TASK_NAME[] = "SDRAM_HAL";

bool SDRAM_Test(const uint32_t SDRAM_APP_START_ADDRESS, const uint32_t SDRAM_ADDRESS_OFFSET, const float SDRAM_TEST_PERCENTAGE)
{
    uint32_t *pui32SDRAM = (uint32_t*) (SDRAM_APP_START_ADDRESS
            + SDRAM_ADDRESS_OFFSET);
    uint32_t ui32ErrorCount = 0;
    const uint32_t ui32TestSize = ( 0x04000000 / sizeof(uint32_t) ) * ((fabsf(SDRAM_TEST_PERCENTAGE) > 1.0f) ? 1.0f : SDRAM_TEST_PERCENTAGE);    //2e7;
    uint32_t i = 0;

    // Setup data
    #define TRANSFER_SIZE   256 // words

    TIVA_LOGI(TASK_NAME, "Starting SDRAM Access Test...");

    // Malloc test
    uint32_t *ptr;
    uint32_t n = 1000;
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
            //TIVA_LOGI(TASK_NAME, "Correct! Malloc verification in address:\n");
            break;
        }
        // else
        // {
        //     TIVA_LOGI(TASK_NAME, "Error! Data corrupted in malloc:\n");
        // }
    }
    if(i != j)
        TIVA_LOGE(TASK_NAME, "Corrupted data in malloc verification!");
    else
        TIVA_LOGI(TASK_NAME, "Correct data verification in malloc!");

    free(ptr);
    TIVA_LOGI(TASK_NAME, "Ended malloc test");

    TIVA_LOGI(TASK_NAME, "Starting SDRAM direct addressing test");
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
                TIVA_LOGE(TASK_NAME, "Error at offset");
            }
        }
    }
    if (ui32ErrorCount == 0)
    {
        TIVA_LOGI(TASK_NAME, "SDRAM Test PASSED. Memory accessible.");
    }
    else
    {
        TIVA_LOGE(TASK_NAME, "SDRAM Test FAILED.");

        return false;
    }


    return true;
}
