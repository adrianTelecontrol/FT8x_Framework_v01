
#include <stdlib.h>

#include <xdc/runtime/System.h>

#include "gfx.h"
#include "config_screen_FT8xx.h"
#include "Widgets.h"
#include "Surface.h"
#include "form_common.h"
#include "EVE_colors.h"

#include "showcase_form.h"

// Enable verbose output
// #define VERBOSE

static Surface g_sShowcaseSrf;

static widget_type_e g_eLockedWidgetType = WD_TYPE_NULL;
static void *g_pLockedWidget = NULL;

static void buttonOnClicked(Button *btn)
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

    #ifdef VERBOSE
    System_printf("Button clicked!");
    System_flush();
    #endif
}

static void buttonOnPosChanged(Button *btn, Position newPos)
{
    if(newPos.x >= 0 && newPos.x < LCD_WIDTH - btn->dim.width && newPos.y >= 0 &&newPos.y < LCD_HEIGHT - btn->dim.height)
    {
        btn->pos.x = newPos.x;
        btn->pos.y = newPos.y;
        gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
    }
    #ifdef VERBOSE
        System_printf("Button pos: (%d, %d)", btn->pos.x, btn->pos.y);
        System_flush();
    #endif
}

static void addButtonCallback(Button *btn)
{
    System_printf("Spawn button event");
    System_flush();

    Button *pNewButton = (Button *)malloc(sizeof(Button));
    if(pNewButton == NULL)
    {
        System_printf("Error: Failed to allocate button\n");
        return;
    }

    // Initialize the button
    pNewButton->name = "addButton";  // WARNING: See note below about string literals
    pNewButton->label = "Spawn Boton";
    pNewButton->dim.width = BUTTON_WIDTH;
    pNewButton->dim.height = BUTTON_HEIGHT / 2;
    pNewButton->pos.x = LCD_WIDTH / 2.0f - BUTTON_WIDTH / 2;
    pNewButton->pos.y = LCD_HEIGHT / 2.0f - BUTTON_HEIGHT / 2.0f;
    pNewButton->sizeFont = 28;
    pNewButton->color.foreground = EVE_PINK;
    pNewButton->color.text = EVE_WHITE;
    pNewButton->onClicked = buttonOnClicked;
    pNewButton->onPosChanged = buttonOnPosChanged;
    
    // Initialize touch region
    gfx_initRegTouch(pNewButton, WD_TYPE_BUTTON);


    GenericWidget spawnButton;
    spawnButton.eWidgetType = WD_TYPE_BUTTON;
    spawnButton.pvWidget = (void *)pNewButton;

    bool ret = srfInsertAtTop(&g_sShowcaseSrf.psWidgets, &spawnButton);
    // bool ret = srfInsertAtBottom(&g_sShowcaseSrf.psWidgets, &spawnButton);
    if(ret)
    {
        System_printf("Button succesfully spawned");
    }
    else
        System_printf("Error during button spawn");

}

static void deleteButtonCallback(Button *btn)
{
    System_printf("Delete button event");
    System_flush();

    if(srfDeleteAtEnd(&g_sShowcaseSrf.psWidgets))
        System_printf("Button succesfully deleted");
    else
        System_printf("Error deleting button ");
}


void initShowcaseForm(void)
{
    g_sShowcaseSrf.ui32BackgroundColor = EVE_BLACK;
    g_sShowcaseSrf.psWidgets = NULL;

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

    GenericWidget addButtonWidget;
    addButtonWidget.eWidgetType = WD_TYPE_BUTTON;
    addButtonWidget.pvWidget = (void *)&(Button){
        .name = "addButton",
        .label = "Spawn Boton",
        .dim.width = BUTTON_WIDTH,
        .dim.height = BUTTON_HEIGHT / 2,
        .pos.x = LCD_WIDTH / 3.0 - BUTTON_WIDTH / 2,
        .pos.y = LCD_HEIGHT - BUTTON_HEIGHT / 1.5f - VERTICAL_MARGIN,        
        .sizeFont = 28,
        .color.foreground = EVE_GREEN,
        .color.text = EVE_WHITE,
        .onClicked = addButtonCallback,
    };  
    gfx_initRegTouch(addButtonWidget.pvWidget, WD_TYPE_BUTTON);

    GenericWidget deleteButtonWidget;
    deleteButtonWidget.eWidgetType = WD_TYPE_BUTTON;
    deleteButtonWidget.pvWidget = (void *)&(Button){
        .name = "deleteButton",
        .label = "Elimina boton",
        .dim.width = BUTTON_WIDTH,
        .dim.height = BUTTON_HEIGHT / 2,
        .pos.x = LCD_WIDTH * ( 2.0f / 3.0f ) - BUTTON_WIDTH / 2,
        .pos.y = LCD_HEIGHT - VERTICAL_MARGIN - BUTTON_HEIGHT / 1.5f,
        .color.foreground = EVE_PURPLE,
        .color.text = EVE_WHITE,                                               
        .sizeFont = 27,
        .onClicked = deleteButtonCallback,
    };
    gfx_initRegTouch(deleteButtonWidget.pvWidget, WD_TYPE_BUTTON); 

    srfInsertAtTop(&g_sShowcaseSrf.psWidgets, &panelWidget);
    srfInsertAtTop(&g_sShowcaseSrf.psWidgets, &addButtonWidget);
    srfInsertAtTop(&g_sShowcaseSrf.psWidgets, &deleteButtonWidget);
}

void renderShowcaseForm(void)
{
    gfx_renderSurface(&g_sShowcaseSrf);
}

bool handleShowcaseFormTouch(TouchStatus touchStatus, gesture_type_e gesture)
{
    Position newPos;
    Button *btn;
    GenericWidgetNode *temp = NULL;
    switch (gesture) 
    {
        case GESTURE_LOCK_OBJ:
            #ifdef VERBOSE
            System_printf("Gesture: Locking object");
            #endif
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            
            temp = g_sShowcaseSrf.psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    Button *btn = ((Button *)temp->sWidget.pvWidget);
                    System_printf("\tLocked onto %s", btn->name);
                    g_pLockedWidget = temp->sWidget.pvWidget;
                    g_eLockedWidgetType = WD_TYPE_BUTTON;

                    break;
                }

                temp = temp->psNext;
            }
            break;
        case GESTURE_DRAG:
            #ifdef VERBOSE
            System_printf("Gesture: Dragging object\t");
            #endif
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
            #ifdef VERBOSE
            System_printf("Gesture: Release object\n");
            #endif
            
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
            #ifdef VERBOSE
            System_printf("Gesture: Clicking object\t");
            #endif
            temp = g_sShowcaseSrf.psWidgets;
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
