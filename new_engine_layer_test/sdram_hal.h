#ifndef _SDRAM_HAL_H_
#define _SDRAM_HAL_H_

#include <stdint.h>
#include <stdbool.h>


bool SDRAM_Test(const uint32_t SDRAM_APP_START_ADDRESS, const uint32_t SDRAM_ADDRESS_OFFSET, const float SDRAM_TEST_PERCENTAGE);

#endif // _SDRAM_HAL_H_