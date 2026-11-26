#include "ServerSaveActivity.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/SaveIDMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "client/Client.h"
#include "client/ThumbnailRendererTask.h"
#include "client/GameSave.h"
#include "client/http/UploadSaveRequest.h"
#include "tasks/Task.h"
#include "gui/Style.h"

class SaveUploadTask: public Task
{
	SaveInfo &save;

	void before() override
	{

	}

	void after() override
	{

	}

	bool doWork() override
	{
		notifyProgress(-1);
		auto uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(save);
		uploadSaveRequest->Start();
		uploadSaveRequest->Wait();
		try
		{
			save.SetID(uploadSaveRequest->Finish());
		}
		catch (const http::RequestError &ex)
		{
			notifyError(ByteString(ex.what()).FromUtf8());
			return false;
		}
		return true;
	}

public:
	SaveUploadTask(SaveInfo &newSave):
		save(newSave)
	{

	}
};

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(440, 200)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point((Size.X/2)-8, 16), "");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);
	CheckName(save->GetName()); //set titleLabel text

	ui::Label * previewLabel = new ui::Label(ui::Point((Size.X/2)+4, 5), ui::Point((Size.X/2)-8, 16), "Vista previa:");
	previewLabel->SetTextColour(style::Colour::InformationTitle);
	previewLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	previewLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(previewLabel);

	nameField = new ui::Textbox(ui::Point(8, 25), ui::Point((Size.X/2)-16, 16), save->GetName(), "[nombre de la partida]");
	nameField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	nameField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	nameField->SetActionCallback({ [this] { CheckName(nameField->GetText()); } });
	nameField->SetLimit(50);
	AddComponent(nameField);
	FocusComponent(nameField);

	descriptionField = new ui::Textbox(ui::Point(8, 65), ui::Point((Size.X/2)-16, Size.Y-(65+16+4)), save->GetDescription(), "[descripción de la partida]");
	descriptionField->SetMultiline(true);
	descriptionField->SetLimit(254);
	descriptionField->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	descriptionField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(descriptionField);

	publishedCheckbox = new ui::Checkbox(ui::Point(8, 45), ui::Point((Size.X/2)-80, 16), "Publicar", "");
	if (!save->IsOwn())
	{
		//Save is not owned by the user, disable by default
		publishedCheckbox->SetChecked(false);
	}
	else
	{
		//Save belongs to the current user, use published state already set
		publishedCheckbox->SetChecked(save->GetPublished());
	}
	AddComponent(publishedCheckbox);

	pausedCheckbox = new ui::Checkbox(ui::Point(160, 45), ui::Point(55, 16), "En pausa", "");
	pausedCheckbox->SetChecked(save->GetGameSave()->paused);
	AddComponent(pausedCheckbox);

	ui::Button * cancelButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point((Size.X/2)-75, 16), "Cancelar");
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback({ [this] {
		Exit();
	} });
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button * okayButton = new ui::Button(ui::Point((Size.X/2)-76, Size.Y-16), ui::Point(76, 16), "Guardar");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.TextInactive = style::Colour::InformationTitle;
	okayButton->SetActionCallback({ [this] {
		Save();
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	ui::Button * PublishingInfoButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-42), ui::Point(150, 16), "Sobre publicar");
	PublishingInfoButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	PublishingInfoButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	PublishingInfoButton->Appearance.TextInactive = style::Colour::InformationTitle;
	PublishingInfoButton->SetActionCallback({ [this] {
		ShowPublishingInfo();
	} });
	AddComponent(PublishingInfoButton);

	ui::Button * RulesButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-22), ui::Point(150, 16), "Normas de publicación");
	RulesButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	RulesButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	RulesButton->Appearance.TextInactive = style::Colour::InformationTitle;
	RulesButton->SetActionCallback({ [this] {
		ShowRules();
	} });
	AddComponent(RulesButton);

	if (save->GetGameSave())
	{
		thumbnailRenderer = new ThumbnailRendererTask(*save->GetGameSave(), Size / 2 - Vec2(16, 16), RendererSettings::decorationAntiClickbait, true);
		thumbnailRenderer->Start();
	}
}

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, bool saveNow, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(200, 50)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	ui::Label * titleLabel = new ui::Label(ui::Point(0, 0), Size, "Guardando en el servidor...");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	AddAuthorInfo();

	saveUploadTask = new SaveUploadTask(*this->save);
	saveUploadTask->AddTaskListener(this);
	saveUploadTask->Start();
}

