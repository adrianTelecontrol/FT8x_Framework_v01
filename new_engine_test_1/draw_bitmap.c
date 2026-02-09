/**
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "image_loader.h"
// #include "FT8xx_params.h"

#include "draw_bitmap.h"

#ifndef RENDER_COLORS
#define C_WHITE 255, 255, 255, 255

#define C_PURPLE 0x701F // Original: 0xFFFF0070 (R=112, G=0,   B=255)
#define C_RED 0xF800    // Original: 0xFF0000FF (R=255, G=0,   B=0)
#define C_GREEN 0x07E0  // Original: 0xFF00FF00 (R=0,   G=255, B=0)
#define C_BLUE 0x001F   // Original: 0xFFFF0000 (R=0,   G=0,   B=255)
#define C_YELLOW 0xFFE0 // Original: 0xFF00FFFF (R=255, G=255, B=0)
#define C_CYAN 0x07FF   // Original: 0xFFFFFF00 (R=0,   G=255, B=255)
#define C_ORANGE 0xFD20 // Original: 0xFF00A5FF (R=255, G=165, B=0)
#define C_PINK 0xCE1F   // Original: 0xFFFFC0CB (R=203, G=192, B=255)
#define C_LIME 0x87E0   // Original: 0xFF00FF80 (R=128, G=255, B=0)
#define C_SKY_BLUE 0x867D

#define SCREEN_WIDTH 800 / 4
#define SCREEN_HEIGHT 480 / 4
#endif

typedef uint32_t u32;
typedef float f32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct {
  f32 x;
  f32 y;
} vect2d;

typedef struct {
  u8 *pixels;
  u16 width;
  u16 height;
} Scene_t;

typedef struct {
  u16 id;
  u16 width;
  u16 height;
  f32 angle;
  f32 scale;
  u16 color;
  u32 mass;
  f32 angularVel;
  f32 inertia;
  f32 boundingBox;
  vect2d vel;
  vect2d pos;
} SquareObject;

static SquareObject squares[10];
const u16 SQUARE_OBJ_LEN = sizeof(squares) / sizeof(SquareObject);

const u16 COLORS[] = {C_PURPLE, C_RED,    C_GREEN, C_BLUE, C_YELLOW,
                      C_CYAN,   C_ORANGE, C_PINK,  C_LIME, C_SKY_BLUE};
const u16 COLORS_LEN = sizeof(COLORS) / sizeof(u16);

static Scene_t scene;

void initializeSquareObjects(void) {
  u16 i = 0;
  for (i = 0; i < SQUARE_OBJ_LEN; i++) {
    SquareObject *sqr = &squares[i];
    // sqr->width = SCREEN_HEIGHT / 6.0f;
    // sqr->height = SCREEN_HEIGHT / 6.0f;
    u16 ui16SideSize = (u16)(rand() % (int)((SCREEN_HEIGHT / 4.0f) -
                                            (SCREEN_HEIGHT / 7.0f) + 1.0f) +
                             (SCREEN_HEIGHT / 7.0f));
    sqr->width = ui16SideSize;
    sqr->height = ui16SideSize;
    sqr->scale = 1.0f;
    sqr->boundingBox =
        sqr->scale *
        (sqrtf(sqr->width * sqr->width + sqr->height * sqr->height) / 2.0f);

    // Margin is the radius. We must spawn between [margin] and [Limit -
    // margin]
    int margin = (int)sqr->boundingBox + 1;
    int safeWidth = SCREEN_WIDTH - (2 * margin);
    int safeHeight = SCREEN_HEIGHT - (2 * margin);

    // If the square is too big for the screen, center it
    sqr->pos.x = (safeWidth > 0) ? (margin + (rand() % safeWidth))
                                 : (SCREEN_WIDTH / 2.0f);
    sqr->pos.y = (safeHeight > 0) ? (margin + (rand() % safeHeight))
                                  : (SCREEN_HEIGHT / 2.0f);

    sqr->vel.x = (f32)((rand() % 20) - 10);
    sqr->vel.y = (f32)((rand() % 20) - 10);
    sqr->angle = (f32)(rand() % 360);
    sqr->color = COLORS[rand() % COLORS_LEN];
    sqr->mass = ( sqr->scale * sqr->width * sqr->height ) / 10;
    sqr->inertia = (1.0f / 6.0f) * sqr->mass * (ui16SideSize * ui16SideSize);
    //sqr->angularVel = 1;
    sqr->angularVel = rand() % 10;
  }
}

void inverseMapping(SquareObject *obj) {
  f32 rad = -obj->angle * (3.14159265f / 180.0f);
  f32 cosA = cosf(rad);
  f32 sinA = sinf(rad);

  f32 stepXi = cosA / obj->scale;
  f32 stepYi = sinA / obj->scale;
  f32 stepXj = -sinA / obj->scale;
  f32 stepYj = cosA / obj->scale;

  f32 hw = obj->width / 2.0f;
  f32 hh = obj->height / 2.0f;

  // 1. Calculate boundaries as SIGNED integers first
  int sI = (int)(obj->pos.x - obj->boundingBox);
  int sJ = (int)(obj->pos.y - obj->boundingBox);
  int eI = (int)(obj->pos.x + obj->boundingBox);
  int eJ = (int)(obj->pos.y + obj->boundingBox);

  // 2. CLAMP boundaries to screen dimensions (This prevents the crash!)
  if (sI < 0)
    sI = 0;
  if (sJ < 0)
    sJ = 0;
  if (eI > SCREEN_WIDTH)
    eI = SCREEN_WIDTH;
  if (eJ > SCREEN_HEIGHT)
    eJ = SCREEN_HEIGHT;

  // 3. Calculate initial offsets based on the CLAMPED start points
  f32 start_dx = (f32)sI - obj->pos.x;
  f32 start_dy = (f32)sJ - obj->pos.y;

  f32 rowX = (start_dx * cosA - start_dy * sinA) / obj->scale;
  f32 rowY = (start_dx * sinA + start_dy * cosA) / obj->scale;

  int j, i;
  for (j = sJ; j < eJ; j++) {
    f32 x = rowX;
    f32 y = rowY;

    // Optimization: Calculate Row Offset once per row
    u32 rowOffset = j * SCREEN_WIDTH * 2;

    for (i = sI; i < eI; i++) {
      if (x >= -hw && x <= hw && y >= -hh && y <= hh) {
        // FIX: Multiply index by 2 for RGB565
        u32 pixelIndex = rowOffset + (i * 2);

        scene.pixels[pixelIndex] = (u8)((obj->color >> 8) & 0xFF);
        scene.pixels[pixelIndex + 1] = (u8)(obj->color & 0xFF);
      }
      x += stepXi;
      y += stepYi;
    }
    rowX += stepXj;
    rowY += stepYj;
  }
}

void basicObjectCollision(void) {
  u16 i = 0;
  for (i = 0; i < SQUARE_OBJ_LEN; i++) {
    SquareObject *a = &squares[i];

    // 1. Update Position based on velocity
    // (Dividing by 6.0f as you did to control speed)
    a->pos.x += a->vel.x / 6.0f;
    a->pos.y += a->vel.y / 6.0f;
    a->angle += a->angularVel / 6.0f;

    // 2. Screen Boundary Collision (with Position Snapping)
    // Horizontal Walls
    if (a->pos.x < a->boundingBox) {
      a->pos.x = a->boundingBox;
      a->vel.x *= -1.0f;
    } else if (a->pos.x > SCREEN_WIDTH - a->boundingBox) {
      a->pos.x = SCREEN_WIDTH - a->boundingBox;
      a->vel.x *= -1.0f;
    }

    // Vertical Walls
    if (a->pos.y < a->boundingBox) {
      a->pos.y = a->boundingBox;
      a->vel.y *= -1.0f;
    } else if (a->pos.y > SCREEN_HEIGHT - a->boundingBox) {
      a->pos.y = SCREEN_HEIGHT - a->boundingBox;
      a->vel.y *= -1.0f;
    }

    // 3. Object-to-Object Collision Loop
    u16 j = 0;
    for (j = i + 1; j < SQUARE_OBJ_LEN; j++) {
      SquareObject *b = &squares[j];

      f32 dx = b->pos.x - a->pos.x;
      f32 dy = b->pos.y - a->pos.y;

      // Optimization: Check squared distance first to avoid unnecessary
      // sqrtf
      f32 distanceSq = (dx * dx) + (dy * dy);
      f32 minDistance = a->boundingBox + b->boundingBox;
      f32 minDistanceSq = minDistance * minDistance;

      if (distanceSq < minDistanceSq) {
        f32 distance = sqrtf(distanceSq);
        if (distance == 0)
          distance = 0.1f; // Prevent division by zero

        // Calculate Normal Vector
        f32 nx = dx / distance;
        f32 ny = dy / distance;

        // --- PHASE 1: Static Resolution (Push them apart) ---
        f32 overlap = minDistance - distance;
        f32 totalMass = a->mass + b->mass;
        f32 ratioA = (f32)b->mass / totalMass;
        f32 ratioB = (f32)a->mass / totalMass;
        a->pos.x -= nx * (overlap * ratioA);
        a->pos.y -= ny * (overlap * ratioA);
        b->pos.x += nx * (overlap * ratioB);
        b->pos.y += ny * (overlap * ratioB);
        // a->pos.x -= nx * (overlap / 2.0f);
        // a->pos.y -= ny * (overlap / 2.0f);
        // b->pos.x += nx * (overlap / 2.0f);
        // b->pos.y += ny * (overlap / 2.0f);

        // For angular impulse
        f32 rax = nx * a->boundingBox;
        f32 ray = ny * a->boundingBox;
        f32 rbx = -nx * b->boundingBox;
        f32 rby = -ny * b->boundingBox;

        f32 Ran = (rax * ny) - (ray * nx);
        f32 Rbn = (rbx * ny) - (rby * nx);
        // --- PHASE 2: Dynamic Resolution (The Impulse/Bounce) ---
        // Calculate relative velocity along the normal
        f32 angVelA_rad = a->angularVel * (3.14159265f / 180.0f);
        f32 angVelB_rad = b->angularVel * (3.14159265f / 180.0f);

        // Velocity of the contact point on Square A
        f32 vax = a->vel.x + (-angVelA_rad * ray);
        f32 vay = a->vel.y + (angVelA_rad * rax);

        // Velocity of the contact point on Square B
        f32 vbx = b->vel.x + (-angVelB_rad * rby);
        f32 vby = b->vel.y + (angVelB_rad * rbx);

        // Relative Velocity at the contact point
        f32 relVelX = vax - vbx;
        f32 relVelY = vay - vby;
        // f32 relVelX = a->vel.x - b->vel.x;
        // f32 relVelY = a->vel.y - b->vel.y;
        f32 dotProduct = (relVelX * nx) + (relVelY * ny);

        // Only resolve if they are actually moving towards each other
        if (dotProduct > 0) {
          // Added the concept of impulse
          f32 e = 1.0f;

          // f32 f32VImpact_a = a->vel + (a->angularVel * );
          // f32 f32VImpact_b = b->vel + (b->angularVel * );

          f32 j = -(1.0f + e) * dotProduct;
          j /= (1.0f / a->mass + 1.0f / b->mass + (Ran * Ran) / a->inertia +
                (Rbn * Rbn) / b->inertia);

          a->vel.x += (j / a->mass) * nx;
          a->vel.y += (j / a->mass) * ny;
          b->vel.x -= (j / b->mass) * nx;
          b->vel.y -= (j / b->mass) * ny;
          // a->angularVel += (Ran * j) / a->inertia;
          // b->angularVel += (Rbn * j) / b->inertia;
          a->angularVel += ((Ran * j) / a->inertia) * (180.0f / 3.14159265f);
          b->angularVel -= ((Rbn * j) / b->inertia) *
                           (180.0f / 3.14159265f); // Note the minus!
          // a->vel.x += (j / a->mass) * nx;
          // a->vel.y += (j / a->mass) * ny;
          // b->vel.x -= (j / b->mass) * nx;
          // b->vel.y -= (j / b->mass) * ny;
          // a->vel.x -= dotProduct * nx;
          // a->vel.y -= dotProduct * ny;
          // b->vel.x += dotProduct * nx;
          // b->vel.y += dotProduct * ny;
        }
      }
    }
  }
}

void TEST_collideSquares(void) {
  // srand();
  scene.pixels = (u8 *)malloc(sizeof(u16) * SCREEN_WIDTH * SCREEN_HEIGHT);
  // scene.pixels = (u8 *)0x60000000;
  scene.width = SCREEN_WIDTH;
  scene.height = SCREEN_HEIGHT;

  BitmapHandler_t sBmpHandler;
  sBmpHandler.ui8Pixels = scene.pixels;
  sBmpHandler.sHeader.bitmap_height = SCREEN_HEIGHT;
  sBmpHandler.sHeader.bitmap_width = SCREEN_WIDTH;

  initializeSquareObjects();

  while (true) {
    basicObjectCollision();
    memset(scene.pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 2);

    u16 i = 0;
    for (i = 0; i < SQUARE_OBJ_LEN; i++) {
      inverseMapping(&squares[i]);
    }

    u8 ui8Flags = 0;
    // ui8Flags |= EVE_LOAD_IMG_POLLING;
    ui8Flags |= EVE_LOAD_IMG_UDMA;
    EVE_LoadBitmap(&sBmpHandler, ui8Flags);
  }
}
