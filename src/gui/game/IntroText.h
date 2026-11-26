#pragma once
#include "Config.h"
#include "SimulationConfig.h"
#include "common/String.h"
#include <cstring>

inline ByteString VersionInfo()
{
	ByteStringBuilder sb;
	sb << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1];
	if constexpr (!SNAPSHOT)
	{
		sb << "." << APP_VERSION.build;
	}
	sb << " " << IDENT;
	if constexpr (MOD)
	{
		sb << " MOD " << MOD_ID << " ORIGINAL " << UPSTREAM_VERSION.build;
	}
	if constexpr (SNAPSHOT)
	{
		sb << " PRELIMINAR " << APP_VERSION.build;
	}
	if constexpr (LUACONSOLE)
	{
		sb << " LUACONSOLE";
	}
	if constexpr (NOHTTP)
	{
		sb << " NOHTTP";
	}
	else if constexpr (ENFORCE_HTTPS)
	{
		sb << " HTTPS";
	}
	if constexpr (DEBUG)
	{
		sb << " DEBUG";
	}
	return sb.Build();
}

inline ByteString IntroText()
{
	ByteStringBuilder sb;
	sb << "\bl\bU" << APPNAME << "\bU - Versión " << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1] << "\n"
	      "https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	      "\n";
	if constexpr (DEFAULT_TOUCH_UI)
	{
		sb << "\bgToca una categoría a la derecha y elige un elemento de la fila inferior.\n"
		      "Toca el área de simulación y arrastra el dedo para dibujar.\n"
		      "\n"
		      "Usa la lupa para buscar elementos por nombre o descripción.\n"
		      "Usa la barra inferior para guardar, recargar o pausar la simulación.\n"
		      "En Configuración puedes cambiar la gravedad, el aire y otras opciones.\n"
		      "\n";
	}
	else
	{
		sb << "\bgPulsa \boF1\bg para mostrar u ocultar la ayuda.\n"
		      "\n"
		      "\bgPasa el cursor por los iconos de la derecha para ver los elementos de cada categoría.\n"
		      "Elige un material con el \bobotón izquierdo o derecho\bg del ratón.\n"
		      "Mantén pulsado un botón del ratón y arrastra para dibujar.\n"
		      "\n"
		      "Usa la \borueda del ratón\bg o \bo[\bg y \bo]\bg para cambiar el tamaño. Pulsa \boTab\bg para cambiar el pincel.\n"
		      "Usa el \bobotón central\bg o \boAlt+clic\bg para tomar una muestra.\n"
		      "\boCtrl+C/V/X\bg permite copiar, pegar y cortar.\n"
		      "Al pegar, pulsa \boR\bg para girar o \boMayús+R\bg y \boMayús+Ctrl+R\bg para voltear en vertical u horizontal.\n"
		      "\boMayús+arrastrar\bg traza líneas. Añade \boAlt\bg para ajustarlas a la horizontal, vertical o diagonal.\n"
		      "\boCtrl+arrastrar\bg crea rectángulos rellenos. Añade \boAlt\bg para cuadrados. \boCtrl+Mayús+clic\bg rellena un área cerrada.\n"
		      "\n"
		      "\boEspacio\bg pausa la simulación, \boF\bg avanza un fotograma y \boF5\bg recarga la partida.\n"
		      "\boCtrl+Z\bg deshace y \boCtrl+Y\bg o \boCtrl+Mayús+Z\bg rehace.\n"
		      "\boS\bg crea un sello de un área, \boL\bg carga el último sello y \boK\bg abre la biblioteca de sellos.\n"
		      "\n"
		      "Usa \bo0-9\bg para cambiar el modo de visualización.\n"
		      "\boH\bg muestra u oculta el HUD y \boD\bg activa o desactiva su modo de depuración.\n"
		      "\boZ\bg activa el zoom. Haz clic para fijar su posición y usa la rueda para ajustar la ampliación.\n"
		      "\boCtrl+F\bg resalta el elemento seleccionado.\n"
		      "\n";
	}
	if constexpr (BETA)
	{
		sb << "\brEsta versión beta no permite publicar partidas. Sus partidas y sellos no se pueden abrir en versiones anteriores.\n"
		      "\brUsa la versión estable si quieres publicar partidas.\n";
	}
	else
	{
		sb << "\bgPara guardar en línea y usar otras funciones en red, regístrate en: \br" << SERVER << "/Register.html\n";
	}
	if constexpr (std::string_view(IDENT_PLATFORM) == "EMSCRIPTEN")
	{
		sb << "\brEl navegador guarda tus partidas locales y podría borrarlas. \bgSi tienes dudas, guarda en línea.\n";
	}
	sb << "\n\bt" << VersionInfo();
	return sb.Build();
}
