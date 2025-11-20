
#include <xdc/runtime/System.h>
#include "gfx.h"
#include "config_screen_FT8xx.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "home_form.h"

static Rectangle g_sPanelRect;
static Button g_sPushButton;
static Button g_sSecondButton;

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

void pushButtonOnRelease(Button *btn)
{

}

void pushButtonOnPosChanged(Button *btn, Position newPos)
{
    if(newPos.x >= 0 && newPos.x < LCD_WIDTH - btn->dim.width && newPos.y >= 0 &&newPos.y < LCD_HEIGHT - btn->dim.height)
    {
        btn->pos.x = newPos.x;
        btn->pos.y = newPos.y;
        gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
    }
    System_printf("Button pos: (%d, %d)", btn->pos.x, btn->pos.y);
    System_flush();
}

void initHomeForm(void)
{
    g_sPanelRect.pos.x = HORIZONTAL_MARGIN;
    g_sPanelRect.pos.y = VERTICAL_MARGIN;
    g_sPanelRect.dim.width = LCD_WIDTH - 2 * HORIZONTAL_MARGIN;
    g_sPanelRect.dim.height = LCD_HEIGHT - 2 * VERTICAL_MARGIN;
    g_sPanelRect.round = 0;
    g_sPanelRect.color = EVE_BLUE;

    g_sPushButton.name = "btn1";
    g_sPushButton.dim.width = BUTTON_WIDTH;
    g_sPushButton.dim.height = BUTTON_HEIGHT;
    g_sPushButton.pos.x = LCD_WIDTH / 2 - g_sPushButton.dim.width / 2;
    g_sPushButton.pos.y = LCD_HEIGHT / 2 - g_sPushButton.dim.height / 2;
    g_sPushButton.label = "PUSH ME";
    g_sPushButton.sizeFont = 28;
    gfx_initRegTouch((void *)&g_sPushButton, WD_TYPE_BUTTON);
    g_sPushButton.color.foreground = EVE_GREEN;
    g_sPushButton.color.text = EVE_WHITE;

    g_sSecondButton.name = "btn2";
    g_sSecondButton.dim.height = 100;
    g_sSecondButton.dim.width = 100;
    g_sSecondButton.pos.x = LCD_WIDTH - HORIZONTAL_MARGIN - g_sSecondButton.dim.width;
    g_sSecondButton.pos.y = LCD_HEIGHT / 2 - g_sSecondButton.dim.height / 2;
    g_sSecondButton.label = "TOUCH ME";
    g_sSecondButton.color.foreground = EVE_PURPLE;
    g_sSecondButton.color.text = EVE_WHITE;
    g_sSecondButton.sizeFont = 27;
    gfx_initRegTouch((void *)&g_sSecondButton, WD_TYPE_BUTTON); 

    g_sPushButton.onClicked = pushButtonOnClicked;
    g_sPushButton.onRelease = pushButtonOnRelease;
    g_sPushButton.onPosChanged = pushButtonOnPosChanged;

    g_sSecondButton.onClicked = pushButtonOnClicked;
    g_sSecondButton.onPosChanged = pushButtonOnPosChanged;
}

void renderHomeForm(void)
{
    gfx_start(EVE_BLACK);
    gfx_Rectangle(&g_sPanelRect);
    gfx_Button(&g_sPushButton);
    gfx_Button(&g_sSecondButton);
    gfx_end();
}

bool handleHomeFormTouch(TouchStatus touchStatus, gesture_type_e gesture)
{
    Position newPos;
    Button *btn;
    switch (gesture) 
    {
        case GESTURE_LOCK_OBJ:
            System_printf("Gesture: Locking object");

            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;

            if( gfx_touchObject(g_sPushButton.regTouch, touchStatus) )
            {
                System_printf("\tLocked onto %s", g_sPushButton.name);
                g_pLockedWidget = (void *)&g_sPushButton;
                g_eLockedWidgetType = WD_TYPE_BUTTON;
            }
            else if( gfx_touchObject(g_sSecondButton.regTouch, touchStatus) )
            {
                System_printf("\tLocked onto %s", g_sSecondButton.name);
                g_pLockedWidget = (void *)&g_sSecondButton;
                g_eLockedWidgetType = WD_TYPE_BUTTON;
            }
            break;
        case GESTURE_DRAG:
            System_printf("Gesture: Dragging object\t");
            if(g_pLockedWidget == NULL)
                break;

            if(g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                btn = (Button *)g_pLockedWidget;
                newPos.x = touchStatus.x - btn->dim.width / 2;
                newPos.y = touchStatus.y - btn->dim.height / 2;
                btn->onPosChanged(btn, newPos);
            }
            break;
        case GESTURE_RELEASE:
            System_printf("Gesture: Release object\n");
            
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
            System_printf("Gesture: Clicking object");
            if(gfx_touchObject(g_sPushButton.regTouch, touchStatus))
                g_sPushButton.onClicked(&g_sPushButton);
            else if(gfx_touchObject(g_sSecondButton.regTouch, touchStatus))
                g_sSecondButton.onClicked(&g_sSecondButton);
            break;
        default:
            break;
    }
    System_flush();

    return true;
}
