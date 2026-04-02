#ifndef _DRAW_BITMAP_H_
#define _DRAW_BITMAP_H_

// Packs standard 8-bit R, G, B values into a 16-bit RGB565 integer
//#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

void initializeSquaresPhysics(void);

void drawSquares(pixel16_t *pPixelBuffer);

#endif
