
#include <stdlib.h>
#include <string.h>

#include "event_engine.h"
#include "gesture_engine.h"
#include "gfx.h"
#include "EVE.h"
#include "graphics_engine.h"

#include "forms/graph_form.h"
#include "forms/home_form.h"
#include "forms/navigation_widgets.h"

#include "forms_manager.h"

static gfx_Canvas *g_psForms[MAX_CANVAS_WIDGETS];

static uint8_t g_ui8FormCounter = 0;
gfx_Canvas *g_psCurrentForm;
uint16_t g_ui16CurrentIndex = 0;

static widget_type_e g_eLockedWidgetType = WD_TYPE_NULL;
static void *g_pLockedWidget = NULL;

int16_t formManagerAddForm(gfx_Canvas *form) {
  if (g_ui8FormCounter < MAX_CANVAS_WIDGETS) {
    g_psForms[g_ui8FormCounter] = form;

    g_ui8FormCounter++;
    return g_ui8FormCounter - 1;
  }

  return -1;
}

static void formManagerOnFormChange(uint32_t arg) {
  int16_t formID = (int16_t)arg;

  if (formID == -1)
    return;

  g_psCurrentForm = g_psForms[formID];

  Event_Post(EVT_CMD_FULL_REPAINT, 0);
}

static void formManagerOnNextFormEvent(uint32_t arg) {
  g_ui16CurrentIndex = (g_ui16CurrentIndex + 1) % g_ui8FormCounter;

  g_psCurrentForm = g_psForms[g_ui16CurrentIndex];

  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, 0);
}

static void formManagerOnPrevFormEvent(uint32_t arg) {
  if (g_ui8FormCounter == 0)
    return; // Protección de seguridad

  // Al sumar g_ui8FormCounter antes de restar 1, evitamos el underflow
  // matemático
  g_ui16CurrentIndex =
      (g_ui16CurrentIndex + g_ui8FormCounter - 1) % g_ui8FormCounter;

  g_psCurrentForm = g_psForms[g_ui16CurrentIndex];
  
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, 0);
}

bool formManagerHandleGesture(TouchStatus touchStatus, gesture_type_e gesture) {
  static Position newPos;
  static gfx_Button *btn;
  static gfx_GenericWidgetNode *temp = NULL;

  switch (gesture) {
  case GESTURE_LOCK_OBJ:
    g_pLockedWidget = NULL;
    g_eLockedWidgetType = WD_TYPE_NULL;

    temp = g_psCurrentForm->psWidgets;
    while (temp != NULL) {
      if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
        g_pLockedWidget = temp->sWidget.pvWidget;
        g_eLockedWidgetType = temp->sWidget.eWidgetType;

        break;
      }

      temp = temp->psNext;
    }
    break;
  case GESTURE_DRAG:
    if (g_pLockedWidget == NULL)
      break;

    if (g_eLockedWidgetType == WD_TYPE_BUTTON) {
      btn = (gfx_Button *)g_pLockedWidget;
      newPos.x = touchStatus.x - btn->size.width / 2;
      newPos.y = touchStatus.y - btn->size.height / 2;
      if (btn->onPosChanged != NULL) {
        btn->onPosChanged(btn, newPos);
      }
    } else if (g_eLockedWidgetType == WD_TYPE_SLIDER) {
      gfx_Slider *sld = (gfx_Slider *)g_pLockedWidget;
      gfx_processSliderTouch(sld, touchStatus);
    }
    break;
  case GESTURE_RELEASE:
    // Only clicking
    temp = g_psCurrentForm->psWidgets;
    while (temp != NULL) {
      if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
        g_pLockedWidget = temp->sWidget.pvWidget;
        g_eLockedWidgetType = temp->sWidget.eWidgetType;

        break;
      }

      temp = temp->psNext;
    }

    // Call release callback if locked widget has one
    if (g_pLockedWidget != NULL) {
      if (g_eLockedWidgetType == WD_TYPE_BUTTON) {
        btn = (gfx_Button *)g_pLockedWidget;
        if (btn->onRelease != NULL) {
          btn->onRelease(btn);
        }
      } else if (g_eLockedWidgetType == WD_TYPE_SLIDER) {
        gfx_Slider *sld = (gfx_Slider *)g_pLockedWidget;
        gfx_processSliderTouch(sld, touchStatus);
      }
    }

    // Clear locked widget
    g_pLockedWidget = NULL;
    g_eLockedWidgetType = WD_TYPE_NULL;
    break;

  case GESTURE_PRESSED:
    temp = g_psCurrentForm->psWidgets;
    while (temp != NULL) {
      if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
        if (temp->sWidget.eWidgetType == WD_TYPE_BUTTON) {
          gfx_Button *btn = ((gfx_Button *)temp->sWidget.pvWidget);
          if (btn->onPressed != NULL)
            btn->onPressed(btn);
        } else if (temp->sWidget.eWidgetType == WD_TYPE_SLIDER) {
          gfx_Slider *sld = (gfx_Slider *)temp->sWidget.pvWidget;
          gfx_processSliderTouch(sld, touchStatus);
        }

        break;
      }
      temp = temp->psNext;
    }
    break;
  default:
    break;
  }

  return true;
}

