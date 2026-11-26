#include "RenderView.h"
#include "simulation/ElementGraphics.h"
#include "simulation/SimulationData.h"
#include "simulation/Simulation.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/VideoBuffer.h"
#include "RenderController.h"
#include "RenderModel.h"
#include "gui/interface/Checkbox.h"
#include "gui/interface/Button.h"
#include "gui/game/GameController.h"
#include "gui/game/GameView.h"

class ModeCheckbox : public ui::Checkbox
{
public:
	using ui::Checkbox::Checkbox;
	uint32_t mode;
};

RenderView::RenderView():
	ui::Window(ui::Point(0, 0), ui::Point(XRES, WINDOWH)),
	ren(nullptr),
	toolTip(""),
	isToolTipFadingIn(false)
{
	auto addPresetButton = [this](int index, Icon icon, ui::Point offset, String tooltip) {
		auto *presetButton = new ui::Button(ui::Point(XRES, YRES) + offset, ui::Point(30, 13), "", tooltip);
		presetButton->SetIcon(icon);
		presetButton->SetActionCallback({ [this, index] { c->LoadRenderPreset(index); } });
		AddComponent(presetButton);
	};
	addPresetButton( 1, IconVelocity  , ui::Point( -37,  6), "Modo de visualización de velocidad");
	addPresetButton( 2, IconPressure  , ui::Point( -37, 24), "Modo de visualización de presión");
	addPresetButton( 3, IconPersistant, ui::Point( -76,  6), "Modo de rastros persistentes");
	addPresetButton( 4, IconFire      , ui::Point( -76, 24), "Modo de visualización de fuego");
	addPresetButton( 5, IconBlob      , ui::Point(-115,  6), "Modo de partículas redondeadas");
	addPresetButton( 6, IconHeat      , ui::Point(-115, 24), "Modo de visualización de calor");
	addPresetButton( 7, IconBlur      , ui::Point(-154,  6), "Modo de efectos avanzados");
	addPresetButton( 8, IconBasic     , ui::Point(-154, 24), "Modo de visualización simple");
	addPresetButton( 9, IconGradient  , ui::Point(-193,  6), "Modo de gradiente térmico");
	addPresetButton( 0, IconAltAir    , ui::Point(-193, 24), "Modo alternativo de velocidad");
	addPresetButton(10, IconLife      , ui::Point(-232,  6), "Modo de visualización de vida");

	auto addRenderModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *renderModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		renderModes.push_back(renderModeCheckbox);
		renderModeCheckbox->mode = mode;
		renderModeCheckbox->SetIcon(icon);
		renderModeCheckbox->SetActionCallback({ [this] {
			auto renderMode = CalculateRenderMode();
			c->SetRenderMode(renderMode);
		} });
		AddComponent(renderModeCheckbox);
	};
	addRenderModeCheckbox(RENDER_EFFE, IconEffect, ui::Point( 1,  4), "Añade destellos especiales a algunos elementos");
	addRenderModeCheckbox(RENDER_FIRE, IconFire  , ui::Point( 1, 22), "Añade un efecto de fuego a los gases");
	addRenderModeCheckbox(RENDER_GLOW, IconGlow  , ui::Point(33,  4), "Añade brillo a algunos elementos");
	addRenderModeCheckbox(RENDER_BLUR, IconBlur  , ui::Point(33, 22), "Difumina los líquidos");
	addRenderModeCheckbox(RENDER_BLOB, IconBlob  , ui::Point(65,  4), "Dibuja todas las partículas con formas redondeadas");
	addRenderModeCheckbox(RENDER_BASC, IconBasic , ui::Point(65, 22), "Gráficos básicos. Al desactivarlos, casi todo se vuelve invisible.");
	addRenderModeCheckbox(RENDER_SPRK, IconEffect, ui::Point(97,  4), "Hace brillar las chispas");

	auto addDisplayModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *displayModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		displayModes.push_back(displayModeCheckbox);
		displayModeCheckbox->mode = mode;
		displayModeCheckbox->SetIcon(icon);
		displayModeCheckbox->SetActionCallback({ [this, displayModeCheckbox] {
			auto displayMode = c->GetDisplayMode();
			// Air display modes are mutually exclusive
			if (displayModeCheckbox->mode & DISPLAY_AIR)
			{
				displayMode &= ~DISPLAY_AIR;
			}
			if (displayModeCheckbox->GetChecked())
			{
				displayMode |= displayModeCheckbox->mode;
			}
			else
			{
				displayMode &= ~displayModeCheckbox->mode;
			}
			c->SetDisplayMode(displayMode);
		} });
		AddComponent(displayModeCheckbox);
	};
	line1 = 130;
	addDisplayModeCheckbox(DISPLAY_AIRC, IconAltAir    , ui::Point(135,  4), "Muestra la presión en rojo y azul y la velocidad en blanco");
	addDisplayModeCheckbox(DISPLAY_AIRP, IconPressure  , ui::Point(135, 22), "Muestra la presión positiva en rojo y la negativa en azul");
	addDisplayModeCheckbox(DISPLAY_AIRV, IconVelocity  , ui::Point(167,  4), "Muestra velocidad y presión positiva. Azul: arriba/abajo. Rojo: izquierda/derecha. Verde: presión en reposo.");
	addDisplayModeCheckbox(DISPLAY_AIRH, IconHeat      , ui::Point(167, 22), "Muestra la temperatura del aire con la misma escala que el modo de calor");
	addDisplayModeCheckbox(DISPLAY_AIRW, IconVort      , ui::Point(199,  4), "Muestra la vorticidad en rojo para el giro horario y en azul para el antihorario");
	line2 = 232;
	addDisplayModeCheckbox(DISPLAY_WARP, IconWarp      , ui::Point(237, 22), "Lente gravitatoria. La gravedad newtoniana curva la luz.");
	addDisplayModeCheckbox(DISPLAY_EFFE, IconEffect    , ui::Point(237,  4), "Activa sólidos móviles, armas de los muñecos y gráficos avanzados");
	addDisplayModeCheckbox(DISPLAY_PERS, IconPersistant, ui::Point(269,  4), "Deja visible por un momento la trayectoria de los elementos");
	line3 = 302;

	auto addColourModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *colourModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		colourModes.push_back(colourModeCheckbox);
		colourModeCheckbox->mode = mode;
		colourModeCheckbox->SetIcon(icon);
		colourModeCheckbox->SetActionCallback({ [this, colourModeCheckbox] {
			auto colorMode = c->GetColorMode();
			// exception: looks like an independent set of settings but behaves more like an index
			if (colourModeCheckbox->GetChecked())
			{
				colorMode = colourModeCheckbox->mode;
			}
			else
			{
				colorMode = 0;
			}
			c->SetColorMode(colorMode);
		} });
		AddComponent(colourModeCheckbox);
	};
	addColourModeCheckbox(COLOUR_HEAT, IconHeat    , ui::Point(307,  4), "Muestra la temperatura de los elementos del azul oscuro (frío) al rosa (caliente)");
	addColourModeCheckbox(COLOUR_LIFE, IconLife    , ui::Point(307, 22), "Muestra el valor de vida de los elementos en escala de grises");
	addColourModeCheckbox(COLOUR_GRAD, IconGradient, ui::Point(339, 22), "Modifica el color de los elementos para mostrar cómo se propaga el calor");
	addColourModeCheckbox(COLOUR_BASC, IconBasic   , ui::Point(339,  4), "Oculta la decoración y desactiva todos los efectos gráficos");
	line4 = 372;
}

