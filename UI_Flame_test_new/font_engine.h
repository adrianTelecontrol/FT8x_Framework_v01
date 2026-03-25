#ifndef FONT_ENGINE_H_
#define FONT_ENGINE_H_

#include <stdbool.h>
#include <stdint.h>

#include "graphics_engine.h"

typedef struct {
  uint32_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  int8_t xOffset;
  int8_t yOffset;
  uint8_t advanceX;
} BDF_Glyph_t;

typedef struct {
    uint16_t firstChar;
    uint16_t lastChar;
    uint8_t  yAdvance;      // Total line height (e.g., 31)
    int8_t   globalYOffset; // Distance from baseline to bottom (e.g., -6)
    BDF_Glyph_t *glyphs;
    uint8_t *pixelPool;
    uint32_t poolSize;
} BDF_Font_t;

typedef enum {
    // Horizontal Alignments (Lower Nibble)
    ALIGN_LEFT      = 0x01,
    ALIGN_RIGHT     = 0x02,
    ALIGN_HCENTER   = 0x04,
    
    // Vertical Alignments (Upper Nibble)
    ALIGN_TOP       = 0x10,
    ALIGN_BOTTOM    = 0x20,
    ALIGN_VCENTER   = 0x40,
    
    // Convenience Combinations
    ALIGN_CENTER    = (ALIGN_HCENTER | ALIGN_VCENTER)
} gfx_Align_e;

// Global font instance
#define FONT_NUMBER		2
#define FONT_ROBOTO		0
#define FONT_BEBAS		1

BDF_Font_t g_SystemFont[FONT_NUMBER];

static inline uint8_t BDF_HexToByte(const char *hex) {
  uint8_t val = 0;
  int i = 0;
  for (; i < 2; i++) {
    uint8_t c = hex[i];
    if (c >= '0' && c <= '9')
      val = (val << 4) | (c - '0');
    else if (c >= 'A' && c <= 'F')
      val = (val << 4) | (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f')
      val = (val << 4) | (c - 'a' + 10);
  }
  return val;
}


void gfx_DrawChar(pixel16_t *pBuffer, uint8_t ui8Font, int16_t *cursorX, int16_t cursorY, char c, uint16_t color, uint16_t scale);

void gfx_DrawString(pixel16_t *pBuffer, uint8_t ui8Font, int16_t x, int16_t y, const char *str, uint16_t color, uint8_t scale, uint8_t alignment);

bool gfx_fontLoad();

#endif // FONT_ENGINE_H


