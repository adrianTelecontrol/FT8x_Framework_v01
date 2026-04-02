
#include <stddef.h>

#include "event_engine.h"
#include "gfx_theme.h"
#include "gfx_colors.h"

gfx_Theme_t *g_pCurrentTheme = NULL;

// ---------------------------------------------------------
// THEME 1: INDUSTRIAL LIGHT MODE
// ---------------------------------------------------------
gfx_Theme_t g_ThemeLight = {
    .palette.background = COLOR_WHITE,
    .palette.surface    = COLOR_GRAY_50,
    .palette.primary    = COLOR_BLUE_PRIMARY,
    .palette.secondary  = COLOR_GRAY_200,
    .palette.textMain   = COLOR_BLACK,
    .palette.textMuted  = COLOR_GRAY_500,
    .palette.border     = COLOR_GRAY_400,
    .palette.danger     = COLOR_RED_ALERT,
    .palette.success    = COLOR_GREEN_OK
};

// ---------------------------------------------------------
// THEME 2: INDUSTRIAL DARK MODE
// ---------------------------------------------------------
gfx_Theme_t g_ThemeDark = {
    .palette.background = COLOR_GRAY_900,
    .palette.surface    = COLOR_GRAY_800,
    .palette.primary    = COLOR_CYAN_NEON,
    .palette.secondary  = COLOR_GRAY_700,
    .palette.textMain   = COLOR_WHITE,
    .palette.textMuted  = COLOR_GRAY_550,
    .palette.border     = COLOR_GRAY_600,
    .palette.danger     = COLOR_RED_MUTED,
    .palette.success    = COLOR_GREEN_MUTED
};

gfx_Theme_t g_ThemeSoftLight = {
    .palette.background = COLOR_OFFWHITE_BLUE, // Fondo que descansa la vista
    .palette.surface    = COLOR_WHITE,         // Los paneles (como tu bgPanel) flotan en blanco puro
    .palette.primary    = COLOR_TEAL_PRIMARY,  // Botones de acción principal
    .palette.secondary  = COLOR_BLUE_GRAY_200, // Botones secundarios
    .palette.textMain   = COLOR_BLUE_GRAY_900, // Texto oscuro (mejor contraste que negro puro)
    .palette.textMuted  = COLOR_BLUE_GRAY_600, // Texto secundario
    .palette.border     = COLOR_BLUE_GRAY_400, // Bordes sutiles
    .palette.danger     = COLOR_SOFT_RED,      // Alertas
    .palette.success    = COLOR_SOFT_GREEN     // Estados OK
};

static void onChangeThemeEvent(uint32_t arg) {
	gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

	if(g_pCurrentTheme == &g_ThemeDark) {
		g_pCurrentTheme = &g_ThemeLight;
	} else {
		g_pCurrentTheme = &g_ThemeDark;
	}

    Theme_SetFontFamily(activeFamily);

	Event_Post(EVT_CMD_FULL_REPAINT, 0);
}

// ---------------------------------------------------------
// INITIALIZATION
// ---------------------------------------------------------
void Theme_Init(void) {
    g_pCurrentTheme = &g_ThemeLight; // Tema por defecto
    
    // Cargar la familia de fuentes por defecto al arrancar
    Theme_SetFontFamily(FONT_FAM_INTER);

	Event_Subscribe(EVT_CMD_CHANGE_THEME, onChangeThemeEvent);
}

void Theme_SetMode(bool isDark) {
    gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

    if (isDark) {
        g_pCurrentTheme = &g_ThemeDark;
    } else {
        g_pCurrentTheme = &g_ThemeLight;
    }

    // Transferir la familia activa al nuevo tema y recargar los punteros
    Theme_SetFontFamily(activeFamily);
}

void Theme_SetFontFamily(gfx_FontFamily_e newFamily) {
    if (!g_pCurrentTheme) return;

    g_pCurrentTheme->fonts.currentFamily = newFamily;

    // Cargar dinámicamente desde la SD y guardar los IDs en el tema
    g_pCurrentTheme->fonts.h1      = gfx_fontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, FONT_SIZE_48);
    g_pCurrentTheme->fonts.h2      = gfx_fontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, FONT_SIZE_32);
    g_pCurrentTheme->fonts.body    = gfx_fontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_24);
    g_pCurrentTheme->fonts.caption = gfx_fontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_18);
    
    // Nota: La fuente mono suele mantenerse constante sin importar el tema general
    // para asegurar que los números no salten.
    g_pCurrentTheme->fonts.mono    = gfx_fontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_24); 
}

