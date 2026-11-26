#include "SpanishText.h"

#ifdef TOUCHUI
static constexpr char introTextUtf8[] =
	"\bl\bUThe Powder Toy Español\bU - Versión " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\n"
	"Versión Android " MTOS(MOBILE_MAJOR) "." MTOS(MOBILE_MINOR) " compilación " MTOS(MOBILE_BUILD) "\n"
	"\n"
	"\bgToca y arrastra por los iconos de la derecha para cambiar de categoría.\n"
	"Si la fila de elementos es larga, arrástrala para desplazarla.\n"
	"Usa los botones de volumen para cambiar el tamaño del pincel.\n"
	"Para abrir partidas en línea, toca el botón de la esquina inferior izquierda.\n"
	"El botón inferior derecho pausa y reanuda la simulación.\n\n"
	"\boBotones de la derecha, de arriba abajo:\n"
	"\bo   Activa el borrador. Mantén pulsado para limpiar la pantalla.\n"
	"\bo   Abre la consola. Mantén pulsado para mostrar el teclado.\n"
	"\bo   Alterna la gravedad newtoniana o la decoración. Mantén pulsado para ver ajustes.\n"
	"\bo   Coloca o quita la ventana de zoom.\n"
	"\bo   Guarda un sello. Mantén pulsado para cargar uno.\n\n"
	"\boPara colocar un sello:\n"
	"\bo   Arrástralo hasta donde quieras.\n"
	"\bo   Tócalo sin moverlo para colocarlo.\n"
	"\bo   Toca fuera del sello para moverlo un píxel.\n"
	"\bo   Toca el borde del sello y arrastra en círculo para girarlo.\n"
	"\n"
	"Colaboradores: \bgStanislaw K Skowronek (diseñó el Powder Toy original),\n"
	"\bgSimon Robertshaw, Skresanov Savely, Pilihp64, Catelite, Victoria Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"\n"
#ifndef BETA
	"\bgPara usar funciones en línea, como guardar partidas, regístrate en: \brhttps://powdertoy.co.uk/Register.html\n"
#else
	"\brEsta versión beta no permite publicar partidas. Usa la versión estable para publicarlas.\n"
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM) " "
#else
static constexpr char introTextUtf8[] =
	"\bl\bUThe Powder Toy Español\bU - Versión " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\n"
	"\brJ\bla\boc\bgo\btb\bb1\bp'\bws \bbMod version " MTOS(MOD_VERSION) "." MTOS(MOD_MINOR_VERSION) " compilación " MTOS(MOD_BUILD_VERSION) "\bg   Codebase based on C version 83.0\n"
	"\n"
	"\bgControl+C/V/X are Copy, Paste and cut respectively.\n"
	"\bgPasa el cursor por los iconos de la derecha para ver los elementos de cada categoría.\n"
	"\bgPick your material from the menu using mouse left/right buttons.\n"
	"Mantén pulsado un botón del ratón y arrastra para dibujar.\n"
	"Shift+drag will create straight lines of particles.\n"
	"Ctrl+drag will result in filled rectangles.\n"
	"Ctrl+Shift+click will flood-fill a closed area.\n"
	"Use the mouse scroll wheel, or '[' and ']', to change the tool size for particles.\n"
	"Middle click or Alt+Click to \"sample\" the particles.\n"
	"Ctrl+Z will act as Undo.\n"
	"\n\boUse 'Z' for a zoom tool. Click to make the drawable zoom window stay around. Use the wheel to change the zoom strength\n"
	"The spacebar can be used to pause and unpause physics.\n"
	"Use 'S' to save parts of the window as 'stamps'.\n"
	"'L' will load the most recent stamp, 'K' shows a library of stamps you saved.\n"
	"The numbers on the keyboard will change the display mode\n"
	"\n"
	"Colaboradores: \bgStanislaw K Skowronek (diseñó el Powder Toy original),\n"
	"\bgSimon Robertshaw, Skresanov Savely, Pilihp64, Catelite, Victoria Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"Thanks to Pilihp64 for the update server for my mod\n"
	"\n"
#ifndef BETA
	"\bgPara usar funciones en línea, como guardar partidas, regístrate en: \brhttps://powdertoy.co.uk/Register.html\n"
#else
	"\brEsta versión beta no permite publicar partidas. Usa la versión estable para publicarlas.\n"
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM)
#endif

#ifdef BETA
	" BETA"
#endif
#ifdef X86
	" X86"
#endif
#ifdef X86_SSE
	" X86_SSE"
#endif
#ifdef X86_SSE2
	" X86_SSE2"
#endif
#ifdef X86_SSE3
	" X86_SSE3"
#endif
#ifdef LIN
#ifdef _64BIT
	" LIN64"
#else
	" LIN32"
#endif
#endif
#ifdef WIN
#ifdef _64BIT
	" WIN64"
#else
	" WIN32"
#endif
#endif
#ifdef MACOSX
	" MACOSX"
#endif
#ifdef ANDROID
	" ANDROID"
#endif
#ifdef LUACONSOLE
	" LUACONSOLE"
#endif
#ifdef GRAVFFT
	" GRAVFFT"
#endif
    ;

static constexpr auto introTextEncoded = EncodeSpanishText(introTextUtf8);
const char *const introText = introTextEncoded.data();
