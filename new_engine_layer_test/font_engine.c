/**
 * font_engine.c
 */

#include <stdlib.h>
#include <string.h>

#include "FT8xx_params.h"

#include "font_engine.h"

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 800
#endif

void Gfx_DrawChar_Scaled(pixel16_t *pBuffer, int16_t *cursorX, int16_t cursorY,
                         char c, uint16_t color, uint8_t scale) {
  // 1. Bounds check and default
  if (c < g_SystemFont.firstChar || c > g_SystemFont.lastChar)
    c = '?';

  // 2. Look up the glyph
  uint16_t charIndex = c - g_SystemFont.firstChar;
  BDF_Glyph_t glyph = g_SystemFont.glyphs[charIndex];

  // 3. Scale the offsets!
  int16_t drawX = *cursorX + (glyph.xOffset * scale);
  int16_t drawY = cursorY + (glyph.yOffset * scale);

  // 4. Point to the pixel pool
  uint8_t *charPixels = &g_SystemFont.pixelPool[glyph.bitmapOffset];

  uint16_t byteIndex = 0;

  int row = 0;
  for (; row < glyph.height; row++) {
    int col = 0;
    for (; col < glyph.width; col++) {

      // Check if this specific bit is a 1
      uint8_t bitMask = 0x80 >> (col % 8);
      uint8_t currentByte = charPixels[byteIndex + (col / 8)];

      if (currentByte & bitMask) {
        // --- SCALING LOGIC ---
        // Instead of drawing 1 pixel, draw a block of (scale x scale) pixels
        int16_t baseX = drawX + (col * scale);
        int16_t baseY = drawY + (row * scale);

        int sy = 0;
        for (; sy < scale; sy++) {
          int16_t screenY = baseY + sy;

          // Vertical bounds check
          if (screenY < 0 || screenY >= SCREEN_HEIGHT)
            continue;

          uint32_t rowOffset = screenY * SCREEN_WIDTH;

          int sx = 0;
          for (; sx < scale; sx++) {
            int16_t screenX = baseX + sx;

            // Horizontal bounds check
            if (screenX >= 0 && screenX < SCREEN_WIDTH) {
              pBuffer[rowOffset + screenX].u16 = color;
            }
          }
        }
      }
    }
    // Advance to the next row of data in the BDF array
    byteIndex += (glyph.width + 7) / 8;
  }

  // 6. Advance the cursor scaled!
  *cursorX += (glyph.advanceX * scale);
}

void Gfx_DrawString(pixel16_t *pBuffer, int16_t x, int16_t y, const char *str,
                    uint16_t color, uint8_t scale) {
  int16_t cursorX = x;
  int16_t cursorY = y;

  while (*str) {
    if (*str == '\n') {
      cursorY += (g_SystemFont.yAdvance * scale); // Scaled newline
      cursorX = x;
    } else {
      // The cursorX is passed by reference so the char function updates it
      // automatically
      Gfx_DrawChar_Scaled(pBuffer, &cursorX, cursorY, *str, color, scale);
    }
    str++;
  }
}
