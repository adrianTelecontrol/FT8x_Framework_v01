/**
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "utils/uartstdio.h"

#include "EVE.h"
#include "EVE_colors.h"
#include "FT8xx_params.h"
#include "font_engine.h"
#include "graphics_engine.h"


#include "gfx.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

#define gfx_write_pixel(pBuf, x, y, color)                                     \
  do {                                                                         \
    if ((x) >= 0 && (x) < LCD_WIDTH && (y) >= 0 && (y) < LCD_HEIGHT) {         \
      (pBuf)[((y) * LCD_WIDTH) + (x)].u16 = (color);                           \
    }                                                                          \
  } while (0)

uint16_t blendRGB565(uint16_t colorTop, uint16_t colorBottom, int16_t currentY, int16_t totalH) {
    if (totalH <= 1) return colorTop; // Protección contra división por cero

    // 1. Extraer canales (desempaquetar 5-6-5)
    int32_t rT = (colorTop >> 11) & 0x1F;
    int32_t gT = (colorTop >> 5)  & 0x3F;
    int32_t bT = colorTop         & 0x1F;

    int32_t rB = (colorBottom >> 11) & 0x1F;
    int32_t gB = (colorBottom >> 5)  & 0x3F;
    int32_t bB = colorBottom         & 0x1F;

    // 2. Calcular la proporción (Escalada por 256 para evitar floats)
    int32_t ratio = (currentY * 256) / totalH; 

    // 3. Interpolar canales
    uint16_t r = rT + (((rB - rT) * ratio) >> 8);
    uint16_t g = gT + (((gB - gT) * ratio) >> 8);
    uint16_t b = bT + (((bB - bT) * ratio) >> 8);

    // 4. Empaquetar de vuelta a RGB565
    return (r << 11) | (g << 5) | b;
}

bool gfx_initRegTouch(void *widget, widget_type_e type) {
  if (type == WD_TYPE_BUTTON) {
    gfx_Button *wd = (gfx_Button *)widget;
	wd->regTouch.x1 = wd->pos.x;
    wd->regTouch.x2 = wd->pos.x + wd->size.width;
    wd->regTouch.y1 = wd->pos.y;
    wd->regTouch.y2 = wd->pos.y + wd->size.height;
  }

  return true;
}

void gfx_calibrate(void) {
  API_LIB_BeginCoProList(); // Begin new screen
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0); // Clear screen
  API_CLEAR(1, 1, 1);
  API_CMD_TEXT(80, 30, 27, OPT_CENTER, "Presiona los puntos...");
  API_CMD_CALIBRATE(0xAAAAAAAA);
  API_DISPLAY();             // Tell EVE that this is end of list
  API_CMD_SWAP();            // Swap buffers in EVE to make this list active
  API_LIB_EndCoProList();    // Finish the co-processor list burst write
  API_LIB_AwaitCoProEmpty(); // Wait until co-processor has consumed all
                             // commands
}

TouchStatus gfx_touchReadRegion(void) {
  uint32_t regTouch;
  uint16_t touch_x;
  uint16_t touch_y;
  float_t xScreen;
  float_t yScreen;
  TouchStatus touch;

  regTouch = EVE_MemRead32(REG_TOUCH_SCREEN_XY); // Lee el registro del touch
  if (regTouch != 0X80008000)                    // y verifica si fue tocado
  {
    // Obtiene las coordenadas segï¿½n el sensor
    touch_x = (uint16_t)(regTouch >> 16);
    touch_y = (uint16_t)regTouch & 0xFFFF;
    // Si es mayor a la regiï¿½n del sensor no fue tocada la pantalla
    if ((touch_x > 800) || (touch_y > 480)) {
      xScreen = 0;
      yScreen = 0;
      touch.state = false;
    } else // Si las coordenas estï¿½n de la pantalla
    {
      xScreen = (float_t)touch_x * (float_t)950 / 965;
      yScreen = touch_y;

      touch.state = true;
    }
    regTouch = 0X80008000;
  } else {
    touch.state = false;
  }

  touch.x = (uint16_t)xScreen;
  touch.y = (uint16_t)yScreen;

  return touch;
}

bool gfx_touchObject(RegionTouchObject regObj, TouchStatus touch) {
  // Si queda dentro de la regiï¿½n deseada, entonces si fue tocado
  if ((touch.x >= regObj.x1) && (touch.x <= regObj.x2) &&
      (touch.y >= regObj.y1) && (touch.y <= regObj.y2))
    return true;

  return false;
}

bool gfx_compositeFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer) {
  if (srf == NULL) {
    UARTprintf("Cannot render canvas! Is empty.");
    return false;
  }

  gfx_GenericWidgetNode *iter = srf->psWidgets;
  
  while (iter != NULL) {
    switch (iter->sWidget.eWidgetType) {
    case WD_TYPE_BUTTON:
      gfx_drawButton(psPixelBuffer, (gfx_Button *)iter->sWidget.pvWidget);
      break;
    case WD_TYPE_RECT:
      gfx_drawRectangle(psPixelBuffer, (gfx_Rectangle *)iter->sWidget.pvWidget);
      break;
    case WD_TYPE_LABEL:
      gfx_drawLabel(psPixelBuffer, (gfx_Label *)iter->sWidget.pvWidget);
      break;
    default:
      break;
    }

    iter = iter->psNext;
  }

  return true;
}

bool gfx_isWidgetTouched(gfx_GenericWidget *wd, TouchStatus touch) {
  if (wd->pvWidget == NULL)
    return false;

  bool ret = false;
  switch (wd->eWidgetType) {
  case WD_TYPE_BUTTON:
    ret = gfx_touchObject(((gfx_Button *)wd->pvWidget)->regTouch, touch);
    break;
  default:
    break;
  }

  return ret;
}

void gfx_start(uint32_t colorBackground) {
  API_LIB_BeginCoProList(); // Begin new screen
  API_CMD_DLSTART();

  API_CLEAR_COLOR_RGB((uint8_t)(colorBackground >> 16),
                      (uint8_t)(colorBackground >> 8),
                      (uint8_t)colorBackground);
  API_CLEAR(1, 1, 1); // Tell EVE that this is end of list
}

void gfx_end(void) {
  API_DISPLAY();  // Ends the diplay cmd list
  API_CMD_SWAP(); // Swap buffers in EVE to make this list active

  // EVE_Flush_Buffer();
  API_LIB_EndCoProList(); // Finish the co-processor list burst write
  API_LIB_AwaitCoProEmpty();
}

void gfx_clear(void) {
  gfx_start(EVE_BLACK);
  gfx_end();
}

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void gfx_writeLine(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                   int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      gfx_write_pixel(pBuf, y0, x0, color);
    } else {
      gfx_write_pixel(pBuf, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void gfx_drawFastVLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t h,
                       uint16_t color) {
  gfx_writeLine(pBuf, x, y, x, y + h - 1, color);
}

void gfx_drawFastHLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       uint16_t color) {
  gfx_writeLine(pBuf, x, y, x + w - 1, y, color);
}

void gfx_fillRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color) {
  int16_t i = x;
  for (; i < x + w; i++) {
    gfx_drawFastVLine(pBuf, i, y, h, color);
  }
}

void gfx_drawCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  gfx_write_pixel(pBuf, x0, y0 + r, color);
  gfx_write_pixel(pBuf, x0, y0 - r, color);
  gfx_write_pixel(pBuf, x0 + r, y0, color);
  gfx_write_pixel(pBuf, x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 + y, y0 + x, color);
    gfx_write_pixel(pBuf, x0 - y, y0 + x, color);
    gfx_write_pixel(pBuf, x0 + y, y0 - x, color);
    gfx_write_pixel(pBuf, x0 - y, y0 - x, color);
  }
}

void gfx_drawCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t cornername, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
      gfx_write_pixel(pBuf, x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
      gfx_write_pixel(pBuf, x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      gfx_write_pixel(pBuf, x0 - y, y0 + x, color);
      gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      gfx_write_pixel(pBuf, x0 - y, y0 - x, color);
      gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    }
  }
}

void gfx_fillCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) {
  gfx_drawFastVLine(pBuf, x0, y0 - r, 2 * r + 1, color);
  gfx_fillCircleHelper(pBuf, x0, y0, r, 3, 0, color);
}

void gfx_fillCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t corners, int16_t delta, uint16_t color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        gfx_drawFastVLine(pBuf, x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        gfx_drawFastVLine(pBuf, x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        gfx_drawFastVLine(pBuf, x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        gfx_drawFastVLine(pBuf, x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void gfx_drawEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color) {
  // Bresenham's ellipse algorithm
  int16_t x = 0, y = rh;
  int32_t rw2 = rw * rw, rh2 = rh * rh;
  int32_t twoRw2 = 2 * rw2, twoRh2 = 2 * rh2;

  int32_t decision = rh2 - (rw2 * rh) + (rw2 / 4);

  // region 1
  while ((twoRh2 * x) < (twoRw2 * y)) {
    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    x++;
    if (decision < 0) {
      decision += rh2 + (twoRh2 * x);
    } else {
      decision += rh2 + (twoRh2 * x) - (twoRw2 * y);
      y--;
    }
  }

  // region 2
  decision = ((rh2 * (2 * x + 1) * (2 * x + 1)) >> 2) +
             (rw2 * (y - 1) * (y - 1)) - (rw2 * rh2);
  while (y >= 0) {
    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    y--;
    if (decision > 0) {
      decision += rw2 - (twoRw2 * y);
    } else {
      decision += rw2 + (twoRh2 * x) - (twoRw2 * y);
      x++;
    }
  }
}

void gfx_fillEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color) {
  // Bresenham's ellipse algorithm
  int16_t x = 0, y = rh;
  int32_t rw2 = rw * rw, rh2 = rh * rh;
  int32_t twoRw2 = 2 * rw2, twoRh2 = 2 * rh2;

  int32_t decision = rh2 - (rw2 * rh) + (rw2 / 4);

  // region 1
  while ((twoRh2 * x) < (twoRw2 * y)) {
    x++;
    if (decision < 0) {
      decision += rh2 + (twoRh2 * x);
    } else {
      decision += rh2 + (twoRh2 * x) - (twoRw2 * y);
      gfx_drawFastHLine(pBuf, x0 - (x - 1), y0 + y, 2 * (x - 1) + 1, color);
      gfx_drawFastHLine(pBuf, x0 - (x - 1), y0 - y, 2 * (x - 1) + 1, color);
      y--;
    }
  }

  // region 2
  decision = ((rh2 * (2 * x + 1) * (2 * x + 1)) >> 2) +
             (rw2 * (y - 1) * (y - 1)) - (rw2 * rh2);
  while (y >= 0) {
    gfx_drawFastHLine(pBuf, x0 - x, y0 + y, 2 * x + 1, color);
    gfx_drawFastHLine(pBuf, x0 - x, y0 - y, 2 * x + 1, color);

    y--;
    if (decision > 0) {
      decision += rw2 - (twoRw2 * y);
    } else {
      decision += rw2 + (twoRh2 * x) - (twoRw2 * y);
      x++;
    }
  }
}

void gfx_drawRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  gfx_drawFastHLine(pBuf, x + r, y, w - 2 * r, color);         // Top
  gfx_drawFastHLine(pBuf, x + r, y + h - 1, w - 2 * r, color); // Bottom
  gfx_drawFastVLine(pBuf, x, y + r, h - 2 * r, color);         // Left
  gfx_drawFastVLine(pBuf, x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  gfx_drawCircleHelper(pBuf, x + r, y + r, r, 1, color);
  gfx_drawCircleHelper(pBuf, x + w - r - 1, y + r, r, 2, color);
  gfx_drawCircleHelper(pBuf, x + w - r - 1, y + h - r - 1, r, 4, color);
  gfx_drawCircleHelper(pBuf, x + r, y + h - r - 1, r, 8, color);
}

void gfx_fillRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  gfx_fillRect(pBuf, x + r, y, w - 2 * r, h, color);
  // draw four corners
  gfx_fillCircleHelper(pBuf, x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  gfx_fillCircleHelper(pBuf, x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void gfx_fillGradientRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t colorTop, uint16_t colorBottom) {
    int16_t max_radius = ((w < h) ? w : h) / 2;
    if (r > max_radius) r = max_radius;
    if (r > 32) r = 32; // Límite de seguridad para el arreglo

    // 1. Precalcular los recortes de las esquinas para evitar usar sqrt()
    int16_t cornerX[32] = {0};
	int16_t i = 0;
    for (; i < r; i++) {
        int32_t dy = r - i;
        int32_t r2 = r * r;
        int32_t dx = 0;
        
        // Algoritmo rápido para encontrar la X del círculo en esta Y
        while ((dx * dx) + (dy * dy) <= r2) { dx++; }
        dx--; 
        
        cornerX[i] = r - dx; // Píxeles vacíos en los bordes
    }

    // 2. Renderizar fila por fila de arriba a abajo
	int16_t row = 0;
    for (; row < h; row++) {
        uint16_t rowColor = blendRGB565(colorTop, colorBottom, row, h);

        int16_t startX = 0;
        if (row < r) {
            startX = cornerX[row];         // Curvatura superior
        } else if (row >= h - r) {
            startX = cornerX[h - 1 - row]; // Curvatura inferior
        }

        int16_t drawW = w - (startX * 2);
        int16_t drawX = x + startX;

        // Dibujar la línea horizontal para esta fila
        gfx_fillRect(pBuf, drawX, y + row, drawW, 1, rowColor);
    }
}

void gfx_drawTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  gfx_writeLine(pBuf, x0, y0, x1, y1, color);
  gfx_writeLine(pBuf, x1, y1, x2, y2, color);
  gfx_writeLine(pBuf, x2, y2, x0, y0, color);
}

void gfx_fillTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    gfx_drawFastHLine(pBuf, a, y0, b - a + 1, color);
    return;
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
    last = y1; // Include y1 scanline
  else
    last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    gfx_drawFastHLine(pBuf, a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    gfx_drawFastHLine(pBuf, a, y, b - a + 1, color);
  }
}

//
// **************** Widget Functions **************************
//
/*
void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn) {
  if (!pBuf || !btn)
    return;

  // Default to the normal colors defined in the struct
  uint16_t activeBgColor = btn->backgroundColor;
  uint16_t activeBorderColor = btn->borderColor;
  uint16_t activeTextColor = btn->textColor;

  // Override colors based on the current physical state
  if (btn->state == BTN_STATE_PRESSED) {
    // Darken the background and border to simulate a physical push
    activeBgColor = DARKEN_COLOR(btn->backgroundColor);
    activeBorderColor = DARKEN_COLOR(btn->borderColor);
  } else if (btn->state == BTN_STATE_DISABLED) {
    // "Gray out" the button to show it cannot be interacted with
    activeBgColor = 0x4208;     // C_DARK_GRAY
    activeBorderColor = 0x8410; // C_GRAY
    activeTextColor = 0x8410;   // C_GRAY
  }

  // 1. Draw Border (Using the active border color)
  if (btn->borderWidth) {
    gfx_fillRoundRect(pBuf, btn->pos.x - btn->borderWidth,
                      btn->pos.y - btn->borderWidth,
                      btn->size.width + btn->borderWidth * 2,
                      btn->size.height + btn->borderWidth * 2, btn->radius,
                      activeBorderColor);
  }

  // 2. Draw Background (Using the active background color)
  gfx_fillRoundRect(pBuf, btn->pos.x, btn->pos.y, btn->size.width,
                    btn->size.height, btn->radius, activeBgColor);

  // 3. Draw Label (Using the active text color)
  gfx_DrawString(pBuf, btn->font, btn->pos.x + btn->size.width / 2.0f,
                 btn->pos.y + btn->size.height / 2.0f, btn->label,
                 activeTextColor, btn->fontScale, ALIGN_CENTER);
}*/


