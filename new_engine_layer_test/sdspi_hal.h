#ifndef SDSPI_HAL_H_
#define SDSPI_HAL_H_

#include <stdint.h>
#include <stdbool.h>

#include <fatfs/src/ff.h>

#include "bitmap_parser.h"
 

int SDSPI_FetchFile(const char *pcFileName, uint8_t *pui32SDRAMBuff, uint32_t ui32BuffSize);

int SDSPI_FetchBitmap(const char *pcFileName, BitmapHandler_t *psBitmapHandler, const uint32_t ui32BuffSize);

bool SDSPI_FetchBDF(const char *pcFileName, uint16_t startChar, uint16_t endChar);

const char* SDSPI_StringFromFResult(FRESULT iFResult);

bool SDSPI_MountFilesystem(void);

int SDSPI_ls(int argc, char *argv[]);

int SDSPI_pwd(int argc, char *argv[]);

#endif // SDSPI_HAL_H_

