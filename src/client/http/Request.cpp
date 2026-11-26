#include "Request.h"
#include "requestmanager/RequestManager.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <json/json.h>

namespace http
{
	Request::Request(ByteString newUri)
	{
		handle = RequestHandle::Create();
		handle->uri = newUri;
	}

	Request::~Request()
	{
		bool tryUnregister;
		{
			std::lock_guard lk(handle->stateMx);
			tryUnregister = handle->state == RequestHandle::running;
		}
		if (tryUnregister)
		{
			// At this point it may have already finished and been unregistered but that's ok,
			// attempting to unregister a request multiple times is allowed. We only do the
			// state-checking dance so we don't wake up RequestManager if we don't have to.
			// In fact, we could just not unregister requests here at all, they'd just run to
			// completion and be unregistered later. All this does is cancel them early.
			RequestManager::Ref().UnregisterRequest(*this);
		}
	}

	void Request::FailEarly(ByteString error)
	{
		assert(handle->state == RequestHandle::ready);
		handle->failEarly = error;
	}

	void Request::Verb(ByteString newVerb)
	{
		assert(handle->state == RequestHandle::ready);
		handle->verb = newVerb;
	}

	void Request::AddHeader(Header header)
	{
		assert(handle->state == RequestHandle::ready);
		handle->headers.push_back(header);
	}

	void Request::AddPostData(PostData data)
	{
		assert(handle->state == RequestHandle::ready);
		// Even if the map is empty, calling this function signifies you want to do a POST request
		handle->isPost = true;
		handle->postData = data;
	}

	void Request::AuthHeaders(ByteString ID, ByteString session)
	{
		assert(handle->state == RequestHandle::ready);
		if (ID.size() && ID != "-1") // -1 is an emscripten hack, see AuthUserEmscripten.cpp
		{
			if (session.size())
			{
				AddHeader({ "X-Auth-User-Id", ID });
				AddHeader({ "X-Auth-Session-Key", session });
			}
			else
			{
				AddHeader({ "X-Auth-User", ID });
			}
		}
	}

	void Request::Start()
	{
		assert(handle->state == RequestHandle::ready);
		handle->state = RequestHandle::running;
		RequestManager::Ref().RegisterRequest(*this);
	}

	bool Request::CheckDone() const
	{
		std::lock_guard lk(handle->stateMx);
		assert(handle->state == RequestHandle::running || handle->state == RequestHandle::done);
		return handle->state == RequestHandle::done;
	}

	std::pair<int64_t, int64_t> Request::CheckProgress() const
	{
		std::lock_guard lk(handle->stateMx);
		assert(handle->state == RequestHandle::running || handle->state == RequestHandle::done);
		return { handle->bytesTotal, handle->bytesDone };
	}

	const std::vector<Header> &Request::ResponseHeaders() const
	{
		std::lock_guard lk(handle->stateMx);
		assert(handle->state == RequestHandle::done);
		return handle->responseHeaders;
	}

	void Request::Wait()
	{
		std::unique_lock lk(handle->stateMx);
		assert(handle->state == RequestHandle::running);
		handle->stateCv.wait(lk, [this]() {
			return handle->state == RequestHandle::done;
		});
	}

	int Request::StatusCode() const
	{
		{
			std::unique_lock lk(handle->stateMx);
			assert(handle->state == RequestHandle::done);
		}
		return handle->statusCode;
	}

	std::pair<int, ByteString> Request::Finish()
	{
		{
			std::unique_lock lk(handle->stateMx);
			assert(handle->state == RequestHandle::done);
		}
		handle->state = RequestHandle::dead;
		if (handle->error)
		{
			throw RequestError(*handle->error);
		}
		return std::pair{ handle->statusCode, std::move(handle->responseData) };
	}

	void RequestHandle::MarkDone()
	{
		{
			std::lock_guard lk(stateMx);
			assert(state == RequestHandle::running);
			state = RequestHandle::done;
		}
		stateCv.notify_one();
		if (error)
		{
			std::cerr << *error << std::endl;
		}
		else if (statusCode >= 400)
		{
			std::cerr << "código de estado " << statusCode << " en la solicitud a " << uri << std::endl;
		}
	}

