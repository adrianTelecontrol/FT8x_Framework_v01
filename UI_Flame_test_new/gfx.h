#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include <stdint.h>

#include "FT8xx.h"
#include "font_engine.h"
#include "graphics_engine.h"
#include "gfx_theme.h"

#define MAX_CANVAS_WIDGETS 20

// Darkens any RGB565 color by 50% safely and instantly
#define DARKEN_COLOR(c) (((c) & 0xF7DE) >> 1)

typedef enum {
	STYLE_DEFAULT = 0,
	STYLE_PRIMARY,
	STYLE_SECONDARY,
	STYLE_DANGER,
	STYLE_SUCCESS
} gfx_WidgetStyle_e;

typedef enum {
  WD_TYPE_NULL = 0,
  WD_TYPE_RECT,
  WD_TYPE_BUTTON,
  WD_TYPE_LABEL,
} widget_type_e;

typedef struct RegionTouchObject {
  uint16_t x1;
  uint16_t y1;
  uint16_t x2;
  uint16_t y2;
} RegionTouchObject;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t state;
} TouchStatus;

typedef struct {
  uint16_t width;
  uint16_t height;
} Size;

typedef struct Position {
  int16_t x;
  int16_t y;
} Position;

/************************  Widgets struct ***********************/
typedef struct Point {
  Position pos;
  uint16_t ratio;
  uint32_t color;
} Point;

typedef struct Line {
  Position posInitial;
  Position posFinal;
  uint16_t size;
  uint32_t color;
} Line;

typedef struct Rectangle {
  Position pos;
  Size dim;
  uint16_t round;
  uint32_t color;
  const char *name;

  bool bIsDirty;
} gfx_Rectangle;

typedef enum {
  BTN_STATE_NORMAL = 0,
  BTN_STATE_PRESSED,
  BTN_STATE_DISABLED
} ButtonState_e;

typedef struct Button {
    Size size;
    Position pos;
    Position oldPos;
    RegionTouchObject regTouch;
    
    uint16_t radius;
    uint8_t borderWidth;
    
    gfx_WidgetStyle_e style;  // Define el COLOR global
    gfx_TypoStyle_e typo;     // Define la FUENTE global
    ButtonState_e state;
    
    char *label;
    char *name;
    bool bIsDirty;

    void (*onPressed)(struct Button *);
    void (*onRelease)(struct Button *);
    void (*onPosChanged)(struct Button *, Position newPos);
} gfx_Button;

typedef struct {
    Position pos;
    char *text;
    
    // The Magic Hooks
    gfx_WidgetStyle_e style;  // Resolves to g_pCurrentTheme->palette
    gfx_TypoStyle_e typo;     // Resolves to g_pCurrentTheme->fonts
    gfx_Align_e alignment;
    
    bool bIsDirty;
} gfx_Label;

typedef struct {
  widget_type_e eWidgetType;

  void *pvWidget;
} gfx_GenericWidget;

typedef struct GenericWidgetNode {
  gfx_GenericWidget sWidget;
  struct GenericWidgetNode *psNext;
  struct GenericWidgetNode *psPrev;
} gfx_GenericWidgetNode;

typedef struct {
  uint16_t ui16BackgroundColor;
  gfx_GenericWidgetNode *psWidgets;
} gfx_Canvas;

bool gfx_initRegTouch(void *, widget_type_e);

TouchStatus gfx_touchReadRegion(void);

//
// ************ Control Funcitons ************************
//
bool gfx_touchObject(RegionTouchObject, TouchStatus);

bool gfx_isWidgetTouched(gfx_GenericWidget *, TouchStatus);

bool gfx_clearSurface(gfx_Canvas *);

bool gfx_compositeFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer);

void gfx_start(uint32_t colorBackground);

void gfx_end(void);

void gfx_clear(void);

void gfx_calibrate(void);

//
// ************ PrimitiveFuncitons ************************
//
void gfx_drawCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color);

void gfx_drawCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t cornername, uint16_t color);

void gfx_fillCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color);

void gfx_fillCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t corners, int16_t delta, uint16_t color);

void gfx_drawEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color);
void gfx_fillEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color);

void gfx_drawTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void gfx_fillTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void gfx_fillRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color);

void gfx_fillGradientRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                               int16_t h, int16_t r, uint16_t colorTop,
                               uint16_t colorBottom);
//
// ************ Widget Funtions ************************
//

void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn);

void gfx_drawLabel(pixel16_t *pBuf, gfx_Label *lb);

void gfx_drawRectangle(pixel16_t *pBuf, gfx_Rectangle *rect);
#endif // GFX_H