// Pre-calcula los píxeles vacíos en el borde de la esquina para evitar sqrt() en el bucle principal.
// cornerX guarda la 'X delta' (píxeles vacíos) para cada 'dy' (fila de la esquina).
static void gfx_getCornerEmptyPixels(int16_t *cornerX, int16_t r) {
    if (r <= 0) return;
    if (r > 32) r = 32; // Límite de seguridad del array

	int16_t i = 0;
    for (; i < r; i++) {
        int32_t dy = r - i;
        int32_t r2 = r * r;
        int32_t dx = 0;
        
        // Algoritmo rápido de círculo entero: encontrar la X máxima dentro del círculo a esta Y.
        while ((dx * dx) + (dy * dy) <= r2) { dx++; }
        dx--; 
        
        cornerX[i] = r - dx; // Número de píxeles vacíos en el borde
    }
}

// Dibuja un marco de contorno redondeado geométricamente perfecto usando la misma lógica que el relleno de degradado.
// x, y, w, h representan la huella GENERAL (footprint) del objeto incluyendo el borde.
void gfx_drawRoundOutline(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r_outer, int16_t borderWidth, uint16_t color) {
    if (!pBuf || borderWidth <= 0) return;

    // Geometría Exterior
    int16_t max_r_outer = ((w < h) ? w : h) / 2;
    if (r_outer > max_r_outer) r_outer = max_r_outer;
    if (r_outer > 32) r_outer = 32;

    // Geometría Interior (donde encaja el fondo)
    int16_t r_inner = r_outer - borderWidth;
    if (r_inner < 0) r_inner = 0; // Si el borde es más grueso que el radio, la esquina interior es cuadrada.

    // Límites para el área de relleno interior
    int16_t innerXStart = x + borderWidth;
    int16_t innerYEnd = x + w - borderWidth;
    int16_t innerH = h - (borderWidth * 2);

    // Pre-calcular curvaturas para ambos radios
    int16_t emptyPixelsOuter[32] = {0};
    int16_t emptyPixelsInner[32] = {0};
    gfx_getCornerEmptyPixels(emptyPixelsOuter, r_outer);
    gfx_getCornerEmptyPixels(emptyPixelsInner, r_inner);

    // Renderizar fila por fila
	int16_t row = 0;
    for (; row < h; row++) {
        
        // Encontrar contorno exterior para esta fila
        int16_t emptyOuterX = 0;
        if (row < r_outer) {
            emptyOuterX = emptyPixelsOuter[row];
        } else if (row >= h - r_outer) {
            emptyOuterX = emptyPixelsOuter[h - 1 - row];
        }
        int16_t drawStartX_outer = x + emptyOuterX;
        int16_t drawEndX_outer = x + w - emptyOuterX;
        int16_t drawW_outer = drawEndX_outer - drawStartX_outer;

        // ¿Esta fila es puramente borde sólido (Top/Bottom)?
        if (row < borderWidth || row >= (h - borderWidth)) {
            // Dibujar la barra exterior completa
            gfx_fillRect(pBuf, drawStartX_outer, y + row, drawW_outer, 1, color);
        } else {
            // Esta fila pasa por la sección central (con "agujero" para el fondo).
            // Dibujar dos segmentos (borde izquierdo y derecho).
            
            int16_t innerRow = row - borderWidth; // Normalizar a coordenada y del rectángulo interior

            // Encontrar lógica de contorno interior (donde empieza el fondo)
            int16_t emptyInnerX = 0;
            if (innerRow < r_inner) {
                emptyInnerX = emptyPixelsInner[innerRow];
            } else if (innerRow >= (innerH - r_inner)) {
                // Ajustar para la curva inferior del *rectángulo interior*
                emptyInnerX = emptyPixelsInner[innerH - 1 - innerRow];
            }

            int16_t startX_inner = innerXStart + emptyInnerX;
            int16_t endX_inner = x + w - borderWidth - emptyInnerX;

            // Dibujar Segmento Izquierdo
            gfx_fillRect(pBuf, drawStartX_outer, y + row, startX_inner - drawStartX_outer, 1, color);

            // Dibujar Segmento Derecho
            gfx_fillRect(pBuf, endX_inner, y + row, drawEndX_outer - endX_inner, 1, color);
        }
    }
}