uint32_t RenderView::CalculateRenderMode()
{
	uint32_t renderMode = 0;
	for (auto &checkbox : renderModes)
	{
		if (checkbox->GetChecked())
			renderMode |= checkbox->mode;
	}

	return renderMode;
}

void RenderView::OnMouseDown(int x, int y, unsigned button)
{
	if(x > XRES || y < YRES)
		c->Exit();
}

void RenderView::OnTryExit(ExitMethod method)
{
	c->Exit();
}

void RenderView::NotifyRendererChanged(RenderModel * sender)
{
	ren = sender->GetRenderer();
	rendererSettings = sender->GetRendererSettings();
}

void RenderView::NotifySimulationChanged(RenderModel * sender)
{
	sim = sender->GetSimulation();
}

void RenderView::NotifyRenderChanged(RenderModel * sender)
{
	for (size_t i = 0; i < renderModes.size(); i++)
	{
		//Compares bitmasks at the moment, this means that "Point" is always on when other options that depend on it are, this might confuse some users, TODO: get the full list and compare that?
		auto renderMode = renderModes[i]->mode;
		renderModes[i]->SetChecked(renderMode == (sender->GetRenderMode() & renderMode));
	}
}

void RenderView::NotifyDisplayChanged(RenderModel * sender)
{
	for (size_t i = 0; i < displayModes.size(); i++)
	{
		auto displayMode = displayModes[i]->mode;
		displayModes[i]->SetChecked(displayMode == (sender->GetDisplayMode() & displayMode));
	}
}

