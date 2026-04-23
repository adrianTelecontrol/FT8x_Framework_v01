/**
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils/uartstdio.h"

#include "EVE.h"
#include "EVE_colors.h"
#include "FT8xx_params.h"
#include "font_engine.h"
#include "graphics_engine.h"
#include "gfx_theme.h"
#include "helpers.h"
#include "gfx_colors.h"

#include "gfx.h"

const char *TASK_NAME = "gfx";

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
  } else if (type == WD_TYPE_SLIDER) {
    gfx_Slider *wd = (gfx_Slider *)widget;
    
    wd->regTouch.x1 = wd->pos.x;
    wd->regTouch.x2 = wd->pos.x + wd->size.width;
    wd->regTouch.y1 = wd->pos.y - wd->knobRadius;
    wd->regTouch.y2 = wd->pos.y + wd->size.height + wd->knobRadius;
  }

  return true;
}

void gfx_calibrate(void) {
  API_LIB_BeginCoProList(); // Begin new screen
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0); // Clear screen
  API_CLEAR(1, 1, 1);
  API_CMD_TEXT(LCD_WIDTH / 2, LCD_HEIGHT / 2, 30, OPT_CENTER, "Calibracion de pantalla. Presione los puntos.");
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
  
  uint32_t i = 0;
  for(; i < LCD_WIDTH * LCD_HEIGHT; i++)
  {
	psPixelBuffer[i].u16 = g_pCurrentTheme->palette.background;	
  }
  
  uint32_t j = i;
  j++;
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
	case WD_TYPE_SLIDER:
      gfx_drawSlider(psPixelBuffer, (gfx_Slider *)iter->sWidget.pvWidget);	
      break;
	case WD_TYPE_GRAPH:
      gfx_drawGraph(psPixelBuffer, (gfx_Graph *)iter->sWidget.pvWidget);	
      break;
	case WD_TYPE_MULTIGRAPH:
      gfx_drawMultiGraph(psPixelBuffer, (gfx_MultiGraph *)iter->sWidget.pvWidget);	
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
  case WD_TYPE_SLIDER:
    ret = gfx_touchObject(((gfx_Slider *)wd->pvWidget)->regTouch, touch);
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
    if (!pBuf || !btn || !g_pCurrentTheme) return;

    // 1. Resolve colors semantically from the active theme
    uint16_t baseBgColor;
    uint16_t baseTextColor = g_pCurrentTheme->palette.textMain;
    uint16_t baseBorderColor = g_pCurrentTheme->palette.border;

    switch (btn->style) {
        case STYLE_PRIMARY:
            baseBgColor = g_pCurrentTheme->palette.primary;
            break;
        case STYLE_DANGER:
            baseBgColor = g_pCurrentTheme->palette.danger;
            break;
        case STYLE_DEFAULT:
        default:
            baseBgColor = g_pCurrentTheme->palette.surface;
            break;
    }

    // 2. Apply Physical State Modifiers (Cinematic shift & darken)
    uint16_t topColor = baseBgColor;
    uint16_t bottomColor = DARKEN_COLOR(baseBgColor);
    int16_t offset = 0;

    if (btn->state == BTN_STATE_PRESSED) {
        offset = 2;
        topColor = DARKEN_COLOR(baseBgColor);
        bottomColor = baseBgColor;
        baseBorderColor = DARKEN_COLOR(baseBorderColor);
    } else if (btn->state == BTN_STATE_DISABLED) {
        topColor = 0x4208;
        bottomColor = 0x2104;
        baseBorderColor = 0x8410;
        baseTextColor = g_pCurrentTheme->palette.textMuted; // Use theme's muted color
    }

    // 3. Calculate footprints and render the chassis
    int16_t footprintX = btn->pos.x - btn->borderWidth + offset;
    int16_t footprintY = btn->pos.y - btn->borderWidth + offset;
    int16_t footprintW = btn->size.width + (btn->borderWidth * 2);
    int16_t footprintH = btn->size.height + (btn->borderWidth * 2);
    int16_t r_outer = btn->radius + btn->borderWidth;

    if (btn->borderWidth > 0) {
        gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, 
                             r_outer, btn->borderWidth, baseBorderColor);
    }

    gfx_fillGradientRoundRect(pBuf, 
                              btn->pos.x + offset, 
                              btn->pos.y + offset, 
                              btn->size.width, 
                              btn->size.height, 
                              btn->radius, topColor, bottomColor);

    // 4. Render the localized Font using the global Theme color
    if (btn->label != NULL) {
        int8_t fontId = -1;
        
        // Resolver el ID de la fuente según la jerarquía solicitada
        switch (btn->typo) {
            case TYPO_H1:      		fontId = g_pCurrentTheme->fonts.h1; break;
            case TYPO_H2:      		fontId = g_pCurrentTheme->fonts.h2; break;
            case TYPO_BODY:    		fontId = g_pCurrentTheme->fonts.body; break;
            case TYPO_CAPTION: 		fontId = g_pCurrentTheme->fonts.caption; break;
            case TYPO_MONO:    		fontId = g_pCurrentTheme->fonts.mono; break;
			case TYPO_MONO_BOLD:	fontId = g_pCurrentTheme->fonts.mono_bold; break; 
        }
        
        // Si el motor SD logró cargar la fuente, la dibujamos
        if (fontId >= 0) {
            gfx_DrawString(pBuf, fontId, 
                           btn->pos.x + offset + (btn->size.width / 2),
                           btn->pos.y + offset + (btn->size.height / 2), 
                           btn->label, baseTextColor, ALIGN_CENTER, 1);
        }
    }
}

void onGenericBtnPressed(gfx_Button *btn) {
    btn->state = BTN_STATE_PRESSED;
    btn->bIsDirty = true;
}

void onGenericBtnRelease(gfx_Button *btn) {
    btn->state = BTN_STATE_NORMAL;
    btn->bIsDirty = true;
}

void gfx_drawLabel(pixel16_t *pBuf, gfx_Label *lb) {
    if (!pBuf || !lb || !lb->text || !g_pCurrentTheme || !lb->isVisible) return;

    // 1. Resolve Semantic Color from the Theme Palette
    // Labels default to textMain, but can be overridden (e.g., a Red DANGER label)
    uint16_t activeColor = g_pCurrentTheme->palette.textMain;
    
    switch (lb->style) {
        case STYLE_PRIMARY:
            activeColor = g_pCurrentTheme->palette.primary;
            break;
        case STYLE_DANGER:
            activeColor = g_pCurrentTheme->palette.danger;
            break;
        case STYLE_SUCCESS:
            activeColor = g_pCurrentTheme->palette.success;
            break;
		case STYLE_TEXT_MAIN:
			activeColor = g_pCurrentTheme->palette.textMain;
			break;
		case STYLE_TEXT_MUTED:
			activeColor = g_pCurrentTheme->palette.textMuted;
			break;
        case STYLE_SECONDARY:
            // Great for subtext/captions that shouldn't distract the user
            activeColor = g_pCurrentTheme->palette.secondary; 
            break;
        case STYLE_DEFAULT:
        default:
            activeColor = g_pCurrentTheme->palette.textMain;
            break;
    }

    // 2. Resolve Semantic Font ID from the Theme Typography
    int8_t fontId = -1;
    switch (lb->typo) {
        case TYPO_H1:      fontId = g_pCurrentTheme->fonts.h1; break;
        case TYPO_H2:      fontId = g_pCurrentTheme->fonts.h2; break;
        case TYPO_BODY:    fontId = g_pCurrentTheme->fonts.body; break;
        case TYPO_CAPTION: fontId = g_pCurrentTheme->fonts.caption; break;
        case TYPO_MONO:    fontId = g_pCurrentTheme->fonts.mono; break;
        case TYPO_MONO_BOLD:    fontId = g_pCurrentTheme->fonts.mono_bold; break;
    }

    // 3. Render the string if the font was successfully loaded from the SD card
    if (fontId >= 0) {
        gfx_DrawString(pBuf, fontId, lb->pos.x, lb->pos.y, lb->text, activeColor, lb->alignment, 1);
    }
}

void gfx_drawRectangle(pixel16_t *pBuf, gfx_Rectangle *rect)
{
	//gfx_fillRect(pBuf, rect->pos.x, rect->pos.y, rect->dim.width, rect->dim.height, rect->color);
	gfx_fillRoundRect(pBuf, rect->pos.x, rect->pos.y, rect->dim.width, rect->dim.height, rect->round, rect->color);

    if (rect->borderWidth > 0) {
    	int16_t footprintX = rect->pos.x - rect->borderWidth;
    	int16_t footprintY = rect->pos.y - rect->borderWidth;
    	int16_t footprintW = rect->dim.width + (rect->borderWidth * 2);
    	int16_t footprintH = rect->dim.height + (rect->borderWidth * 2);
    	int16_t r_outer = rect->round + rect->borderWidth;
        gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, 
                             r_outer, rect->borderWidth, LIGHTEN_COLOR(rect->color));
    }
}

void gfx_drawSlider(pixel16_t *pBuf, gfx_Slider *sl) {
    if (!pBuf || !sl || !g_pCurrentTheme) return;

    // 1. Resolver colores base del tema
    uint16_t trackBgColorTop = g_pCurrentTheme->palette.secondary;      
    uint16_t trackBgColorBot = DARKEN_COLOR(trackBgColorTop); 

    uint16_t trackActiveColorTop = g_pCurrentTheme->palette.success;  
    uint16_t trackActiveColorBot = DARKEN_COLOR(trackActiveColorTop); 

    uint16_t knobColorTop = g_pCurrentTheme->palette.primary;         
    uint16_t knobColorBot = DARKEN_COLOR(knobColorTop);

    // 2. Proteger límites matemáticos
    if (sl->currentValue < sl->minValue) sl->currentValue = sl->minValue;
    if (sl->currentValue > sl->maxValue) sl->currentValue = sl->maxValue;

    // 3. Abstracción de Orientación (Horizontal vs Vertical)
    uint16_t thickness = sl->bIsVertical ? sl->size.width : sl->size.height;
    uint16_t length    = sl->bIsVertical ? sl->size.height : sl->size.width;

    // Geometría Dinámica
    uint16_t dynamicKnobRadius = thickness * 1.25;
    sl->knobRadius = dynamicKnobRadius;

    int16_t pixelRange = sl->bShowKnob ? length - (1.5 * dynamicKnobRadius) : length - dynamicKnobRadius;
    int16_t valueRange = sl->maxValue - sl->minValue;
    if (valueRange == 0) valueRange = 1;

    int16_t activeOffset = ((sl->currentValue - sl->minValue) * pixelRange) / valueRange;

    // Variables finales de renderizado
    int16_t knobCenterX, knobCenterY;
    int16_t activeTrackX, activeTrackY, activeTrackW, activeTrackH;

    if (sl->bIsVertical) {
        // --- LOGICA VERTICAL ---
        knobCenterX = sl->pos.x + (thickness / 2);
        // Empieza en la parte inferior (pos.y + length) y sube (- activeOffset)
        knobCenterY = (sl->pos.y + length) - (1.0 * dynamicKnobRadius) - activeOffset;

        if (sl->currentValue == 0 && sl->bShowKnob) {
            knobCenterY = (sl->pos.y + length) - (0.5 * dynamicKnobRadius);
        }

        activeTrackX = sl->pos.x;
        activeTrackY = knobCenterY; // Dibuja desde el knob hacia abajo
        activeTrackW = thickness;
        activeTrackH = (sl->pos.y + length) - knobCenterY;
        
    } else {
        // --- LOGICA HORIZONTAL ---
        knobCenterX = sl->pos.x + (1.0 * dynamicKnobRadius) + activeOffset;
        knobCenterY = sl->pos.y + (thickness / 2);

        if (sl->currentValue == 0 && sl->bShowKnob) {
            knobCenterX = sl->pos.x + (0.5 * dynamicKnobRadius);
        }

        activeTrackX = sl->pos.x;
        activeTrackY = sl->pos.y;
        activeTrackW = knobCenterX - sl->pos.x;
        activeTrackH = thickness;
    }

    // =========================================================
    // DIBUJADO CON GRADIENTES
    // =========================================================
    
    // 1. Track de Fondo
    gfx_fillGradientRoundRect(pBuf, 
                              sl->pos.x, sl->pos.y, 
                              sl->size.width, sl->size.height, 
                              thickness, 
                              trackBgColorBot, trackBgColorTop); 

    // 2. Track Activo
    if (sl->currentValue > sl->minValue || (sl->currentValue == 0 && !sl->bShowKnob)) { 
        gfx_fillGradientRoundRect(pBuf, 
                                  activeTrackX, activeTrackY, 
                                  activeTrackW, activeTrackH, 
                                  thickness, 
                                  trackActiveColorTop, trackActiveColorBot);
    }

    // 3. Dibujar la Perilla (Knob)
    if(sl->bShowKnob) {
        gfx_fillCircle(pBuf, knobCenterX, knobCenterY, dynamicKnobRadius / 2, trackBgColorBot);
        gfx_fillCircle(pBuf, knobCenterX, knobCenterY, (dynamicKnobRadius / 2) - 2, knobColorTop);
    }

    // Limpiar bandera
    sl->bIsDirty = false;
}

bool gfx_processSliderTouch(gfx_Slider *sl, TouchStatus touch) {
    if (gfx_touchObject(sl->regTouch, touch)) {
        
        uint16_t thickness = sl->bIsVertical ? sl->size.width : sl->size.height;
        uint16_t dynamicKnobRadius = thickness * 1.25;
        if (dynamicKnobRadius < 2) dynamicKnobRadius = 2;

        int16_t minLimit, maxLimit, touchAxis;
        int16_t newValue = 0;

        if (sl->bIsVertical) {
            // Evaluamos el eje Y (invertido, porque Y=0 es la parte superior)
            minLimit = sl->pos.y + dynamicKnobRadius;
            maxLimit = sl->pos.y + sl->size.height - dynamicKnobRadius;
            touchAxis = touch.y;
            
            if (touchAxis < minLimit) touchAxis = minLimit;
            if (touchAxis > maxLimit) touchAxis = maxLimit;
            
            int16_t pixelRange = maxLimit - minLimit;
            int16_t valueRange = sl->maxValue - sl->minValue;
            
            // Invertimos la matemática: Tocar arriba = MaxValue
            newValue = sl->maxValue - (((touchAxis - minLimit) * valueRange) / pixelRange);
            
        } else {
            // Evaluamos el eje X (como lo tenías antes)
            minLimit = sl->pos.x + dynamicKnobRadius;
            maxLimit = sl->pos.x + sl->size.width - dynamicKnobRadius;
            touchAxis = touch.x;
            
            if (touchAxis < minLimit) touchAxis = minLimit;
            if (touchAxis > maxLimit) touchAxis = maxLimit;
            
            int16_t pixelRange = maxLimit - minLimit;
            int16_t valueRange = sl->maxValue - sl->minValue;
            
            newValue = sl->minValue + (((touchAxis - minLimit) * valueRange) / pixelRange);
        }

        if (newValue != sl->currentValue) {
            sl->currentValue = newValue;
            sl->bIsDirty = true;
            if (sl->onValueChanged != NULL) {
                sl->onValueChanged(sl, sl->currentValue);
            }
            return true; 
        }
    }
    return false;
}

/*
bool gfx_processSliderTouch(gfx_Slider *sl, TouchStatus touch) {
    // Verificar si el toque ocurrió dentro de la RegionTouchObject del slider
        
        // Determinar límites físicos de arrastre
        int16_t minX = sl->pos.x + sl->knobRadius;
        int16_t maxX = sl->pos.x + sl->size.width - sl->knobRadius;
        int16_t touchX = touch.x;

        // Restringir el toque a los bordes
        if (touchX < minX) touchX = minX;
        if (touchX > maxX) touchX = maxX;

        // Calcular nuevo valor
        int16_t pixelRange = maxX - minX;
        int16_t valueRange = sl->maxValue - sl->minValue;
        
        int16_t newValue = sl->minValue + (((touchX - minX) * valueRange) / pixelRange);

		TIVA_LOGI(TASK_NAME, "Slider new value: %u", newValue);

        sl->bIsDirty = true;

        // Si el valor cambió, actualizar y disparar callback
        if (newValue != sl->currentValue) {
            sl->currentValue = newValue;

            
            if (sl->onValueChanged != NULL) {
                sl->onValueChanged(sl, sl->currentValue);
            }
            return true; // El evento fue consumido
        }
	return false;
}*/
/*
void gfx_drawGraph(pixel16_t *pBuf, gfx_Graph *graph) {
    if (!pBuf || !graph) return;

    // 1. Dibujar fondo de la gráfica con el color personalizado
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, graph->bgColor);

    // =========================================================
    // 2. Dibujar Cuadrícula (Uso de funciones FAST optimizadas)
    // =========================================================
    
    // Líneas Horizontales (Eje Y)
    if (graph->gridLinesY > 0) {
        int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
		int i = 1;
        for (; i <= graph->gridLinesY; i++) {
            int16_t gy = graph->pos.y + (i * stepY);
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }
    }
    
    // Líneas Verticales (Eje X)
    if (graph->gridLinesX > 0) {
        int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
		int i = 1;
        for (; i <= graph->gridLinesX; i++) {
            int16_t gx = graph->pos.x + (i * stepX);
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }
    }

	// =========================================================
    // 3. Trazar los Datos (Grosor y Color Dinámicos)
    // =========================================================
    if (graph->data != NULL && graph->maxPoints > 1) {
        int16_t rangeY = graph->maxY - graph->minY;
        if (rangeY <= 0) rangeY = 1; // Prevenir división por cero

        int16_t prevX = -1, prevY = -1;

        // Calculamos el offset para centrar el grosor de la línea
        int8_t widthOffset = -(graph->lineWidth / 2);

		uint16_t i = 0;
        for (; i < graph->maxPoints; i++) {
            uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
            int16_t val = graph->data[dataIdx];

            // Clamping 
            if (val < graph->minY) val = graph->minY;
            if (val > graph->maxY) val = graph->maxY;

            // EL FIX: Forzar la matemática a 32 bits para evitar desbordamiento
            int16_t px = graph->pos.x + (int16_t)(((int32_t)i * graph->size.width) / (graph->maxPoints - 1));
            int16_t py = graph->pos.y + graph->size.height - (int16_t)(((int32_t)(val - graph->minY) * graph->size.height) / rangeY);

            // Conectar con el punto anterior
            if (prevX != -1) {
                // Loop para engrosar la línea iterando sobre el eje Y
				uint8_t w = 0;
                for (; w < graph->lineWidth; w++) {
                    int8_t currentOffset = widthOffset + w;
                    
                    int16_t adjPrevY = prevY + currentOffset;
                    int16_t adjPy = py + currentOffset;
                    
                    gfx_writeLine(pBuf, prevX, adjPrevY, px, adjPy, graph->lineColor);
                }
            }
            prevX = px;
            prevY = py;
        }
    }

    graph->bIsDirty = false;
} */

