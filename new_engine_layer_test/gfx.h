#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include <stdint.h>


#include "FT8xx.h"
#include "graphics_engine.h"
#include "Widgets.h"


bool gfx_initRegTouch(void *, widget_type_e);

TouchStatus gfx_touchReadRegion(void);

bool gfx_touchObject(RegionTouchObject, TouchStatus);

bool gfx_isWidgetTouched(GenericWidget *, TouchStatus);

bool gfx_clearSurface(Surface *);

bool gfx_renderSurface(Surface *);

void gfx_start(uint32_t colorBackground);

void gfx_end(void);

void gfx_clear(void);

void gfx_drawCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                              uint16_t color);

void gfx_drawCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                                    uint8_t cornername, uint16_t color);

void gfx_fillCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                              uint16_t color);

void gfx_fillCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                                    uint8_t corners, int16_t delta,
                                    uint16_t color);

void gfx_drawEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw, int16_t rh,
                               uint16_t color);
void gfx_fillEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw, int16_t rh,
                               uint16_t color);

void gfx_drawTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color);

void gfx_fillTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color);

void gfx_fillRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color);

#endif // GFX_H