void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn) {
    if (!pBuf || !btn) return;

    // 1. Definir colores base
    uint16_t activeBorderColor = btn->borderColor;
    uint16_t activeTextColor = btn->textColor;
    uint16_t topColor = btn->backgroundColor;
    uint16_t bottomColor = DARKEN_COLOR(btn->backgroundColor); 

    // 2. Determinar el desplazamiento cinemático según el estado
    int16_t offset = 0;
    if (btn->state == BTN_STATE_PRESSED) {
        offset = 2; // Todo el botón (borde, fondo y texto) bajará 2 píxeles
        
        // Invertir el gradiente para simular hundimiento físico
        topColor = DARKEN_COLOR(btn->backgroundColor); 
        bottomColor = btn->backgroundColor; 
        activeBorderColor = DARKEN_COLOR(btn->borderColor);
        
    } else if (btn->state == BTN_STATE_DISABLED) {
        topColor = 0x4208;    // Gris mate claro
        bottomColor = 0x2104; // Gris mate oscuro
        activeBorderColor = 0x8410;
        activeTextColor = 0x8410;
    }

    // 3. Calcular la huella del Borde APLICANDO el offset cinemático
    int16_t footprintX = btn->pos.x - btn->borderWidth + offset;
    int16_t footprintY = btn->pos.y - btn->borderWidth + offset;
    int16_t footprintW = btn->size.width + (btn->borderWidth * 2);
    int16_t footprintH = btn->size.height + (btn->borderWidth * 2);

    // Unificar Radios para geometría concéntrica perfecta
    int16_t r_outer = btn->radius + btn->borderWidth;

    // 4. Renderizar Borde (Chasis unificado)
    if (btn->borderWidth > 0) {
        gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, 
                             r_outer, btn->borderWidth, activeBorderColor);
    }

    // 5. Renderizar Fondo (Con gradiente lineal vertical)
    gfx_fillGradientRoundRect(pBuf, 
                              btn->pos.x + offset, 
                              btn->pos.y + offset, 
                              btn->size.width, 
                              btn->size.height, 
                              btn->radius, topColor, bottomColor);

    // 6. Renderizar Etiqueta (Centrada y desplazada)
    if (btn->label != NULL) {
        gfx_DrawString(pBuf, btn->font, 
                       btn->pos.x + offset + (btn->size.width / 2.0f),
                       btn->pos.y + offset + (btn->size.height / 2.0f), 
                       btn->label, activeTextColor, btn->fontScale, ALIGN_CENTER);
    }
}

