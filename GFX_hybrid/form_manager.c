
#include <stdlib.h>
#include <string.h>

#include "event_engine.h"
#include "gesture_engine.h"
#include "gfx.h"
#include "EVE.h"
#include "FT8xx_params.h"
#include "graphics_engine.h"

#include "forms/common_widgets.h"
#include "forms/graph_form.h"
#include "forms/home_form.h"
#include "forms/boot_form.h"
#include "forms/dashboard_form.h"
#include "forms/config_form.h"

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

static void formManagerOnNextFormEvent(EventParam_t arg) {
  g_ui16CurrentIndex = (g_ui16CurrentIndex + 1) % g_ui8FormCounter;

  g_psCurrentForm = g_psForms[g_ui16CurrentIndex];

  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void formManagerOnPrevFormEvent(EventParam_t arg) {
  if (g_ui8FormCounter == 0)
    return; // Protección de seguridad

  // Al sumar g_ui8FormCounter antes de restar 1, evitamos el underflow
  // matemático
  g_ui16CurrentIndex =
      (g_ui16CurrentIndex + g_ui8FormCounter - 1) % g_ui8FormCounter;

  g_psCurrentForm = g_psForms[g_ui16CurrentIndex];
  
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowHomeFormEvent(EventParam_t arg) {
	g_psCurrentForm = g_psForms[g_i16DashboardFormID];
  	g_bIsBackgroundReady = false;

    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowGraphFormEvent(EventParam_t arg) {
	g_psCurrentForm = g_psForms[g_i16GraphFormID];
  	g_bIsBackgroundReady = false;

    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowConfigFormEvent(EventParam_t arg) {
	g_psCurrentForm = g_psForms[g_i16ConfigFormID];
  	g_bIsBackgroundReady = false;

    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
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
  initCommonWidgets();
  
  //initHomeForm();
  initGraphForm();
  initBootForm();
  initDashboardForm();
  initConfigForm();
  Event_Subscribe(EVT_SYS_NEXT_FORM,
                  (EventHandler_fn)formManagerOnNextFormEvent);
  Event_Subscribe(EVT_SYS_PREV_FORM,
                  (EventHandler_fn)formManagerOnPrevFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_HOME_FORM, (EventHandler_fn)onShowHomeFormEvent);
  Event_Subscribe(EVT_CMD_SHOW_GRAPH_FORM, (EventHandler_fn)onShowGraphFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_CONFIG_FORM, (EventHandler_fn)onShowConfigFormEvent);

  g_psCurrentForm = g_psForms[g_i16BootFormID];
  // g_psCurrentForm = g_psForms[g_i16GraphFormID];
}

void formManagerComposite(pixel16_t *psPixelBuffer) {
  gfx_compositeFrame(g_psCurrentForm, psPixelBuffer);
}

void formManagerRenderEVEComponents(void) {
  //if(!g_bIsBackgroundReady) return;

  gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;
  // Lets check if the canvas contains a widget with EVE elements

  iter = g_psCurrentForm->psWidgets;
  // 1. Iniciamos una nueva Display List
  // API_LIB_BeginCoProList();
  // API_CMD_DLSTART();
  // API_CLEAR_COLOR_RGB(0, 0, 0);
  // API_CLEAR(1, 1, 1);
  // API_COLOR_RGB(255, 255, 255);

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
      		gfx_GraphRenderEVEComponents((gfx_Graph *)iter->sWidget.pvWidget);
			gf->bEVEDirty = false;

    		API_LIB_EndCoProList();
    		API_LIB_AwaitCoProEmpty();
    		API_LIB_BeginCoProList();
    } else if (iter->sWidget.eWidgetType == WD_TYPE_MULTIGRAPH) {
		gfx_MultiGraph *gf = (gfx_MultiGraph *)iter->sWidget.pvWidget;
		//if(gf->bEVEDirty) 
		{
      		gfx_MultigraphRenderEVEComponents(gf);
			gf->bEVEDirty = false;

    		API_LIB_EndCoProList();
    		API_LIB_AwaitCoProEmpty();
    		API_LIB_BeginCoProList();
		}
    } else if(iter->sWidget.eWidgetType == WD_TYPE_IMAGE) { 
		gfx_Image *img = (gfx_Image *)iter->sWidget.pvWidget;
		gfx_ImageRenderEVEComponents(img);

    	API_LIB_EndCoProList();
    	API_LIB_AwaitCoProEmpty();
    	API_LIB_BeginCoProList();
	}

    iter = iter->psNext;
  }

  // 3. Cerramos y hacemos el SWAP en el hardware
  // API_DISPLAY();
  // API_CMD_SWAP();
  // API_LIB_EndCoProList();

  // // Esperamos a que todo el frame (SWAP incluido) sea procesado
  // API_LIB_AwaitCoProEmpty();
}

// En form_manager.c
bool formManagerCheckSoftwareDirty(void) {
    bool bNeedsUpdate = false;
    gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;

    while (iter != NULL) {
        gfx_DirtyRect bbox = {0};

        // Delegamos la lógica al tipo de widget correspondiente
        switch (iter->sWidget.eWidgetType) {
            case WD_TYPE_BUTTON:
			    if(( (gfx_Button *)iter->sWidget.pvWidget )->bIsDirty) {
                	bbox = gfx_ButtonProcessState((gfx_Button *)iter->sWidget.pvWidget);
					gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x, bbox.y, bbox.w, bbox.h);
				}
                break;
            case WD_TYPE_LABEL: {
			    if(( (gfx_Label *)iter->sWidget.pvWidget )->bIsDirty) {
                	bbox = gfx_LabelProcessState((gfx_Label *)iter->sWidget.pvWidget);
					gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x, bbox.y, bbox.w, bbox.h);
				}
                break;
			}
			case WD_TYPE_SLIDER: {
				if(((gfx_Slider *)iter->sWidget.pvWidget)->bIsDirty) {
					bbox = gfx_SliderProcessState((gfx_Slider *)iter->sWidget.pvWidget);
					gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x, bbox.y, bbox.w, bbox.h);
				}
				break;
			}
			case WD_TYPE_GRAPH_OVERLAY: {
				if(((gfx_GraphOverlay *)iter->sWidget.pvWidget)->bIsDirty) {
					bbox = gfx_GraphOverlayProcessState((gfx_GraphOverlay *)iter->sWidget.pvWidget);
					gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x, bbox.y, bbox.w, bbox.h);
				}
				break;
			}
            // ... otros widgets
        }

        // Si el widget cambió, creamos los trabajos DMA solo para su área
        if (bbox.isDirty) {
            bNeedsUpdate = true;
            
            // Generar trabajos Scatter-Gather solo para el Bounding Box
			int row = 0;
            for (; row < bbox.h; row++) {
                DMARenderJob_t job;
                
                // Calcular el offset en SDRAM y RAM_G
                uint32_t pixelOffset = ((bbox.y + row) * LCD_WIDTH) + bbox.x;
                
                // Puntero de origen en SDRAM (g_pDrawingBuffer + offset)
                job.pSrcSDRAM = (uint8_t *)(g_pDrawingBuffer + pixelOffset);
                
                // Dirección destino en EVE G_RAM (0 + offset * 2 bytes por pixel RGB565)
                job.destRAMG = (pixelOffset * 2); 
                
                // Longitud a transferir (ancho del widget * 2)
                job.length = bbox.w * 2; 

                // Meter el trabajo a la cola de tu ISR
                Gfx_PushDMAJob(job); 
            }
			
			return bNeedsUpdate;
        }
        iter = iter->psNext;
    }

    return bNeedsUpdate;
}

