#ifndef WIDGETS_H
#define WIDGETS_H

#define BTN_STATE_PRESSED   OPT_FLAT
#define BTN_STATE_NORMAL    0

#define MAX_SURFACE_WIDGETS 20

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
    const char *name;
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

typedef struct
{
    widget_type_e eWidgetType;
    void *pvWidget;
} GenericWidget;

typedef struct GenericWidgetNode
{
    GenericWidget sWidget;
    struct GenericWidgetNode *psNext;
    struct GenericWidgetNode *psPrev;    
} GenericWidgetNode;

typedef struct
{
    uint32_t ui32BackgroundColor;
    GenericWidgetNode *psWidgets;
} Surface;


#endif // WIDGETS_H