void gfx_drawGraph(pixel16_t *pBuf, gfx_Graph *graph) {
    if (!pBuf || !graph) return;

    // 1. Dibujar fondo de la gráfica
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, g_pCurrentTheme->palette.surface);

	// =========================================================
    // 2. Dibujar Cuadrícula y Etiquetas (Y-Axis)
    // =========================================================
    
    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    if (graph->bShowLabels) {
        fontId = gfx_ResolveFontId(graph->typo); // Using the helper we made earlier!
        if (fontId >= 0) {
            // Get the height of a standard number to center it vertically
            gfx_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // Líneas Horizontales (Eje Y) y Textos
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    int16_t valueStep = (graph->maxY - graph->minY) / (graph->gridLinesY + 1);

    // Iteramos desde 0 hasta gridLinesY + 1 para incluir el techo (Max) y el piso (Min)
	int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        
        int16_t gy = graph->pos.y + (i * stepY);
        int16_t gridValue = graph->maxY - (i * valueStep); // Y is inverted, so top is Max

        // 1. Draw the Grid Line (Skip i=0 and the last i, as they are the borders of the widget)
        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        // 2. Draw the Numeric Label
        if (graph->bShowLabels && fontId >= 0) {
            char valStr[12];
            snprintf(valStr, sizeof(valStr), "%d", gridValue); // Format integer to string
            
            // Draw 4 pixels from the left edge, centered vertically on the grid line
            int16_t textX = graph->pos.x + 5;
            int16_t textY = gy + (textH / 3);
            
            // Prevent the top and bottom text from bleeding outside the graph limits
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            // !! IMPORTANT !!
            // Replace 'gfx_DrawString' with whatever your primitive text drawing function 
            // is called in your engine (the one that writes directly to pBuf).
            // gfx_DrawString(pBuf, valStr, textX, textY, fontId, graph->textColor);
            gfx_DrawString(pBuf, fontId, textX, textY, valStr, graph->textColor, ALIGN_LEFT, 1);
        }
    }

    // =========================================================
    // 2. Dibujar Cuadrícula
    // =========================================================
    if (graph->gridLinesY > 0) {
        int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
		int i = 1;
        for (; i <= graph->gridLinesY; i++) {
            int16_t gy = graph->pos.y + (i * stepY);
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }
    }
    
    if (graph->gridLinesX > 0) {
        int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
		int i = 1;
        for (; i <= graph->gridLinesX; i++) {
            int16_t gx = graph->pos.x + (i * stepX);
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }
    }

    // =========================================================
    // 3. Trazar los Datos (Grosor Inteligente)
    // =========================================================
	#if 0
    if (graph->data != NULL && graph->maxPoints > 1) {
        int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
		if (rangeY <= 0) rangeY = 1;

        int16_t prevX = -1, prevY = -1;
        int8_t widthOffset = -(graph->lineWidth / 2);
		uint16_t i = 0;
        for (; i < graph->maxPoints; i++) {
            uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
            int16_t val = graph->data[dataIdx];

            // Clamping 
            // Clamping 
            if (val < graph->minY) val = graph->minY;
            if (val > graph->maxY) val = graph->maxY;

            // EL FIX: Invertir el mapeo del eje X
            // Calculamos un índice inverso para que el dato más viejo (i=0) 
            // se dibuje a la derecha, y el más nuevo se dibuje a la izquierda.
            uint16_t reverse_i = (graph->maxPoints - 1) - i;

            int16_t px = (int16_t)(
                (int32_t)graph->pos.x + 
                ((int32_t)reverse_i * (int32_t)(graph->size.width - 1)) / 
                (int32_t)(graph->maxPoints - 1)
            );
            
            int16_t py = (int16_t)(
                (int32_t)graph->pos.y + (int32_t)(graph->size.height - 1) -
                (((int32_t)(val - graph->minY) * (int32_t)(graph->size.height - 1)) / rangeY)
            );
            // Conectar con el punto anterior
            if (prevX != -1) {
                
                // EL FIX: Calcular si la línea es más vertical que horizontal
                bool isSteep = abs(py - prevY) > abs(px - prevX);
                uint8_t w = 0;
                for (; w < graph->lineWidth; w++) {
                    int8_t currentOffset = widthOffset + w;
                    
                    if (isSteep) {
                        // Si la caída es vertical, desplazamos la línea hacia los LADOS (Eje X)
                        gfx_writeLine(pBuf, prevX + currentOffset, prevY, px + currentOffset, py, graph->lineColor);
                    } else {
                        // Si la línea es horizontal, la desplazamos hacia ARRIBA/ABAJO (Eje Y)
                        gfx_writeLine(pBuf, prevX, prevY + currentOffset, px, py + currentOffset, graph->lineColor);
                    }
                }
            }
            prevX = px;
            prevY = py;
        }
    }
	#endif

    graph->bIsDirty = false;
}