void RenderView::NotifyColourChanged(RenderModel * sender)
{
	for (size_t i = 0; i < colourModes.size(); i++)
	{
		auto colorMode = colourModes[i]->mode;
		colourModes[i]->SetChecked(colorMode == sender->GetColorMode());
	}
}

void RenderView::OnDraw()
{
	Graphics * g = GetGraphics();
	g->DrawFilledRect(WINDOW.OriginRect(), 0x000000_rgb);
	auto *view = GameController::Ref().GetView();
	view->PauseRendererThread();
	ren->ApplySettings(*rendererSettings);
	view->RenderSimulation(*sim, true);
	view->AfterSimDraw(*sim);
	for (auto y = 0; y < YRES; ++y)
	{
		auto &video = ren->GetVideo();
		std::copy_n(video.data() + video.Size().X * y, video.Size().X, g->Data() + g->Size().X * y);
	}
	g->DrawLine({ 0, YRES }, { XRES-1, YRES }, 0xC8C8C8_rgb);
	g->DrawLine({ line1, YRES }, { line1, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line2, YRES }, { line2, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line3, YRES }, { line3, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line4, YRES }, { line4, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ XRES, 0 }, { XRES, WINDOWH }, 0xFFFFFF_rgb);
	if(toolTipPresence && toolTip.length())
	{
		int alpha = toolTipPresence > 51 ? 255 : toolTipPresence * 5;
		auto textPosition = Vec2{ 6, Size.Y-MENUSIZE-12 };
		int textWidth = Graphics::TextSize(toolTip).X;

		g->BlendFilledRect(
			RectSized(textPosition - Vec2{ 4, 4 }, Vec2{ textWidth + 8, 15 }),
			0x000000_rgb .WithAlpha(alpha / 2)
		);
		g->BlendText(textPosition, toolTip, 0xFFFFFF_rgb .WithAlpha(alpha));
	}
}

void RenderView::OnTick()
{
	if (isToolTipFadingIn)
	{
		isToolTipFadingIn = false;
		toolTipPresence.SetTarget(120);
	}
	else
	{
		toolTipPresence.SetTarget(0);
	}
}

void RenderView::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
		return;
	if (shift && key == '1')
		c->LoadRenderPreset(10);
	else if(key >= '0' && key <= '9')
	{
		c->LoadRenderPreset(key-'0');
	}
}

void RenderView::ToolTip(ui::Point senderPosition, String toolTip)
{
	this->toolTip = toolTip;
	this->isToolTipFadingIn = true;
}

RenderView::~RenderView() {
}
