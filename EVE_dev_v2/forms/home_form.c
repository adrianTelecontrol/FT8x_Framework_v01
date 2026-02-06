
#include <stdlib.h>

#include <xdc/runtime/System.h>

#include "gfx.h"
#include "config_screen_FT8xx.h"
#include "Widgets.h"
#include "Surface.h"
#include "form_common.h"
#include "EVE_colors.h"

#include "home_form.h"

static Surface g_sHomeSurface;

static widget_type_e g_eLockedWidgetType = WD_TYPE_NULL;
static void *g_pLockedWidget = NULL;

void pushButtonOnClicked(Button *btn)
{
    if(btn->status == BTN_STATE_PRESSED)
        btn->status = BTN_STATE_NORMAL;
    else
    {
        btn->status = BTN_STATE_PRESSED;
        // btn->pos.x = LCD_WIDTH / 2 - btn->dim.width / 2;
        // btn->pos.y = LCD_HEIGHT / 2 - btn->dim.height / 2;
        // gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
    }

    System_printf("Button clicked!");
    System_flush();
}

void pushButtonOnPosChanged(Button *btn, Position newPos)
{
    if(newPos.x >= 0 && newPos.x < LCD_WIDTH - btn->dim.width && newPos.y >= 0 &&newPos.y < LCD_HEIGHT - btn->dim.height)
    {
        btn->pos.x = newPos.x;
        btn->pos.y = newPos.y;
        gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
    }
    //System_printf("Button pos: (%d, %d)", btn->pos.x, btn->pos.y);
    System_flush();
}

void initHomeForm(void)
{
    g_sHomeSurface.ui32BackgroundColor = EVE_BLACK;
    g_sHomeSurface.psWidgets = NULL;

    GenericWidget panelWidget;
    panelWidget.eWidgetType = WD_TYPE_RECT;
    panelWidget.pvWidget = (void *)&(Rectangle){
        .name = "bgPanel",
        .pos.x = HORIZONTAL_MARGIN,
        .pos.y = VERTICAL_MARGIN,
        .dim.width = LCD_WIDTH - 2 * HORIZONTAL_MARGIN,
        .dim.height = LCD_HEIGHT - 2 * VERTICAL_MARGIN,
        .round = 0,
        .color = EVE_BLUE,
    };

    GenericWidget buttonWidget;
    buttonWidget.eWidgetType = WD_TYPE_BUTTON;
    buttonWidget.pvWidget = (void *)&(Button){
        .name = "btn1",
        .label = "PUSH ME",
        .dim.width = BUTTON_WIDTH,
        .dim.height = BUTTON_HEIGHT,
        .pos.x = LCD_WIDTH / 2 - BUTTON_WIDTH / 2,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,        
        .sizeFont = 28,
        .color.foreground = EVE_GREEN,
        .color.text = EVE_WHITE,
        .onPosChanged = pushButtonOnPosChanged,
        .onClicked = pushButtonOnClicked,
    };  
    gfx_initRegTouch(buttonWidget.pvWidget, WD_TYPE_BUTTON);

    GenericWidget secondWidget;
    secondWidget.eWidgetType = WD_TYPE_BUTTON;
    secondWidget.pvWidget = (void *)&(Button){
        .name = "btn2",
        .dim.height = 100,
        .dim.width = 100,
        .pos.x = LCD_WIDTH - HORIZONTAL_MARGIN - BUTTON_WIDTH,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .label = "TOUCH ME",
        .color.foreground = EVE_PURPLE,
        .color.text = EVE_WHITE,                                               
        .sizeFont = 27,
        .onPosChanged = pushButtonOnPosChanged,
        .onClicked = pushButtonOnClicked,
    };
    gfx_initRegTouch(secondWidget.pvWidget, WD_TYPE_BUTTON); 

    srfInsertAtTop(&g_sHomeSurface.psWidgets, &panelWidget);
    srfInsertAtTop(&g_sHomeSurface.psWidgets, &buttonWidget);
    srfInsertAtTop(&g_sHomeSurface.psWidgets, &secondWidget);
}

void renderHomeForm(void)
{
    gfx_renderSurface(&g_sHomeSurface);
}

bool handleHomeFormTouch(TouchStatus touchStatus, gesture_type_e gesture)
{
    Position newPos;
    Button *btn;
    GenericWidgetNode *temp = NULL;
    switch (gesture) 
    {
        case GESTURE_LOCK_OBJ:
            //System_printf("Gesture: Locking object");
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            
            temp = g_sHomeSurface.psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    Button *btn = ((Button *)temp->sWidget.pvWidget);
                    //System_printf("\tLocked onto %s", btn->name);
                    g_pLockedWidget = temp->sWidget.pvWidget;
                    g_eLockedWidgetType = WD_TYPE_BUTTON;

                    break;
                }

                temp = temp->psNext;
            }
            break;
        case GESTURE_DRAG:
            //System_printf("Gesture: Dragging object\t");
            if(g_pLockedWidget == NULL)
                break;

            if(g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                btn = (Button *)g_pLockedWidget;
                newPos.x = touchStatus.x - btn->dim.width / 2;
                newPos.y = touchStatus.y - btn->dim.height / 2;
                if(btn->onPosChanged != NULL)
                    btn->onPosChanged(btn, newPos);
            }
            break;
        case GESTURE_RELEASE:
            //System_printf("Gesture: Release object\n");
            
            // Call release callback if locked widget has one
            if(g_pLockedWidget != NULL && g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                btn = (Button *)g_pLockedWidget;
                if(btn->onRelease != NULL)
                {
                    btn->onRelease(btn);
                }
            }
            
            // Clear locked widget
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            break;

        case GESTURE_CLICK:
            //System_printf("Gesture: Clicking object\t");
            temp = g_sHomeSurface.psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    Button *btn = ((Button *)temp->sWidget.pvWidget);
                    if(btn->onClicked != NULL)
                        btn->onClicked(btn);

                    break;
                }
                temp = temp->psNext;
            }
            break;
        default:
            break;
    }
    System_flush();

    return true;
}