void UpdateDisplayWithGraphOverlay(gfx_Graph *graph, gfx_Graph *graph2) {
  // 1. Iniciamos una nueva Display List
  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0);
  API_CLEAR(1, 1, 1);
  API_COLOR_RGB(255, 255, 255);

  // =======================================================
  // CAPA 1: TU BITMAP DE SOFTWARE (Exactamente como lo tenías)
  // =======================================================
  uint16_t ui16Width = 800;
  uint16_t ui16Height = 480;
  API_BITMAP_HANDLE(0);
  API_BITMAP_SOURCE(RAM_G); // La base de tu framebuffer
  
  uint16_t BytesPerPixel = 2;
  uint16_t ui16Stride = ui16Width * BytesPerPixel;
  API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
  API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);
  API_BITMAP_SIZE(NEAREST, BORDER, BORDER, ui16Width, ui16Height);
  API_BITMAP_SIZE_H(ui16Width >> 9, ui16Height >> 9);

  API_BEGIN(BITMAPS);
  API_VERTEX2II(0, 0, 0, 0); // Dibuja todo el fondo
  API_END(); // Cerramos el dibujo de bitmaps

  // =======================================================
  // CAPA 2: GRÁFICA 1
  // =======================================================
  if (graph != NULL && graph->data != NULL && graph->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
      
      uint32_t color = 0x22AAAA; 
      API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);

	  uint16_t i = 0;
      for (; i < graph->maxPoints; i++) {
          uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
          int16_t val = graph->data[dataIdx];

          if (val < graph->minY) val = graph->minY;
          if (val > graph->maxY) val = graph->maxY;

          uint16_t reverse_i = (graph->maxPoints - 1) - i;
          int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)reverse_i * (graph->size.width - 1)) / (graph->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
      
      // Vaciamos el FIFO de comandos (RAM_CMD) hacia la RAM_DL 
  }
  
  API_LIB_EndCoProList(); 
  
  API_LIB_AwaitCoProEmpty();

  // 3. Volvemos a abrir la ráfaga SPI para continuar inyectando comandos
  API_LIB_BeginCoProList();
  // =======================================================
  // CAPA 3: GRÁFICA 2
  // =======================================================
  if (graph2 != NULL && graph2->data != NULL && graph2->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph2->maxY - (int32_t)graph2->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph2->lineWidth * 16 / 2); 
      
      uint32_t color = 0xAA2222; 
      API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);

	  uint16_t i = 0;
      for (; i < graph2->maxPoints; i++) {
          uint16_t dataIdx = (graph2->head + i) % graph2->maxPoints;
          int16_t val = graph2->data[dataIdx];

          if (val < graph2->minY) val = graph2->minY;
          if (val > graph2->maxY) val = graph2->maxY;

          uint16_t reverse_i = (graph2->maxPoints - 1) - i;
          int16_t px = (int16_t)((int32_t)graph2->pos.x + ((int32_t)reverse_i * (graph2->size.width - 1)) / (graph2->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph2->pos.y + (graph2->size.height - 1) - (((int32_t)(val - graph2->minY) * (graph2->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
  }

  // 3. Cerramos y hacemos el SWAP en el hardware
  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();

  // Esperamos a que todo el frame (SWAP incluido) sea procesado
  API_LIB_AwaitCoProEmpty();
}

void gfx_GraphAddPoint(gfx_Graph *graph, int16_t newValue) {
    if(!graph || !graph->data) return;

    // Sobrescribimos el dato más viejo en la posición 'head'
    graph->data[graph->head] = newValue;

    // Avanzamos el 'head' circularmente
    graph->head = (graph->head + 1) % graph->maxPoints;

	//UpdateDisplayWithGraphOverlay(graph);
    // Le avisamos al motor que debe redibujar la gráfica en el próximo frame
    //graph->bIsDirty = true;
	graph->bEVEDirty = true;
}

void gfx_GraphRenderEVEComponents(gfx_Graph *graph) {
  if (graph != NULL && graph->data != NULL && graph->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
      
      uint32_t color = 0x22AAAA; 
      API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);

	  uint16_t i = 0;
      for (; i < graph->maxPoints; i++) {
          uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
          int16_t val = graph->data[dataIdx];

          if (val < graph->minY) val = graph->minY;
          if (val > graph->maxY) val = graph->maxY;

          uint16_t reverse_i = (graph->maxPoints - 1) - i;
          int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)reverse_i * (graph->size.width - 1)) / (graph->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
      // Vaciamos el FIFO de comandos (RAM_CMD) hacia la RAM_DL 
  }
}

void gfx_MultiGraphAddData(gfx_MultiGraph *graph, uint8_t traceIndex, int16_t newValue) {
    // Validaciones de seguridad
    if (!graph || graph->maxPoints == 0) return;
    if (traceIndex >= graph->activeTraces || graph->dataSets[traceIndex] == NULL) return;

    // 1. Obtenemos la cabecera actual específica de este trazo
    uint16_t currentHead = graph->heads[traceIndex];

    // 2. Inyectamos el nuevo valor
    graph->dataSets[traceIndex][currentHead] = newValue;

    // 3. Avanzamos ÚNICAMENTE la cabecera de este trazo
    graph->heads[traceIndex] = (currentHead + 1) % graph->maxPoints;

	graph->bEVEDirty = true;
}

void gfx_drawMultiGraph(pixel16_t *pBuf, gfx_MultiGraph *graph) {
    if (!pBuf || !graph) return;

    // 1. Draw Background
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, graph->bgColor);

    // =========================================================
    // 2. Draw Grid and Labels
    // =========================================================
    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    if (graph->bShowLabels) {
        fontId = gfx_ResolveFontId(graph->typo); 
        if (fontId >= 0) {
            gfx_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // Y-Axis Horizontal Lines and Texts
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    int16_t valueStep = (graph->maxY - graph->minY) / (graph->gridLinesY + 1);

    int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        int16_t gy = graph->pos.y + (i * stepY);
        int16_t gridValue = graph->maxY - (i * valueStep); 

        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        if (graph->bShowLabels && fontId >= 0) {
            char valStr[12];
            snprintf(valStr, sizeof(valStr), "%d", gridValue); 
            
            int16_t textX = graph->pos.x + 5;
            int16_t textY = gy + (textH / 3);
            
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            // Use your engine's text drawing function
            gfx_DrawString(pBuf, fontId, textX, textY, valStr, graph->textColor, ALIGN_LEFT, 1);
        }
    }

    // X-Axis Vertical Lines
    if (graph->gridLinesX > 0) {
        int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
		int i = 1;
        for (; i <= graph->gridLinesX; i++) {
            int16_t gx = graph->pos.x + (i * stepX);
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }
    }

    graph->bIsDirty = false;
}

void gfx_MultigraphRenderEVEComponents(gfx_MultiGraph *graph) {

    if (graph != NULL && graph->activeTraces > 0 && graph->maxPoints > 1) {
        int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
        if (rangeY <= 0) rangeY = 1;

        // Downsampling (Decimación) to prevent SPI/FIFO overload
        uint16_t renderPoints = graph->size.width; 
        if (renderPoints > graph->maxPoints) {
            renderPoints = graph->maxPoints;
        }

        if (renderPoints > 1) {
            
            // Loop through all active traces dynamically
			uint8_t trace = 0;
            for (; trace < graph->activeTraces; trace++) {
                
                // Skip if data array is not initialized
                if (graph->dataSets[trace] == NULL) continue;

                API_BEGIN(LINE_STRIP); 
                API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
                
                // Extract RGB from the specific trace's color (Assuming RGB565 to RGB888 conversion)
                uint16_t c = graph->lineColors[trace];
                API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));
				uint16_t i = 0;
				uint16_t currentTraceHead = graph->heads[trace];
                for (; i < renderPoints; i++) {
                    uint16_t dataOffset = (i * (graph->maxPoints - 1)) / (renderPoints - 1);
                    // EL CAMBIO: Usamos la cabecera individual de la señal actual
                    uint16_t dataIdx = (currentTraceHead + dataOffset) % graph->maxPoints;
                    
                    int16_t val = graph->dataSets[trace][dataIdx];

                    if (val < graph->minY) val = graph->minY;
                    if (val > graph->maxY) val = graph->maxY;

                    uint16_t reverse_i = (renderPoints - 1) - i;
                    int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)reverse_i * (graph->size.width - 1)) / (renderPoints - 1));
                    int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

                    API_VERTEX2F(px * 16, py * 16);
                }
                
                API_END(); // End the current trace

                // =======================================================
                // THE SPI BURST FIX (Pause between traces)
                // =======================================================
                // We only need to pause if there are more traces to draw.
                // Doing this safely flushes the 1024-command FIFO.
                if (trace < (graph->activeTraces - 1)) {
                    API_LIB_EndCoProList(); 
                    API_LIB_AwaitCoProEmpty();
                    API_LIB_BeginCoProList();
                }
            }
        }
    }
}