void ServerSaveActivity::NotifyDone(Task * task)
{
	if(!task->GetSuccess())
	{
		Exit();
		new ErrorMessage("Error", task->GetError());
	}
	else
	{
		if (onUploaded)
		{
			onUploaded(std::move(save));
		}
		Exit();
	}
}

void ServerSaveActivity::Save()
{
	if (!nameField->GetText().length())
	{
		new ErrorMessage("Error", "Escribe un nombre para la partida.");
		return;
	}
	if (!save->IsOwn() && publishedCheckbox->GetChecked())
	{
		new ConfirmPrompt("Publicar", "El autor de esta partida es " + save->GetUserName().FromUtf8() + ". Vas a publicarla con tu nombre. Si el autor te dio permiso, puedes continuar. Si no, desmarca «Publicar».", { [this] {
			saveUpload();
		} });
	}
	else
	{
		saveUpload();
	}
}

void ServerSaveActivity::AddAuthorInfo()
{
	Json::Value serverSaveInfo;
	serverSaveInfo["type"] = "save";
	serverSaveInfo["id"] = save->GetID();
	auto user = Client::Ref().GetAuthUser();
	serverSaveInfo["username"] = user ? user->Username : ByteString("");
	serverSaveInfo["title"] = save->GetName().ToUtf8();
	serverSaveInfo["description"] = save->GetDescription().ToUtf8();
	serverSaveInfo["published"] = (int)save->GetPublished();
	serverSaveInfo["date"] = (Json::Value::UInt64)time(nullptr);
	Client::Ref().SaveAuthorInfo(&serverSaveInfo);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->authors = serverSaveInfo;
		save->SetGameSave(std::move(gameSave));
	}
}

void ServerSaveActivity::saveUpload()
{
	okayButton->Enabled = false;
	save->SetName(nameField->GetText());
	save->SetDescription(descriptionField->GetText());
	save->SetPublished(publishedCheckbox->GetChecked());
	auto user = Client::Ref().GetAuthUser();
	save->SetUserName(user ? user->Username : ByteString(""));
	save->SetID(0);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->paused = pausedCheckbox->GetChecked();
		save->SetGameSave(std::move(gameSave));
	}
	AddAuthorInfo();
	uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(*save);
	uploadSaveRequest->Start();
}

void ServerSaveActivity::Exit()
{
	WindowActivity::Exit();
}

void ServerSaveActivity::ShowPublishingInfo()
{
	String info =
		"The Powder Toy permite guardar simulaciones en tu cuenta con dos niveles de visibilidad. Marca «Publicar» para que aparezcan públicamente. Las partidas se guardan sin publicar de forma predeterminada.\n"
		"\n"
		"Las \btpartidas publicadas\bw aparecen en «Por fecha» y cuentan para tu puntuación media, que se muestra en tu perfil público. Publica las partidas que quieras compartir para que otros puedan votar y comentar.\n"
		"Las \btpartidas sin publicar\bw no aparecen en «Por fecha» ni cuentan para tu puntuación media. Cualquiera que tenga el ID puede ver una partida, así que puedes compartirlo con personas concretas.\n"
		"\n"
		"Para volver a subir una partida, ábrela y pulsa el lado izquierdo del botón de guardado dividido, \bt«Volver a subir la simulación actual»\bw. Para cambiar la descripción o la visibilidad, pulsa el lado derecho, \bt«Modificar propiedades de la simulación»\bw. Si cambias el nombre al guardar, se crea otra partida, sin los comentarios, votos ni etiquetas de la original.\n"
		"Para publicar o retirar una partida, ábrela, elige «Modificar propiedades de la simulación» y cambia su visibilidad. También puedes \btretirar o eliminar partidas\bw desde «Mis partidas» en el navegador. Selecciónalas y usa los botones de abajo.\n"
		"Si una partida publicada con menos de una semana gana popularidad rápidamente, aparece automáticamente en la \btportada\bw. Los moderadores también pueden destacar partidas en la portada, aunque es poco común, o retirar las que incumplan una norma o no correspondan.\n"
		"Puedes guardar de nuevo una partida tantas veces como quieras. El \bthistorial de partidas\bw conserva algunas versiones anteriores. Haz clic derecho en la partida en el navegador y elige «Ver historial». Así puedes recuperar una versión anterior si guardaste algo por error.\n"
		;

	new InformationMessage("Sobre publicar", info, true);
}

