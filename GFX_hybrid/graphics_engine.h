#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// #include "gfx.h"

typedef union
{
	uint16_t u16;
	uint8_t u8[2];
} pixel16_t;

typedef struct
{
	uint16_t ui16Width;
	uint16_t ui16Height;
	uint32_t ui32BuffSize;
	pixel16_t *psPixelBuffer; 
} GfxLayer_t;

extern pixel16_t *g_pDrawingBuffer;
extern pixel16_t *g_pSendingBuffer;

extern uint32_t g_ui32StartTxCycles;
extern uint32_t g_ui32EndTxCycles;

extern bool g_bIsBackgroundReady;

// --- DIRTY RECTANGLE QUEUE STRUCTURES ---
typedef struct {
    uint8_t *pSrcSDRAM;  
    uint32_t destRAMG; 
    uint16_t length;   
} DMARenderJob_t;

#define MAX_DMA_JOBS 1024

typedef struct {
    DMARenderJob_t jobs[MAX_DMA_JOBS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} DMAJobQueue_t;

typedef enum {
    RENDER_IDLE = 0,
    RENDER_WAIT_DMA,
	RENDER_EVE_COMPONENTS,
	RENDER_FINISHED,
} RenderState_e;

typedef struct {
    RenderState_e state;
} RenderEngine_t;

extern volatile DMAJobQueue_t g_DMAQueue;
extern volatile RenderEngine_t g_RenderEngine;

extern bool g_bIsJobTransfer; 

void Helper_FloatToString(char *buffer, uint32_t whole, uint32_t frac, bool bAddEnd);

bool Gfx_initEngine(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight);

void Gfx_loadIntoBuffer(uint32_t ui32Index, uint16_t ui16Pixel);

void Gfx_composite(pixel16_t *pLayer0, pixel16_t *pLayer1);


inline void Gfx_RestoreBackground_Fast(pixel16_t *pCleanBackground, pixel16_t *pDrawBuffer, 
                                              int16_t x, int16_t y, int16_t width, int16_t height);

void Gfx_Start_SG_Transfer(bool bIsBlocking);

void Gfx_BuildDynamicSG(uint8_t *pSrc, uint32_t totalBytes);

bool Gfx_BuildSg_For_Segments(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h);

void Gfx_SendContinuousBlock_SG(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h);

void Gfx_displayBitmap(void);

void Gfx_renderTask(void);

void Gfx_sendFullFrame(bool bIsBlocking);

bool Gfx_PushDMAJob(DMARenderJob_t job);

void Gfx_showFullFrame();


#endif