void formManagerInit(void) {
  initNavigationWidgets();
  initHomeForm();
  initGraphForm();
  Event_Subscribe(EVT_SYS_NEXT_FORM,
                  (EventHandler_fn)formManagerOnNextFormEvent);
  Event_Subscribe(EVT_SYS_PREV_FORM,
                  (EventHandler_fn)formManagerOnPrevFormEvent);

  g_psCurrentForm = g_psForms[g_i16GraphFormID];
}

void formManagerComposite(pixel16_t *psPixelBuffer) {
  gfx_compositeFrame(g_psCurrentForm, psPixelBuffer);
}

void formManagerRenderEVEComponents(void) {
  if(!g_bIsBackgroundReady) return;

  gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;
  // Lets check if the canvas contains a widget with EVE elements

  while (iter != NULL) {
    if (iter->sWidget.eWidgetType == WD_TYPE_GRAPH) {
		gfx_Graph *gf = (gfx_Graph *)iter->sWidget.pvWidget;
		if(gf->bEVEDirty) {
			// If one of the components is dirty, then interate the whole
			// canvas looking for more dirty widgets
			break;
		}
	} else if (iter->sWidget.eWidgetType == WD_TYPE_MULTIGRAPH) {
		gfx_MultiGraph *gf = (gfx_MultiGraph *)iter->sWidget.pvWidget;
		if(gf->bEVEDirty) {
			// If one of the components is dirty, then interate the whole
			// canvas looking for more dirty widgets
			break;
		}
	}

    iter = iter->psNext;
  }
  if (iter == NULL)
    return;
  
  iter = g_psCurrentForm->psWidgets;
  // 1. Iniciamos una nueva Display List
  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0);
  API_CLEAR(1, 1, 1);
  API_COLOR_RGB(255, 255, 255);

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
  API_END();                 // Cerramos el dibujo de bitmaps

  while (iter != NULL) {
    if (iter->sWidget.eWidgetType == WD_TYPE_GRAPH) {
		gfx_Graph *gf = (gfx_Graph *)iter->sWidget.pvWidget;
		if(gf->bEVEDirty) {
      		gfx_GraphRenderEVEComponents((gfx_Graph *)iter->sWidget.pvWidget);
			gf->bEVEDirty = false;

    		API_LIB_EndCoProList();
    		API_LIB_AwaitCoProEmpty();
    		API_LIB_BeginCoProList();
		}
    } else if (iter->sWidget.eWidgetType == WD_TYPE_MULTIGRAPH) {
		gfx_MultiGraph *gf = (gfx_MultiGraph *)iter->sWidget.pvWidget;
		if(gf->bEVEDirty) {
      		gfx_MultigraphRenderEVEComponents(gf);
			gf->bEVEDirty = false;

    		API_LIB_EndCoProList();
    		API_LIB_AwaitCoProEmpty();
    		API_LIB_BeginCoProList();
		}
    }

    iter = iter->psNext;
  }

  // 3. Cerramos y hacemos el SWAP en el hardware
  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();

  // Esperamos a que todo el frame (SWAP incluido) sea procesado
  API_LIB_AwaitCoProEmpty();
}
