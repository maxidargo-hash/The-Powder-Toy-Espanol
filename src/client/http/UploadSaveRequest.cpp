#include "UploadSaveRequest.h"
#include "client/SaveInfo.h"
#include "client/Client.h"
#include "client/GameSave.h"
#include "Config.h"

namespace http
{
	UploadSaveRequest::UploadSaveRequest(const SaveInfo &saveInfo) : Request(ByteString::Build(SERVER, "/Save.api"))
	{
		auto [ fromNewerVersion, gameData ] = saveInfo.GetGameSave()->Serialise();
		if (!gameData.size())
		{
			FailEarly("No se pudo serializar la partida");
			return;
		}
		else if (ALLOW_FAKE_NEWER_VERSION && fromNewerVersion && saveInfo.GetPublished())
		{
			FailEarly("No se puede publicar la partida porque es incompatible con la última versión estable");
			return;
		}
		auto user = Client::Ref().GetAuthUser();
		if (!user)
		{
			FailEarly("Debes iniciar sesión");
			return;
		}
		AuthHeaders(ByteString::Build(user->UserID), user->SessionID);
		AddPostData(FormData{
			{ "Name", saveInfo.GetName().ToUtf8() },
			{ "Description", saveInfo.GetDescription().ToUtf8() },
			{ "Data", ByteString(gameData.begin(), gameData.end()), "save.bin" },
			{ "Publish", saveInfo.GetPublished() ? "Public" : "Private" },
			{ "Key", user->SessionKey },
		});
	}

	int UploadSaveRequest::Finish()
	{
		auto [ status, data ] = Request::Finish();
		ParseResponse(data, status, responseOk);
		int saveID = ByteString(data.begin() + 3, data.end()).ToNumber<int>();
		if (!saveID)
		{
			throw RequestError("El servidor no devolvió el ID de la partida");
		}
		return saveID;
	}
}
