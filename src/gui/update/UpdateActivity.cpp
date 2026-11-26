#include "UpdateActivity.h"
#include "client/http/Request.h"
#include "prefs/GlobalPrefs.h"
#include "common/platform/Platform.h"
#include "tasks/Task.h"
#include "tasks/TaskWindow.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/interface/Engine.h"
#include "Config.h"
#include <bzlib.h>
#include <memory>

class UpdateDownloadTask : public Task
{
public:
	UpdateDownloadTask(ByteString updateName, UpdateActivity * a) : a(a), updateName(updateName) {}
private:
	UpdateActivity * a;
	ByteString updateName;
	void notifyDoneMain() override {
		a->NotifyDone(this);
	}
	void notifyErrorMain() override
	{
		a->NotifyError(this);
	}
	bool doWork() override
	{
		auto &prefs = GlobalPrefs::Ref();

		auto niceNotifyError = [this](String error) {
			notifyError("La actualización descargada está dañada\n" + error);
			return false;
		};

		auto request = std::make_unique<http::Request>(updateName);
		request->Start();
		notifyStatus("Descargando actualización");
		notifyProgress(-1);
		while(!request->CheckDone())
		{
			int64_t total, done;
			std::tie(total, done) = request->CheckProgress();
			if (total == -1)
			{
				notifyProgress(-1);
			}
			else
			{
				notifyProgress(total ? done * 100 / total : 0);
			}
			Platform::Millisleep(1);
		}

		int status;
		ByteString data;
		try
		{
			std::tie(status, data) = request->Finish();
		}
		catch (const http::RequestError &ex)
		{
			return niceNotifyError("No se pudo descargar la actualización: " + String::Build("El servidor devolvió el estado ", ByteString(ex.what()).FromUtf8()));
		}
		if (status!=200)
		{
			return niceNotifyError("No se pudo descargar la actualización: " + String::Build("El servidor devolvió el estado ", status));
		}
		if (!data.size())
		{
			return niceNotifyError("El servidor no devolvió datos");
		}

		notifyStatus("Descomprimiendo la actualización");
		notifyProgress(-1);

		unsigned int uncompressedLength;

		if(data.size()<16)
		{
			return niceNotifyError(String::Build("Datos insuficientes. Se recibieron ", data.size(), " bytes"));
		}
		if (data[0]!=0x42 || data[1]!=0x75 || data[2]!=0x54 || data[3]!=0x54)
		{
			return niceNotifyError("Formato de actualización no válido");
		}

		uncompressedLength  = (unsigned char)data[4];
		uncompressedLength |= ((unsigned char)data[5])<<8;
		uncompressedLength |= ((unsigned char)data[6])<<16;
		uncompressedLength |= ((unsigned char)data[7])<<24;

		std::vector<char> res(uncompressedLength);

		int dstate;
		dstate = BZ2_bzBuffToBuffDecompress(res.data(), (unsigned *)&uncompressedLength, &data[8], data.size()-8, 0, 0);
		if (dstate)
		{
			return niceNotifyError(String::Build("No se pudo descomprimir la actualización: ", dstate));
		}

		notifyStatus("Aplicando la actualización");
		notifyProgress(-1);

		prefs.Set("version.update", true);
		if (!Platform::UpdateStart(res))
		{
			prefs.Set("version.update", false);
			Platform::UpdateCleanup();
			notifyError("No se pudo actualizar. Descarga una versión nueva.");
			return false;
		}

		return true;
	}
};

UpdateActivity::UpdateActivity(UpdateInfo info)
{
	updateDownloadTask = new UpdateDownloadTask(info.file, this);
	updateWindow = new TaskWindow("Descargando actualización...", updateDownloadTask, true);
}

void UpdateActivity::NotifyDone(Task * sender)
{
	if(sender->GetSuccess())
	{
		Exit();
	}
}

void UpdateActivity::Exit()
{
	updateWindow->Exit();
	ui::Engine::Ref().Exit();
	delete this;
}

void UpdateActivity::NotifyError(Task * sender)
{
	StringBuilder sb;
	if constexpr (USE_UPDATESERVER)
	{
		sb << "Conéctate a Internet y descarga una versión más reciente.\n";
	}
	else
	{
		sb << "Visita el sitio web y descarga una versión más reciente.\n";
	}
	sb << "Error: " << sender->GetError();
	new ConfirmPrompt("Falló la actualización automática", sb.Build(), { [this] {
		if constexpr (!USE_UPDATESERVER)
		{
			Platform::OpenURI(ByteString::Build(SERVER, "/Download.html"));
		}
		Exit();
	}, [this] { Exit(); } });
}


UpdateActivity::~UpdateActivity() {
}
