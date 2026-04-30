#ifndef FILE_MANAGER_H_
#define FILE_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <fatfs/src/ff.h>

#include <fatfs/src/ffconf.h>

#include "font_engine.h"
#include "bitmap_parser.h"
#include "gfx.h"

// =====================================================================
// DEFINICIONES DE UNIDADES LÓGICAS
// =====================================================================
#define DRIVE_SD  "0:"
#define DRIVE_USB "1:"

#define DRIVE_SD_ID		0
#define DRIVE_USB_ID	1

// =====================================================================
// FUNCIONES DE LECTURA (Gráficos, Fuentes, Assets)
// =====================================================================
int FM_FetchFile(const char *drive, const char *pcFilePath, uint8_t *pui32SDRAMBuff, uint32_t ui32BuffSize);

int FM_FetchBitmap(const char *drive, const char *pcFilePath, BitmapHandler_t *psBitmapHandler, const uint32_t ui32BuffSize);

bool FM_FetchBDF(const char *drive, const char *pcFilePath, BDF_Font_t *psFont,  uint16_t startChar, uint16_t endChar);

bool FM_LoadEVEImage(const char *drive, const char *pcFilePath, gfx_Image *img, uint32_t targetRamGAddr);

// =====================================================================
// FUNCIONES DE ESCRITURA (Data Logging)
// =====================================================================
// Escribe datos crudos a un archivo (Sobrescribe si existe)
bool FM_WriteFile(const char *drive, const char *pcFilePath, const uint8_t *pData, uint32_t size);

// Añade texto al final de un archivo (Ideal para Logs). Si no existe, lo crea.
bool FM_AppendLog(const char *drive, const char *pcFilePath, const char *logText);

// =====================================================================
// UTILIDADES
// =====================================================================
const char* FM_StringFromFResult(FRESULT iFResult);

// Función genérica para listar el contenido de un directorio
int FM_ListDirectory(const char *drive, const char *pcDirPath);

#endif // FILE_MANAGER_H_


