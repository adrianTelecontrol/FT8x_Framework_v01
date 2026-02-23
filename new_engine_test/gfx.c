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

#include "gfx.h"

bool gfx_initRegTouch(void *widget, widget_type_e type) {
  if (type == WD_TYPE_BUTTON) {
    Button *wd = (Button *)widget;
    wd->regTouch.x1 = wd->pos.x;
    wd->regTouch.x2 = wd->pos.x + wd->dim.width;
    wd->regTouch.y1 = wd->pos.y;
    wd->regTouch.y2 = wd->pos.y + wd->dim.height;
  }

  return true;
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

bool gfx_renderSurface(Surface *srf) {
  if (srf == NULL) {
    UARTprintf("Cannot render surface! Is empty.");
    return false;
  }

  GenericWidgetNode *iter = srf->psWidgets;
  gfx_start(srf->ui32BackgroundColor);
  while (iter != NULL) {
    switch (iter->sWidget.eWidgetType) {
    case WD_TYPE_BUTTON:
      gfx_Button((Button *)iter->sWidget.pvWidget);
      break;
    case WD_TYPE_RECT:
      gfx_Rectangle((Rectangle *)iter->sWidget.pvWidget);
      break;
    default:
      break;
    }

    iter = iter->psNext;
  }
  gfx_end();

  return true;
}

bool gfx_isWidgetTouched(GenericWidget *wd, TouchStatus touch) {
  if (wd->pvWidget == NULL)
    return false;

  bool ret = false;
  switch (wd->eWidgetType) {
  case WD_TYPE_BUTTON:
    ret = gfx_touchObject(((Button *)wd->pvWidget)->regTouch, touch);
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

void gfx_Point(Point *p) {
  gfx_ColorText(p->color);
  API_BEGIN(FTPOINTS);
  API_POINT_SIZE(p->ratio * 16);
  API_VERTEX2F(p->pos.x * 16, p->pos.y * 16);
  API_END();
}

void gfx_Line(Line *l) {
  API_BEGIN(LINES);
  API_LINE_WIDTH(l->size);
  gfx_ColorText(l->color);
  API_VERTEX2F(l->posInitial.x * 16, l->posInitial.y * 16);
  API_VERTEX2F(l->posFinal.x * 16, l->posFinal.y * 16);
  API_END();
}

void gfx_Rectangle(Rectangle *r) {
  API_BEGIN(RECTS);
  if (r->round != 0) // Revisa el redondeo del rectángulo
    API_LINE_WIDTH(r->round);
  gfx_ColorText(r->color);
  // Se colocan los vertices
  API_VERTEX2F(r->pos.x * 16, r->pos.y * 16);
  API_VERTEX2F((r->pos.x + r->dim.width) * 16, (r->pos.y + r->dim.height) * 16);
  API_END();
}

void gfx_Button(Button *bt) {
  gfx_ColorText(bt->color.text);
  API_CMD_FGCOLOR(bt->color.foreground);
  API_CMD_BUTTON(bt->pos.x, bt->pos.y, bt->dim.width, bt->dim.height,
                 bt->sizeFont, bt->status, bt->label);
}

void gfx_ColorText(uint32_t color) {
  API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);
}
