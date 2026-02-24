#ifndef _DRAW_BITMAP_H_
#define _DRAW_BITMAP_H_

#define C_PURPLE 0x701F // Original: 0xFFFF0070 (R=112, G=0,   B=255)
#define C_RED 0xF800    // Original: 0xFF0000FF (R=255, G=0,   B=0)
#define C_GREEN 0x07E0  // Original: 0xFF00FF00 (R=0,   G=255, B=0)
#define C_BLUE 0x001F   // Original: 0xFFFF0000 (R=0,   G=0,   B=255)
#define C_YELLOW 0xF7E0 // Original: 0xFF00FFFF (R=255, G=255, B=0)
#define C_CYAN 0x07FE   // Original: 0xFFFFFF00 (R=0,   G=255, B=255)
#define C_ORANGE 0xFD20 // Original: 0xFF00A5FF (R=255, G=165, B=0)
#define C_PINK 0xCE1F   // Original: 0xFFFFC0CB (R=203, G=192, B=255)
#define C_LIME 0x87E0   // Original: 0xFF00FF80 (R=128, G=255, B=0)
#define C_SKY_BLUE 0x867D

#define C_MAGENTA     0xF81F    // Original: 0xFFFF00FF (R=255, G=0,   B=255)
#define C_TEAL        0x0410    // Original: 0xFF808000 (R=0,   G=128, B=128)
#define C_NAVY        0x0010    // Original: 0xFF800000 (R=0,   G=0,   B=128)
#define C_MAROON      0x8000    // Original: 0xFF000080 (R=128, G=0,   B=0)
#define C_OLIVE       0x8400    // Original: 0xFF008080 (R=128, G=128, B=0)
#define C_BROWN       0xA145    // Original: 0xFF2A2AA5 (R=165, G=42,  B=42)
#define C_GOLD        0xFEA0    // Original: 0xFF00D7FF (R=255, G=215, B=0)
#define C_CORAL       0xFBEA    // Original: 0xFF507FFF (R=255, G=127, B=80)
#define C_INDIGO      0x4810    // Original: 0xFF82004B (R=75,  G=0,   B=130)

// Packs standard 8-bit R, G, B values into a 16-bit RGB565 integer
//#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

void initializeSquaresPhysics(void);

void drawSquares(pixel16_t *pPixelBuffer);

#endif