void ServerSaveActivity::ShowRules()
{
	String rules =
		"\boSección S: normas sociales y de la comunidad\n"
		"\bwEstas normas se aplican a las partidas subidas, los comentarios, los foros y otras áreas de la comunidad. El equipo de moderación se encarga de hacerlas cumplir. Puedes avisarle si ves una infracción.\n"
		"\n"
		"\bt1. Procura escribir con claridad y buena gramática.\bw El inglés es el idioma oficial de la comunidad. También puedes usar otros idiomas en grupos regionales o culturales. Si te cuesta escribir en inglés, puedes usar el Traductor de Google.\n"
		"\bt2. Evita el spam.\bw El spam toma muchas formas. El equipo de moderación puede ocultar o eliminar lo siguiente:\n"
		   "- Abrir varios hilos sobre el mismo tema. Agrupa tus comentarios o propuestas sobre el juego en un solo hilo.\n"
		   "- Revivir hilos antiguos con una respuesta. El contenido del hilo podría haber quedado obsoleto. Abre uno nuevo si tienes información actualizada.\n"
		   "- Responder «+1» u otros mensajes breves que solo vuelven a subir el hilo. Usa las respuestas para aportar y el botón «+1» para mostrar tu apoyo.\n"
		   "- Publicar comentarios excesivamente largos o sin sentido, como repetir una letra muchas veces. Esta regla no se aplica por el mero hecho de escribir en otro idioma.\n"
		   "- Abusar del formato. Usa mayúsculas, negrita y cursiva con moderación.\n"
		"\bt3. Limita las groserías.\bw Los comentarios o partidas que las incluyan pueden eliminarse, también si están en otro idioma.\n"
		"\bt4. Evita publicar contenido sexual explícito, ofensivo o inapropiado.\bw\n"
		   "- Incluye temas como sexo, drogas, racismo, exceso de contenido político o material que ofenda a un grupo de personas.\n"
		   "- La norma también se aplica si haces referencia a estos temas en otro idioma.\n"
		   "- También se aplica a enlaces e imágenes, incluso en tu perfil.\n"
		"\bt5. Evita anunciar juegos, sitios o comunidades ajenos a The Powder Toy.\bw\n"
		   "- Esto incluye promocionar tus propios juegos y productos.\n"
		   "- Se prohíbe promocionar espacios comunitarios no autorizados, como servidores de Discord no oficiales.\n"
		"\bt6. Se prohíben las provocaciones deliberadas.\bw Quienes provoquen a otros repetidamente tienen más probabilidades de recibir sanciones y de que estas duren más.\n"
		"\bt7. Evita suplantar a otros usuarios.\bw No se permite registrar cuentas con nombres deliberadamente parecidos a los de otras personas de esta u otras comunidades.\n"
		"\bt8. Evita discutir las decisiones de moderación en público.\bw Si suspendieron tu cuenta o retiraron contenido, contacta con un moderador por el sistema de mensajes.\n"
		"\bt9. Deja las sanciones a cargo de los moderadores.\bw Evita amenazar a otros usuarios con expulsiones o castigos. Si ves una posible infracción, utiliza «Denunciar» o el sistema de mensajes del sitio web.\n"
		"\bt10. Se prohíbe alentar actividades ilegales.\bw Las leyes pueden variar según el país. Entre los casos habituales están:\n"
		   "- Piratería de software, música, bagels, etc.\n"
		   "- Acceso no autorizado a cuentas o robo de cuentas.\n"
		   "- Robo o fraude.\n"
		"\bt11. Se prohíbe acosar o perseguir a otros usuarios.\bw El acoso puede adoptar formas como estas:\n"
		   "- Investigar y divulgar dónde vive un usuario o cuál es su identidad real.\n"
		   "- Enviar mensajes repetidos a alguien que ha pedido que no lo contacten.\n"
		   "- Votar negativamente en masa las partidas de una persona.\n"
		   "- Dejar comentarios groseros o innecesarios en partidas, hilos u otro contenido.\n"
		   "- Incitar a un grupo de usuarios a hostigar a alguien.\n"
		   "- Mantener peleas personales en los comentarios o crear partidas para difundir odio.\n"
		   "- Discriminar por motivos religiosos, étnicos o de otro tipo.\n"
		"\n"
		"\boSección G: normas del juego\n"
		"\bwAdemás de las normas de la sección S, las siguientes se aplican a la interacción dentro del juego.\n"
		"\bt1. Respeta la autoría de otros usuarios.\bw Volver a subir una partida ajena o reutilizar grandes partes de ella sin reconocer al autor puede incumplir esta norma. Se permiten obras derivadas que aporten algo original. Salvo que el autor indique otras condiciones, dale crédito cuando uses su trabajo. Las partidas copiadas sin permiso se retirarán o desactivarán.\n"
		"\bt2. Se prohíbe votar por tus propias partidas o manipular votaciones.\bw No crees varias cuentas para votar por tus partidas o por las de otros. Esta norma se aplica de forma estricta y pocas apelaciones prosperan. Evita que varias cuentas de un mismo hogar voten por la misma partida. Se expulsará permanentemente a las cuentas alternas, se suspenderá la principal y se desactivarán las partidas afectadas.\n"
		"\bt3. Evita pedir votos.\bw Las partidas que lo hagan se retirarán hasta que se corrijan. Esta norma incluye:\n"
		   "- Letreros que sugieran votar a favor o en contra, como los que muestran una flecha verde.\n"
		   "- Promesas a cambio de votos. Por ejemplo, ofrecer una versión mejor si se alcanzan 100 votos.\n"
		   "- Pedir votos a cambio de permitir el uso de una partida o por cualquier otro motivo.\n"
		"\bt4. Evita el spam.\bw Estos son algunos ejemplos dentro del juego:\n"
		   "- Subir o volver a subir partidas similares en poco tiempo. Evita manipular el sistema para conseguir visitas o votos. Se retirarán las partidas vacías o de relleno sin un propósito claro.\n"
		   "- Subir partidas que solo contengan texto, como anuncios o peticiones de ayuda. Usa los foros y comentarios para estos fines. Estas partidas se retirarán de la portada.\n"
		   "- Se permite subir arte, aunque puede retirarse de la portada. Se valora el uso creativo de diversos elementos. Las partidas que solo contengan decoración normalmente se retirarán de la portada.\n"
		"\bt5. Se prohíbe subir material sexual explícito o inapropiado. Se eliminará la partida y se sancionará a la cuenta.\bw\n"
		   "- Incluye temas como sexo, drogas, racismo, exceso de contenido político o material que ofenda a un grupo de personas.\n"
		   "- La norma cubre las referencias deliberadas, ya sean directas o indirectas.\n"
		   "- La norma también se aplica si haces referencia a estos temas en otro idioma.\n"
		   "- También se aplica a enlaces e imágenes, incluso en tu perfil.\n"
		"\bt6. Se prohíbe importar imágenes mediante scripts o herramientas externas.\bw Se eliminarán las partidas creadas por estos medios y podrás recibir una sanción.\n"
		"\bt7. Limita el uso de logotipos y letreros.\bw Las partidas que abusen de ellos pueden retirarse de la portada. Esto incluye:\n"
		   "- Usar demasiados logotipos.\n"
		   "- Colocar letreros sin un propósito.\n"
		   "- Mostrar avisos o notificaciones de actualización falsos.\n"
		   "- Enlazar otras partidas sin relación.\n"
		"\bt8. Usa etiquetas pertinentes y apropiadas.\bw Las etiquetas sirven para mejorar la búsqueda y normalmente deben describir la partida con una sola palabra. Las frases o etiquetas subjetivas pueden eliminarse. Las etiquetas ofensivas pueden provocar una sanción.\n"
		"\bt9. Se prohíben las partidas diseñadas para ralentizar o bloquear el juego.\bw Si la mayoría de los usuarios indica que una partida provoca estos fallos, puede retirarse de la portada o desactivarse.\n"
		"\bt10. Usa el sistema de denuncias de buena fe.\bw Describe la posible infracción o el problema para la comunidad. Evita motivos vagos como «mala partida» o texto sin sentido. Si crees que hay un problema, denúncialo. Denunciar de buena fe no conlleva sanciones.\n"
		"\bt11. Solicita retirar partidas de la portada solo si incumplen alguna norma.\bw Esta norma también se aplica a las partidas artísticas.\n"
		"\n"
		"\boSección R: otras disposiciones\n"
		"\bwLos moderadores interpretan estas normas y toman la decisión final sobre las infracciones. La aplicación puede variar según la norma. Se publicará un aviso en el hilo cuando se actualicen.\n"
		"\n"
		"Las infracciones pueden conllevar la retirada de publicaciones, comentarios o partidas. En casos más graves, pueden causar una suspensión temporal o permanente. Para aplicar las normas se emplean medidas manuales y automáticas. La gravedad de las sanciones puede variar según el caso y el moderador.\n"
		"\n"
		"Si tienes dudas sobre estas normas, contacta con un moderador.";

	new InformationMessage("Normas de publicación", rules, true);
}

