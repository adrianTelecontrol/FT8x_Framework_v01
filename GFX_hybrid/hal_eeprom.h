#ifndef HAL_EEPROM_H_
#define HAL_EEPROM_H_

#include <stdbool.h>

bool HAL_EEPROM_init();

bool HAL_EEPROM_writeBytes(uint32_t addr, const uint32_t *data, uint32_t size);

bool HAL_EEPROM_readBytes(uint32_t addr, uint32_t *data, uint32_t size);



#endif // HAL_EEPROM_H_




