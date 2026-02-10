#ifndef GRAPHICS_ENGINE_H_
#define GRAPHICS_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>


typedef union
{
	uint16_t u16;
	uint8_t u8[2];
} pixel16_t;

typedef struct
{
	uint16_t ui16Width;
	uint16_t ui16Height;
	uint32_t ui32BuffSize;
	pixel16_t *psPixelBuffer; 
} GfxLayer_t;

bool Gfx_initEngine(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight);

void Gfx_loadIntoBuffer(uint32_t ui32Index, uint16_t ui16Pixel);

void Gfx_render(void);

#endif