bool formManagerCheckHardwareDirty(void) {
    bool bNeedsUpdate = false;
    gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;

    // Iteramos por todos los widgets de la pantalla actual
    while (iter != NULL) {
        
        // Evaluamos SOLO los widgets que se renderizan mediante EVE (Hardware)
        switch (iter->sWidget.eWidgetType) {
            
            case WD_TYPE_MULTIGRAPH: {
                gfx_MultiGraph *mgraph = (gfx_MultiGraph *)iter->sWidget.pvWidget;
                // Verificamos si alguna de sus banderas de actualización está activa
                if (mgraph->bIsDirty || mgraph->bEVEDirty) {
                    bNeedsUpdate = true;
                }
                break;
            }

            case WD_TYPE_GRAPH: {
                gfx_Graph *graph = (gfx_Graph *)iter->sWidget.pvWidget;
                if (graph->bIsDirty || graph->bEVEDirty) {
                    bNeedsUpdate = true;
                }
                break;
            }

            case WD_TYPE_IMAGE: {
                gfx_Image *img = (gfx_Image *)iter->sWidget.pvWidget;
                // Las imágenes decodificadas por hardware usan este flag si 
                // cambian de posición, escala, o si se ocultan/muestran.
                if (img->bIsDirty) {
                    bNeedsUpdate = true;
                }
                break;
            }

            // NOTA: Si en el futuro decides que los Sliders se rendericen 
            // 100% por EVE (usando CMD_SLIDER) en lugar de software, 
            // agregarías su case aquí.
        }

        // Si ya encontramos al menos un componente sucio, podríamos hacer un 'break'
        // para ahorrar ciclos de CPU (Fast Return), pero si necesitas que todos 
        // ejecuten alguna pre-lógica, lo dejamos iterar. Para máximo rendimiento:
        if (bNeedsUpdate) {
            break; 
        }

        iter = iter->psNext;
    }

    return bNeedsUpdate;
}
