#ifndef _IMAGE_LOADER_H
#define _IMAGE_LOADER_H

#include <stdint.h>

#include "bitmap_parser.h"
//
// EVE Image Loader Flags
//

#define EVE_LOAD_IMG_POLLING  (uint8_t)( 1 << 0 )	
#define EVE_LOAD_IMG_UDMA     (uint8_t)( 1 << 1 )

void EVE_LoadPNG(const uint8_t *pui8PNGSrc, const uint32_t ui32PNGSize);

void EVE_LoadBitmap(BitmapHandler_t *psBitmapHandler, const uint8_t ui8Flags);

void EVE_RAMG_IntegrityTest(const uint32_t ui32RAMGAddr, const uint8_t *pui8ImgSrc,  const uint32_t ui32SrcSize);

#endif // _IMAGE_LOADER_H 