void gfx_ImageRenderEVEComponents(gfx_Image *img) {
    if (!img || img->size.width == 0 || img->size.height == 0) return;

    // 1. Configurar el contexto de la imagen en el Handle 1
    API_BITMAP_HANDLE(1);
    API_BITMAP_SOURCE(img->ramgAddress);

    uint16_t stride = img->size.width * 2; // RGB565 usa 2 bytes por pixel

    API_BITMAP_LAYOUT(RGB565, stride, img->size.height);
    API_BITMAP_LAYOUT_H(stride >> 10, img->size.height >> 9);

    uint16_t drawnW = img->size.width * img->scale;
    uint16_t drawnH = img->size.height * img->scale;

    API_BITMAP_SIZE(NEAREST, BORDER, BORDER, drawnW, drawnH);
    API_BITMAP_SIZE_H(drawnW >> 9, drawnH >> 9);

    // 2. Aplicar matriz de escalado si es necesario
    if (img->scale > 1) {
        int32_t s32ScaleFactor = 65536 * img->scale; 
        API_CMD_LOADIDENTITY();
        API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
        API_CMD_SETMATRIX();
    }

    // 3. Dibujar el bitmap usando el Handle 1
    API_BEGIN(BITMAPS);
    
    // Nota: VERTEX2II usa parámetros (x, y, handle, cell)
    API_VERTEX2II(img->pos.x, img->pos.y, 1, 0); 
    
    API_END();

    // 4. Restaurar la matriz para no afectar a otros widgets de hardware
    if (img->scale > 1) {
        API_CMD_LOADIDENTITY();
        API_CMD_SETMATRIX();
    }
}

