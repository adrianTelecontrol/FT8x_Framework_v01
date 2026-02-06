#ifndef GFX_H 
#define GFX_H
    
#include <stdint.h>
#include <stdbool.h>

#include "Widgets.h"
#include "FT8xx.h"

bool gfx_initRegTouch(void *, widget_type_e);

TouchStatus gfx_touchReadRegion(void);

bool gfx_touchObject(RegionTouchObject, TouchStatus);

bool gfx_isWidgetTouched(GenericWidget*, TouchStatus);

bool gfx_clearSurface(Surface *);

bool gfx_renderSurface(Surface *);

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


/**
 * gfx_Button  - Render a button
 * @bt: button to be drawn
 */
void gfx_Button(Button *bt);

/**
 * gfx_ColorText - Helper function to select ColorText
 *
 * @color: Color for the text
 */
void gfx_ColorText(uint32_t color);

#endif // GFX_H 


