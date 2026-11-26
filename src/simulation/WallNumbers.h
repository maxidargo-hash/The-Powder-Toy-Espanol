#include "SpanishText.h"
#ifndef WALLNUMBERS_H
#define WALLNUMBERS_H
#include <string>
#include <graphics/ARGBColour.h>

#define O_WL_WALLELEC     122
#define O_WL_EWALL        123
#define O_WL_DETECT       124
#define O_WL_STREAM       125
#define O_WL_SIGN         126
#define O_WL_FAN          127
#define O_WL_FANHELPER    255
#define O_WL_ALLOWLIQUID  128
#define O_WL_DESTROYALL   129
#define O_WL_ERASE        130
#define O_WL_WALL         131
#define O_WL_ALLOWAIR     132
#define O_WL_ALLOWSOLID   133
#define O_WL_ALLOWALLELEC 134
#define O_WL_EHOLE        135
#define O_WL_ALLOWGAS     140
#define O_WL_GRAV         142
#define O_WL_ALLOWENERGY  145
#define O_WL_ERASEALL     147

#define WL_ERASE        0
#define WL_WALLELEC     1
#define WL_EWALL        2
#define WL_DETECT       3
#define WL_STREAM       4
#define WL_FAN          5
#define WL_ALLOWLIQUID  6
#define WL_DESTROYALL   7
#define WL_WALL         8
#define WL_ALLOWAIR     9
#define WL_ALLOWPOWDER  10
#define WL_ALLOWALLELEC 11
#define WL_EHOLE        12
#define WL_ALLOWGAS     13
#define WL_GRAV         14
#define WL_ALLOWENERGY  15
#define WL_BLOCKAIR     16
#define WL_ERASEALL     17
#define WL_STASIS       18
#define WL_FANHELPER    255

#define WALLCOUNT 19

struct wallType
{
	std::string name;
	std::string identifier;
	ARGBColour colour;
	ARGBColour eglow; // if emap set, add this to fire glow
	int drawstyle;
	std::string descs;
};
typedef struct wallType wallType;

const wallType wallTypes[] =
{
	{"ERASE",           "DEFAULT_WL_ERASE", COLPACK(0x808080), COLPACK(0x000000), -1, "Erases walls."},
	{"MURO CONDUCTOR", "DEFAULT_WL_CNDTW", COLPACK(0xC0C0C0), COLPACK(0x101010), 0,  "Bloquea todo y conduce electricidad."},
	{"EWALL",           "DEFAULT_WL_EWALL", COLPACK(0x808080), COLPACK(0x808080), 0,  TPT_ES("Muro eléctrico. Deja pasar partículas cuando recibe corriente.")},
	{"DETECTOR",        "DEFAULT_WL_DTECT", COLPACK(0xFF8080), COLPACK(0xFF2008), 1,  TPT_ES("Detector. Genera electricidad cuando hay una partícula dentro.")},
	{"STREAMLINE",      "DEFAULT_WL_STRM",  COLPACK(0x808080), COLPACK(0x000000), 0,  TPT_ES("Dibuja una línea que sigue la corriente del aire.")},
#ifndef TOUCHUI
	{"FAN",             "DEFAULT_WL_FAN",   COLPACK(0x8080FF), COLPACK(0x000000), 1,  TPT_ES("Acelera el aire. Usa la herramienta de línea para ajustar la dirección y la fuerza.")},
#else
	{"FAN",             "DEFAULT_WL_FAN",   COLPACK(0x8080FF), COLPACK(0x000000), 1,  "Fan. Accelerates air. Fan acceleration cannot be set on Android."},
#endif
	{"LIQUID WALL",     "DEFAULT_WL_LIQD",  COLPACK(0xC0C0C0), COLPACK(0x101010), 2,  TPT_ES("Deja pasar líquidos y bloquea las demás partículas. Conduce electricidad.")},
	{"ABSORB WALL",     "DEFAULT_WL_ABSRB", COLPACK(0x808080), COLPACK(0x000000), 1,  TPT_ES("Absorbe partículas y deja pasar el aire.")},
	{"WALL",            "DEFAULT_WL_WALL",  COLPACK(0x808080), COLPACK(0x000000), 3,  TPT_ES("Muro básico. Bloquea todo.")},
	{"AIRONLY WALL",    "DEFAULT_WL_AIR",   COLPACK(0x3C3C3C), COLPACK(0x000000), 1,  TPT_ES("Deja pasar el aire y bloquea todas las partículas.")},
	{"POWDER WALL",     "DEFAULT_WL_POWDR", COLPACK(0x575757), COLPACK(0x000000), 1,  TPT_ES("Deja pasar polvos y bloquea las demás partículas.")},
	{"CONDUCTOR",       "DEFAULT_WL_CNDTR", COLPACK(0xFFFF22), COLPACK(0x101010), 2,  TPT_ES("Deja pasar todas las partículas y conduce electricidad.")},
	{"EHOLE",           "DEFAULT_WL_EHOLE", COLPACK(0x242424), COLPACK(0x101010), 0,  TPT_ES("Absorbe partículas y las libera al recibir corriente.")},
	{"GAS WALL",        "DEFAULT_WL_GAS",   COLPACK(0x579777), COLPACK(0x000000), 1,  TPT_ES("Deja pasar gases y bloquea las demás partículas.")},
	{"GRAVITY WALL",    "DEFAULT_WL_GRVTY", COLPACK(0xFFEE00), COLPACK(0xAA9900), 4,  TPT_ES("Muro gravitatorio. Anula la gravedad newtoniana dentro de un recinto hecho con él.")},
	{"ENERGY WALL",     "DEFAULT_WL_ENRGY", COLPACK(0xFFAA00), COLPACK(0xAA5500), 4,  TPT_ES("Deja pasar partículas de energía y bloquea las demás.")},
	{"AIRBLOCK WALL",   "DEFAULT_WL_NOAIR", COLPACK(0xDCDCDC), COLPACK(0x000000), 1,  TPT_ES("Deja pasar todas las partículas y bloquea el aire.")},
	{"ERASEALL",        "DEFAULT_WL_ERASEA",COLPACK(0x808080), COLPACK(0x000000), -1, TPT_ES("Borra muros, partículas y letreros.")},
	{"STASIS WALL",     "DEFAULT_WL_STASIS",COLPACK(0x800080), COLPACK(0x000000), 0,  "Freezes particles inside the wall in place until powered"},
};

#endif