bool gfx_ImageLoadPNG(gfx_Image *img, const uint8_t *pngData, uint32_t dataSize, uint32_t targetRamGAddr) {
    if (!img || !pngData || dataSize == 0) return false;

    // 1. Decodificar el PNG apuntando a la zona segura de la RAM_G
    API_LIB_BeginCoProList();
    API_CMD_LOADIMAGE(targetRamGAddr, 0); 
    API_LIB_EndCoProList();
    
    // Inyectar los bytes del archivo PNG
    API_LIB_WriteDataToCMD(pngData, dataSize);
    API_LIB_AwaitCoProEmpty();

    // 2. Extraer las propiedades calculadas por EVE
    API_LIB_BeginCoProList();
    API_CMD_GETPROPS(0, 0, 0);
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();

    uint16_t REG_CMD_WRITE_OFFSET = EVE_MemRead16(REG_CMD_WRITE);
    
    // El ancho (width) se almacena 8 bytes atrás en la RAM_CMD
    uint16_t ParameterAddr = ((REG_CMD_WRITE_OFFSET - 8) & 4095);
    img->size.width = EVE_MemRead16((RAM_CMD + ParameterAddr));

    // El alto (height) se almacena 4 bytes atrás
    ParameterAddr = ((REG_CMD_WRITE_OFFSET - 4) & 4095);
    img->size.height = EVE_MemRead16((RAM_CMD + ParameterAddr));

    // Guardamos la dirección asignada
    img->ramgAddress = targetRamGAddr;
    
    // Valor por defecto
    if(img->scale == 0) img->scale = 1; 

    return true;
}