/*
void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn) {
    if (!pBuf || !btn) return;

    // Calcular la huella total del objeto incluyendo borde
    int16_t footprintX = btn->pos.x - btn->borderWidth;
    int16_t footprintY = btn->pos.y - btn->borderWidth;
    int16_t footprintW = btn->size.width + (btn->borderWidth * 2);
    int16_t footprintH = btn->size.height + (btn->borderWidth * 2);

    // Colores base
    uint16_t activeBorderColor = btn->borderColor;
    uint16_t activeTextColor = btn->textColor;
    uint16_t topColor = btn->backgroundColor;
    uint16_t bottomColor = DARKEN_COLOR(btn->backgroundColor); 

    int16_t offset = 0;

    // Manejo de estado cinemático
    if (btn->state == BTN_STATE_PRESSED) {
        offset = 2; // Cinemática
        topColor = DARKEN_COLOR(btn->backgroundColor); // Invertir gradiente
        bottomColor = btn->backgroundColor;
        activeBorderColor = DARKEN_COLOR(btn->borderColor);
    } else if (btn->state == BTN_STATE_DISABLED) {
        topColor = 0x4208;
        bottomColor = 0x2104;
        activeBorderColor = 0x8410;
        activeTextColor = 0x8410;
    }

    // Unificar Radios
    int16_t r_outer = btn->radius + btn->borderWidth;

    // 1. Dibujar el Borde Unificado (Chasis estático)
    // Usamos 'r_outer' y 'btn->borderWidth' para la matemática concéntrica
    gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, r_outer, btn->borderWidth, activeBorderColor);

    // 2. Dibujar el Fondo con Gradiente (Centro móvil)
    // Usa los valores puros 'btn->pos' y 'btn->radius' que definen el interior
    // y que son geométricamente concéntricos con el marco interior de gfx_drawRoundOutline.
    gfx_fillGradientRoundRect(pBuf, 
                              btn->pos.x + offset, 
                              btn->pos.y + offset, 
                              btn->size.width, 
                              btn->size.height, 
                              btn->radius, topColor, bottomColor);

    // 3. Dibujar la Etiqueta (Centro móvil)
    gfx_DrawString(pBuf, btn->font, 
                   btn->pos.x + offset + (btn->size.width / 2.0f),
                   btn->pos.y + offset + (btn->size.height / 2.0f), 
                   btn->label, activeTextColor, btn->fontScale, ALIGN_CENTER);
}*/

void gfx_drawLabel(pixel16_t *pBuf, gfx_Label *lb)
{
  	gfx_DrawString(pBuf, lb->font, lb->pos.x, lb->pos.y, lb->text, lb->textColor, lb->scale, lb->alignment);
}

void gfx_drawRectangle(pixel16_t *pBuf, gfx_Rectangle *rect)
{
	gfx_fillRect(pBuf, rect->pos.x, rect->pos.y, rect->dim.width, rect->dim.height, rect->color);
}
