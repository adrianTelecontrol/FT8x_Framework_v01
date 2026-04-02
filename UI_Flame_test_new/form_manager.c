
#include <stdlib.h>

#include "gfx.h"
#include "graphics_engine.h"
#include "gesture_engine.h"

#include "forms_manager.h"


static gfx_Canvas *g_psForms[MAX_CANVAS_WIDGETS];

static uint8_t g_ui8FormCounter = 0;
gfx_Canvas *g_psCurrentForm;

static widget_type_e g_eLockedWidgetType = WD_TYPE_NULL;
static void *g_pLockedWidget = NULL;

void formManagerLoadForm(gfx_Canvas *form)
{
	if(g_ui8FormCounter < MAX_CANVAS_WIDGETS)
	{
		g_psForms[g_ui8FormCounter] = form;
		g_psCurrentForm = g_psForms[g_ui8FormCounter];
		
		g_ui8FormCounter++;
	}
}

/*
bool formManagerHandleGesture(TouchStatus touchStatus, gesture_type_e gesture)
{
    static Position newPos;
    static gfx_Button *btn;
    static gfx_GenericWidgetNode *temp = NULL;

    switch (gesture) 
    {
        case GESTURE_LOCK_OBJ:
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            
			temp = g_psCurrentForm->psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    g_pLockedWidget = temp->sWidget.pvWidget;
                    g_eLockedWidgetType = WD_TYPE_BUTTON;

                    break;
                }

                temp = temp->psNext;
            }
            break;
        case GESTURE_DRAG:
            if(g_pLockedWidget == NULL)
                break;

            if(g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                btn = (gfx_Button *)g_pLockedWidget;
                newPos.x = touchStatus.x - btn->size.width / 2;
                newPos.y = touchStatus.y - btn->size.height / 2;
                if(btn->onPosChanged != NULL)
                    btn->onPosChanged(btn, newPos);
            }
            break;
        case GESTURE_RELEASE:
			// Only clicking
			temp = g_psCurrentForm->psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    g_pLockedWidget = temp->sWidget.pvWidget;
                    g_eLockedWidgetType = WD_TYPE_BUTTON;

                    break;
                }

                temp = temp->psNext;
            }

            // btn = (gfx_Button *)g_pLockedWidget;
            // if(btn->onRelease != NULL)
            // {
            //     btn->onRelease(btn);
            // }
			
            // Call release callback if locked widget has one
            if(g_pLockedWidget != NULL && g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                btn = (gfx_Button *)g_pLockedWidget;
                if(btn->onRelease != NULL)
                {
                    btn->onRelease(btn);
                }
            }
            
            // Clear locked widget
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            break;

        case GESTURE_PRESSED:
			temp = g_psCurrentForm->psWidgets;
            while (temp != NULL) 
            {
                if(gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    gfx_Button *btn = ((gfx_Button *)temp->sWidget.pvWidget);
                    if(btn->onPressed != NULL)
                        btn->onPressed(btn);

                    break;
                }
                temp = temp->psNext;
            }
            break;
        default:
            break;
    }

    return true;
} */

bool formManagerHandleGesture(TouchStatus touchStatus, gesture_type_e gesture)
{
    gfx_GenericWidgetNode *temp = NULL;

    switch (gesture) 
    {
        case GESTURE_PRESSED:
            // 1. Search for the touched widget
            temp = g_psCurrentForm->psWidgets;
            while (temp != NULL) 
            {
                if (gfx_isWidgetTouched(&temp->sWidget, touchStatus))
                {
                    // 2. Lock the widget so we remember it for the Release phase
                    g_pLockedWidget = temp->sWidget.pvWidget;
                    g_eLockedWidgetType = temp->sWidget.eWidgetType;

                    // 3. Fire the Pressed callback
                    if (g_eLockedWidgetType == WD_TYPE_BUTTON) 
                    {
                        gfx_Button *btn = (gfx_Button *)g_pLockedWidget;
                        if (btn->onPressed != NULL) {
                            btn->onPressed(btn);
                        }
                    }
                    
                    // Stop searching once we find a hit (respects Z-order)
                    break;
                }
                temp = temp->psNext;
            }
            break;

        case GESTURE_RELEASE:
            // 1. Fire the Release callback on whatever we locked during PRESSED
            if (g_pLockedWidget != NULL && g_eLockedWidgetType == WD_TYPE_BUTTON)
            {
                gfx_Button *btn = (gfx_Button *)g_pLockedWidget;
                if (btn->onRelease != NULL)
                {
                    btn->onRelease(btn);
                }
            }
            
            // 2. Clear the lock completely
            g_pLockedWidget = NULL;
            g_eLockedWidgetType = WD_TYPE_NULL;
            break;

        default:
            // Ignore GESTURE_DRAG and GESTURE_LOCK_OBJ for now
            break;
    }

    return true;
}

void formManagerComposite(pixel16_t *psPixelBuffer)
{
	gfx_compositeFrame(g_psCurrentForm, psPixelBuffer);
}