	const char *StatusText(int ret)
	{
		switch (ret)
		{
		case 0:   return "Código de estado 0 (posible error)";
		case 100: return "Continuar";
		case 101: return "Cambio de protocolo";
		case 102: return "En proceso";
		case 200: return "OK";
		case 201: return "Creado";
		case 202: return "Aceptado";
		case 203: return "Información no oficial";
		case 204: return "Sin contenido";
		case 205: return "Restablecer contenido";
		case 206: return "Contenido parcial";
		case 207: return "Estado múltiple";
		case 300: return "Múltiples opciones";
		case 301: return "Movido permanentemente";
		case 302: return "Encontrado";
		case 303: return "Consulta otro recurso";
		case 304: return "Sin modificar";
		case 305: return "Usar proxy";
		case 306: return "Cambiar proxy";
		case 307: return "Redirección temporal";
		case 400: return "Solicitud incorrecta";
		case 401: return "Sin autorización";
		case 402: return "Pago obligatorio";
		case 403: return "Acceso prohibido";
		case 404: return "No encontrado";
		case 405: return "Método no permitido";
		case 406: return "No aceptable";
		case 407: return "El proxy requiere autenticación";
		case 408: return "La solicitud agotó el tiempo de espera";
		case 409: return "Conflicto";
		case 410: return "Ya no está disponible";
		case 411: return "Se requiere la longitud del contenido";
		case 412: return "Falló una condición previa";
		case 413: return "El contenido de la solicitud es demasiado grande";
		case 414: return "URI demasiado larga";
		case 415: return "Tipo de contenido incompatible";
		case 416: return "Intervalo de datos no disponible";
		case 417: return "No se pudo cumplir el requisito de la solicitud";
		case 418: return "Soy una tetera";
		case 422: return "No se puede procesar el contenido";
		case 423: return "Bloqueado";
		case 424: return "Falló una dependencia";
		case 425: return "Colección desordenada";
		case 426: return "Se requiere una actualización";
		case 444: return "Sin respuesta";
		case 450: return "Bloqueado por el control parental de Windows";
		case 499: return "El cliente cerró la solicitud";
		case 500: return "Error interno del servidor";
		case 501: return "Función sin implementar";
		case 502: return "Puerta de enlace incorrecta";
		case 503: return "Servicio no disponible";
		case 504: return "La puerta de enlace agotó el tiempo de espera";
		case 505: return "Versión de HTTP incompatible";
		case 506: return "La variante también negocia";
		case 507: return "Espacio insuficiente";
		case 509: return "Se superó el límite de ancho de banda";
		case 510: return "Falta una extensión del protocolo";
		case 600: return "Error interno del cliente";
		case 601: return "Protocolo incompatible";
		case 602: return "Servidor no encontrado";
		case 603: return "Respuesta con formato incorrecto";
		case 604: return "Red no disponible";
		case 605: return "La solicitud agotó su tiempo de espera";
		case 606: return "URL con formato incorrecto";
		case 607: return "Conexión rechazada";
		case 608: return "Servidor proxy no encontrado";
		case 609: return "SSL: estado del certificado no válido";
		case 611: return "Demasiadas redirecciones";
		case 612: return "SSL: error de conexión";
		case 613: return "SSL: motor criptográfico no encontrado";
		case 614: return "SSL: no se pudo establecer el motor criptográfico predeterminado";
		case 615: return "SSL: problema con el certificado local";
		case 616: return "SSL: no se pudo usar el cifrado indicado";
		case 617: return "SSL: no se pudo iniciar el motor criptográfico";
		case 618: return "SSL: no se pudo cargar el archivo CACERT";
		case 619: return "SSL: no se pudo cargar el archivo CRL";
		case 620: return "SSL: no se pudo verificar al emisor del certificado";
		case 621: return "SSL: la clave pública no coincide con la esperada";
		}
		return "Código de estado desconocido";
	}

	void Request::ParseResponse(const ByteString &result, int status, ResponseType responseType)
	{
		// no server response, return "Malformed Response"
		if (status == 200 && !result.size())
		{
			status = 603;
		}
		if (status == 302)
		{
			return;
		}
		if (status != 200)
		{
			throw RequestError(ByteString::Build("Error HTTP ", status, ": ", http::StatusText(status)));
		}

		switch (responseType)
		{
		case responseOk:
			if (strncmp(result.c_str(), "OK", 2))
			{
				throw RequestError(result);
			}
			break;

		case responseJson:
			{
				std::istringstream ss(result);
				Json::Value root;
				int status;
				try
				{
					ss >> root;
					// assume everything is fine if an empty [] is returned
					if (root.size() == 0)
					{
						return;
					}
					status = root.get("Status", 1).asInt();
				}
				catch (const std::exception &ex)
				{
					// sometimes the server returns a 200 with the text "Error: 401"
					if (!strncmp(result.c_str(), "Error: ", 7))
					{
						status = ByteString(result.begin() + 7, result.end()).ToNumber<int>();
						throw RequestError(ByteString::Build("Error HTTP ", status, ": ", http::StatusText(status)));
					}
					throw RequestError("No se pudo leer la respuesta: " + ByteString(ex.what()));
				}

				if (status != 1)
				{
					throw RequestError(ByteString(root.get("Error", "Error sin especificar").asString()));
				}
			}
			break;

		case responseData:
			// no further processing required
			break;
		}
	}
}