void ServerSaveActivity::CheckName(String newname)
{
	auto user = Client::Ref().GetAuthUser();
	if (newname.length() && newname == save->GetName() && user && save->GetUserName() == user->Username)
		titleLabel->SetText("Modificar propiedades de la partida:");
	else
		titleLabel->SetText("Subir partida nueva:");
}

void ServerSaveActivity::OnTick()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Poll();
		if (thumbnailRenderer->GetDone())
		{
			thumbnail = thumbnailRenderer->Finish();
			thumbnailRenderer = nullptr;
		}
	}

	if (uploadSaveRequest && uploadSaveRequest->CheckDone())
	{
		okayButton->Enabled = true;
		try
		{
			save->SetID(uploadSaveRequest->Finish());
			Exit();
			new SaveIDMessage(save->GetID());
			if (onUploaded)
			{
				onUploaded(std::move(save));
			}
		}
		catch (const http::RequestError &ex)
		{
			new ErrorMessage("Error", "No se pudo subir la partida:\n" + ByteString(ex.what()).FromUtf8());
		}
		uploadSaveRequest.reset();
	}

	if(saveUploadTask)
		saveUploadTask->Poll();
}

void ServerSaveActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->BlendRGBAImage(saveToServerImage->data(), RectSized(Vec2(-10, 0), saveToServerImage->Size()));
	g->DrawFilledRect(RectSized(Position, Size).Inset(-1), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);

	if (Size.X > 220)
		g->DrawLine(Position + Vec2(Size.X / 2 - 1, 0), Position + Vec2(Size.X / 2 - 1, Size.Y - 1), 0xFFFFFF_rgb);

	if (thumbnail)
	{
		auto rect = RectSized(Position + Vec2(Size.X / 2 + (Size.X / 2 - thumbnail->Size().X) / 2, 25), thumbnail->Size());
		g->BlendImage(thumbnail->Data(), 0xFF, rect);
		g->DrawRect(rect, 0xB4B4B4_rgb);
	}
}

ServerSaveActivity::~ServerSaveActivity()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Abandon();
	}
	delete saveUploadTask;
}
