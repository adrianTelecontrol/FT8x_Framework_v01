#ifndef WIDGETS_H
#define WIDGETS_H

#include <stdint.h>
#include <stdbool.h>
#include "FT8xx.h"

#define BTN_STATE_PRESSED   OPT_FLAT
#define BTN_STATE_NORMAL    0

typedef enum {
    WD_TYPE_NULL = 0,
    WD_TYPE_RECT,
    WD_TYPE_BUTTON,
} widget_type_e;

typedef struct RegionTouchObject
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
} RegionTouchObject;

typedef struct TouchStatus
{
    uint16_t x;
    uint16_t y;
    uint8_t state;
} TouchStatus;

typedef struct Dimensions
{
    uint16_t width;
    uint16_t height;
} Dimensions;

typedef struct Position
{
    int16_t x;
    int16_t y;
} Position;

typedef struct ColorTF
{
    uint32_t text;
    uint32_t foreground;
} ColorTF;

typedef struct ColorSB
{
    uint32_t status;
    uint32_t background;
} ColorSB;

typedef struct ColorTBF
{
    uint32_t text;
    uint32_t background;
    uint32_t foreground;
} ColorTBF;

typedef struct ColorBFC
{
    uint32_t background;
    uint32_t foreground;
    uint32_t change;
} ColorBFC;

/************************  Widgets struct ***********************/
typedef struct Point
{
    Position pos;
    uint16_t ratio;
    uint32_t color;
} Point;

typedef struct Line
{
    Position posInitial;
    Position posFinal;
    uint16_t size;
    uint32_t color;
} Line;

typedef struct Rectangle
{
    Position pos;
    Dimensions dim;
    uint16_t round;
    uint32_t color;
} Rectangle;

typedef struct Button
{
    ColorTF color;
    Dimensions dim;
    Position pos;
    RegionTouchObject regTouch;
    uint32_t status;
    uint8_t sizeFont;
    uint8_t activate;
    const char *label;
    const char *name;

    void (*onClicked)(struct Button*);
    void (*onRelease)(struct Button*);
    void (*onPosChanged)(struct Button*, Position newPos);
} Button;

bool gfx_initRegTouch(void *, widget_type_e);

TouchStatus gfx_touchReadRegion(void);

bool gfx_touchObject(RegionTouchObject, TouchStatus);

/**
 * gfx_start - Clean the screen with the color indicated and starts the cmd sequence to draw
 *
 * @colorBackground: Background color for the screen
 */
void gfx_start(uint32_t colorBackground);

/**
 * gfx_end - Ends the drawing cmd sequence
 */
void gfx_end(void);

/**
 * gfx_clear - Clear the screen with black color
 */
void gfx_clear(void);

/**
 * gfx_Point: Draw a point in the screen
 *
 * point: Point to be drawn
 */
void gfx_Point(Point *point);

/**
 * gfx_Line: Draws a line in the screen
 *
 * @line: The line to be drawn
 */
void gfx_Line(Line *line);

/**
 * gfx_Rectangle - Draws a rectangle in the screen
 *
 * @rect: Rectangle to be drawn
 */
void gfx_Rectangle(Rectangle *rect);


void gfx_Button(Button *bt);

/**
 * gfx_ColorText - Helper function to select ColorText
 *
 * @color: Color for the text
 */
void gfx_ColorText(uint32_t color);

#endif // WIDGETS_H


