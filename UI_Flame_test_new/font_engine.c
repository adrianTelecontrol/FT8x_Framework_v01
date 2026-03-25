/**
 * font_engine.c
 */

#include <stdlib.h>
#include <string.h>

#include "sdspi_hal.h"
#include "helpers.h"

#include "FT8xx_params.h"

#include "font_engine.h"

const char *FONT_BEBAS_PATH = "BEBAS32.BDF";
const char *FONT_ROBOTO_PATH = "ROBOTO.BDF";

static const char TASK_NAME[] = "fontEngine";

void gfx_GetStringDimensions(const char *str, uint8_t font, uint16_t *pWidth, uint16_t *pHeight, uint8_t scale) {
    uint16_t width = 0;

    // Safety checks
    if (!str || !pWidth || !pHeight || font >= FONT_NUMBER) return;

    // Calculate total advanceX for the single string
    while (*str) {
        char c = *str;
        if (c >= g_SystemFont[font].firstChar && c <= g_SystemFont[font].lastChar) {
            uint16_t charIndex = c - g_SystemFont[font].firstChar;
            width += g_SystemFont[font].glyphs[charIndex].advanceX;
        }
        str++;
    }

    // Pass the scaled dimensions back
    *pWidth = width * scale;
    *pHeight = g_SystemFont[font].yAdvance * scale;
}

void gfx_DrawChar(pixel16_t *pBuffer, uint8_t ui8Font, int16_t *cursorX, int16_t cursorY, char c, uint16_t color, uint16_t scale) {
	
	if(ui8Font >=  FONT_NUMBER)
		return;

	BDF_Font_t sFont = g_SystemFont[ui8Font];

  // 1. Bounds check and default
  if (c < sFont.firstChar || c > sFont.lastChar)
    c = '?';

  // 2. Look up the glyph
  uint16_t charIndex = c - sFont.firstChar;
  BDF_Glyph_t glyph = sFont.glyphs[charIndex];

  // 3. Scale the offsets!
  int16_t drawX = *cursorX + (glyph.xOffset * scale);
  int16_t drawY = cursorY + (glyph.yOffset * scale);

  // 4. Point to the pixel pool
  uint8_t *charPixels = &sFont.pixelPool[glyph.bitmapOffset];

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
          if (screenY < 0 || screenY >= LCD_HEIGHT)
            continue;

          uint32_t rowOffset = screenY * LCD_WIDTH;

          int sx = 0;
          for (; sx < scale; sx++) {
            int16_t screenX = baseX + sx;

            // Horizontal bounds check
            if (screenX >= 0 && screenX < LCD_WIDTH) {
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

void gfx_DrawString(pixel16_t *pBuffer, uint8_t font, int16_t x, int16_t y, 
                    const char *str, uint16_t color, uint8_t scale, uint8_t alignment) 
{
    if (!str || !pBuffer || font >= FONT_NUMBER) return;

    BDF_Font_t *sFont = &g_SystemFont[font];

    // 1. Calculate the scaled dimensions
    uint16_t totalWidth = 0, totalHeight = 0;
    gfx_GetStringDimensions(str, font, &totalWidth, &totalHeight, scale);
    
    // 2. Adjust starting X (Horizontal Alignment)
    int16_t cursorX = x;
    if (alignment & ALIGN_RIGHT) {
        cursorX = x - totalWidth;
    } else if (alignment & ALIGN_HCENTER) {
        cursorX = x - (totalWidth / 2);
    }

    // 3. Adjust starting Y (Vertical Alignment using Typography Math!)
    int16_t cursorY = y;
    
    // Ascent is how far the text goes ABOVE the baseline. 
    // Example: (31 + (-6)) * scale = 25 * scale.
    int16_t ascent = (sFont->yAdvance + sFont->globalYOffset) * scale;
    
    // Descent is how far the text goes BELOW the baseline (Usually negative).
    int16_t descent = (sFont->globalYOffset) * scale; 

    if (alignment & ALIGN_TOP) {
        // Push the baseline down so the top of the letters touch 'y'
        cursorY = y + ascent; 
    } 
    else if (alignment & ALIGN_VCENTER) {
        // Push baseline down by ascent, then pull it back up by half the total box
        cursorY = y + ascent - (totalHeight / 2);
    }
    else if (alignment & ALIGN_BOTTOM) {
        // Since descent is negative, adding it pulls the baseline UP from 'y'
        cursorY = y + descent; 
    }

    // 4. Fast, single-pass render loop
    while (*str) {
        gfx_DrawChar(pBuffer, font, &cursorX, cursorY, *str, color, scale);
        str++;
    }
}

bool gfx_fontLoad()
{
  // Load only Space (32) through Tilde (126)
  if (SDSPI_FetchBDF(&g_SystemFont[FONT_ROBOTO], FONT_ROBOTO_PATH, 32, 126)) {
    TIVA_LOGI(TASK_NAME, "Roboto Font parsed successfully! Pool size: %u bytes",
              g_SystemFont[FONT_ROBOTO].poolSize);
  } else {
    TIVA_LOGE(TASK_NAME, "Failed to load or parse BDF font.");
	return false;
  }
  if (SDSPI_FetchBDF(&g_SystemFont[FONT_BEBAS], FONT_BEBAS_PATH, 32, 126)) {
    TIVA_LOGI(TASK_NAME, "Bebas Font parsed successfully! Pool size: %u bytes",
              g_SystemFont[FONT_BEBAS].poolSize);
  } else {
    TIVA_LOGE(TASK_NAME, "Failed to load or parse BDF font.");
	return false;
  }

  return true;
}








