#include "SpanishText.h"
#include "Request.h"
#ifndef NOHTTP
#include "defines.h"
#include <curl/curl.h>
#include <iostream>
#include "RequestManager.h"
#include "common/Format.h"
#include "common/Platform.h"


#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 56, 0)
# define REQUEST_USE_CURL_MIMEPOST
#endif

#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 61, 0)
# define REQUEST_USE_CURL_TLSV13CL
#endif

void SetupCurlEasyCiphers(CURL *easy)
{
#ifdef SECURE_CIPHERS_ONLY
	curl_version_info_data *version_info = curl_version_info(CURLVERSION_NOW);
	std::string ssl_type = version_info->ssl_version;
	if (ssl_type.find("OpenSSL") != ssl_type.npos)
	{
		curl_easy_setopt(easy, CURLOPT_SSL_CIPHER_LIST, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES128-SHA:DHE-RSA-AES128-GCM-SHA256");
#ifdef REQUEST_USE_CURL_TLSV13CL
		curl_easy_setopt(easy, CURLOPT_TLS13_CIPHERS, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_128_CCM_8_SHA256:TLS_AES_128_CCM_SHA256");
#endif
	}
	else if (ssl_type.find("Schannel") != ssl_type.npos)
	{
		// TODO: add more cipher algorithms
		curl_easy_setopt(easy, CURLOPT_SSL_CIPHER_LIST, "CALG_ECDH_EPHEM");
	}
#endif
	// TODO: Find out what TLS1.2 is supported on, might need to also allow TLS1.0
	curl_easy_setopt(easy, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 70, 0)
	curl_easy_setopt(easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_REVOKE_BEST_EFFORT);
#elif defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 44, 0)
	curl_easy_setopt(easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);
#elif defined(WIN)
# error "Curl version too low (v7.44+ required)."
#endif
}

Request::Request(std::string uri_):
	uri(uri_),
	rm_total(0),
	rm_done(0),
	rm_finished(false),
	rm_canceled(false),
	rm_started(false),
	added_to_multi(false),
	status(0),
	headers(NULL),
#ifdef REQUEST_USE_CURL_MIMEPOST
	post_fields(NULL)
#else
	post_fields_first(NULL),
	post_fields_last(NULL)
#endif
{
	error_buffer = new char[CURL_ERROR_SIZE];
	easy = curl_easy_init();
	if (!RequestManager::Ref().AddRequest(this))
	{
		status = 604;
		rm_finished = true;
	}
}
#else
Request::Request(std::string uri_) {}
#endif

Request::~Request()
{
#ifndef NOHTTP
	curl_easy_cleanup(easy);
#ifdef REQUEST_USE_CURL_MIMEPOST
	curl_mime_free(post_fields);
#else
	curl_formfree(post_fields_first);
#endif
	curl_slist_free_all(headers);
	delete[] error_buffer;
#endif
}

void Request::Verb(std::string newVerb)
{
#ifndef NOHTTP
	verb = newVerb;
#endif
}

void Request::AddHeader(http::Header header)
{
#ifndef NOHTTP
	headers = curl_slist_append(headers, (header.name + ": " + header.value).c_str());
#endif
}

// add post data to a request
void Request::AddPostData(http::PostData data)
{
	isPost = true;
#ifndef NOHTTP
	if (easy)
	{
		if (std::holds_alternative<http::FormData>(data) && std::get<http::FormData>(data).size())
		{
			auto &formData = std::get<http::FormData>(data);
#ifdef REQUEST_USE_CURL_MIMEPOST
			if (!post_fields)
			{
				post_fields = curl_mime_init(easy);
			}

			for (auto &field : formData)
			{
				curl_mimepart *part = curl_mime_addpart(post_fields);
				curl_mime_data(part, &field.value[0], field.value.size());
				curl_mime_name(part, field.name.c_str());
				if (field.filename.has_value())
				{
					curl_mime_filename(part, field.filename->c_str());
				}
			}
#else
			post_fields_map.insert(formData.begin(), formData.end());
#endif
			use_string_post_field = false;
		}
		else if (std::holds_alternative<http::StringData>(data) && std::get<http::StringData>(data).size())
		{
			auto &stringData = std::get<http::StringData>(data);
			post_field_str = stringData;
			use_string_post_field = true;
		}
	}
#endif
}

// add userID and sessionID headers to the request
void Request::AuthHeaders(std::string ID, std::string session)
{
	if (ID.size())
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

#ifndef NOHTTP
size_t Request::HeaderDataHandler(char *ptr, size_t size, size_t count, void *userdata)
{
	Request *req = (Request *)userdata;
	auto actual_size = size * count;
	if (actual_size >= 2 && ptr[actual_size - 2] == '\r' && ptr[actual_size - 1] == '\n')
	{
		if (actual_size > 2 && req->gotStatusLine) // Don't include header list terminator or the status line.
		{
			std::string line = std::string(ptr, ptr + actual_size - 2);
			size_t splitPos = line.find(":");
			if (splitPos != line.npos)
			{
				std::string before = line.substr(0, splitPos);
				std::string after = line.substr(splitPos + 1);

				while (after.size() && (after.front() == ' ' || after.front() == '\t'))
				{
					after = after.substr(1);
				}
				while (after.size() && (after.back() == ' ' || after.back() == '\t'))
				{
					after = after.substr(0, after.size() - 1);
				}
				req->response_headers.push_back({ Format::ToLower(before), after });
			}
			else
			{
				std::cerr << "skipping weird header: " << line << std::endl;
			}
		}
		req->gotStatusLine = true;
		return actual_size;
	}
	return 0;
}

size_t Request::WriteDataHandler(char *ptr, size_t size, size_t count, void *userdata)
{
	Request *req = (Request *)userdata;
	auto actual_size = size * count;
	req->response_body.append(ptr, actual_size);
	return actual_size;
}
#endif

// start the request thread
void Request::Start()
{
#ifndef NOHTTP
	if (CheckStarted() || CheckDone())
	{
		return;
	}

	if (easy)
	{
		if (use_string_post_field)
		{
			curl_easy_setopt(easy, CURLOPT_POSTFIELDS, &post_field_str[0]);
			curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(post_field_str.size()));
		}
		else
		{
#ifdef REQUEST_USE_CURL_MIMEPOST
			if (post_fields)
			{
				curl_easy_setopt(easy, CURLOPT_MIMEPOST, post_fields);
			}
			else if (isPost)
			{
				curl_easy_setopt(easy, CURLOPT_POST, 1L);
				curl_easy_setopt(easy, CURLOPT_POSTFIELDS, "");
			}
			else
			{
				curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
			}
#else
			if (!post_fields_map.empty())
			{
				for (auto &field : post_fields_map)
				{
					if (field.filename.has_value())
					{
						curl_formadd(&post_fields_first, &post_fields_last,
							CURLFORM_COPYNAME, field.name.c_str(),
							CURLFORM_BUFFER, field.filename->c_str(),
							CURLFORM_BUFFERPTR, &field.value[0],
							CURLFORM_BUFFERLENGTH, field.value.size(),
						CURLFORM_END);
					}
					else
					{
						curl_formadd(&post_fields_first, &post_fields_last,
							CURLFORM_COPYNAME, field.name.c_str(),
							CURLFORM_PTRCONTENTS, &field.value[0],
							CURLFORM_CONTENTLEN, field.value.size(),
						CURLFORM_END);
					}
				}
				curl_easy_setopt(easy, CURLOPT_HTTPPOST, post_fields_first);
			}
			else
			{
				curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
			}
#endif
		}

		if (verb.size())
		{
			curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, verb.c_str());
		}
		curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 85, 0)
# ifdef ENFORCE_HTTPS
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https");
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
# else
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https,http");
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https,http");
# endif
#else
# ifdef ENFORCE_HTTPS
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
# else
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
# endif
#endif
		SetupCurlEasyCiphers(easy);
		curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 10L);

		curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer);
		error_buffer[0] = 0;

		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);
		curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(easy, CURLOPT_URL, uri.c_str());

		if (proxy.size())
		{
			curl_easy_setopt(easy, CURLOPT_PROXY, proxy.c_str());
		}

		curl_easy_setopt(easy, CURLOPT_PRIVATE, (void *) this);
		curl_easy_setopt(easy, CURLOPT_USERAGENT, user_agent.c_str());
#ifdef DEBUG
		curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
#endif

		curl_easy_setopt(easy, CURLOPT_HEADERDATA, (void *)this);
		curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, Request::HeaderDataHandler);

		curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *) this);
		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, Request::WriteDataHandler);
	}

	{
		std::lock_guard<std::mutex> g(rm_mutex);
		rm_started = true;
	}
	RequestManager::Ref().StartRequest(this);
#endif
}


// finish the request (if called before the request is done, this will block)
std::string Request::Finish(int *status_out, std::vector<http::Header> *headers_out)
{
#ifndef NOHTTP
	if (CheckCanceled())
	{
		return ""; // shouldn't happen but just in case
	}

	std::string response_out;
	{
		std::unique_lock<std::mutex> l(rm_mutex);
		done_cv.wait(l, [this]() { return rm_finished; });

		rm_started = false;
		rm_canceled = true;
		if (status_out)
		{
			*status_out = status;
		}
		if (headers_out)
		{
			*headers_out = std::move(response_headers);
		}
		response_out = std::move(response_body);
	}

	RequestManager::Ref().RemoveRequest(this);
	return response_out;
#else
	if (status_out)
		*status_out = 604;
	return "";
#endif
}

void Request::CheckProgress(int *total, int *done)
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	if (total)
	{
		*total = rm_total;
	}
	if (done)
	{
		*done = rm_done;
	}
#endif
}

// returns true if the request has finished
bool Request::CheckDone()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_finished;
#else
	return true;
#endif
}

// returns true if the request was canceled
bool Request::CheckCanceled()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_canceled;
#else
	return false;
#endif
}

// returns true if the request is running
bool Request::CheckStarted()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_started;
#else
	return true;
#endif
}

// cancels the request, the request thread will delete the Request* when it finishes (do not use Request in any way after canceling)
void Request::Cancel()
{
#ifndef NOHTTP
	{
		std::lock_guard<std::mutex> g(rm_mutex);
		rm_canceled = true;
	}
	RequestManager::Ref().RemoveRequest(this);
#endif
}

std::string Request::Simple(std::string uri, int *status, http::FormData postData)
{
	return SimpleAuth(uri, status, "", "", postData);
}

std::string Request::SimpleAuth(std::string uri, int *status, std::string ID, std::string session, http::FormData postData)
{
	Request *request = new Request(uri);
	request->AddPostData(postData);
	request->AuthHeaders(ID, session);
	request->Start();
	return request->Finish(status);
}

std::string Request::GetStatusCodeDesc(int ret)
{
	switch (ret)
	{
	case 0:   return TPT_ES("Código de estado 0 (posible error)");
	case 100: return "Continue";
	case 101: return "Cambio de protocolo";
	case 102: return "Processing";
	case 200: return "OK";
	case 201: return "Created";
	case 202: return "Accepted";
	case 203: return TPT_ES("Información no oficial");
	case 204: return "No Content";
	case 205: return "Reset Content";
	case 206: return "Contenido parcial";
	case 207: return "Multi-Status";
	case 300: return TPT_ES("Múltiples opciones");
	case 301: return "Movido permanentemente";
	case 302: return "Found";
	case 303: return "See Other";
	case 304: return "Not Modified";
	case 305: return "Use Proxy";
	case 306: return "Switch Proxy";
	case 307: return TPT_ES("Redirección temporal");
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 402: return "Pago obligatorio";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return TPT_ES("Método no permitido");
	case 406: return "Not Acceptable";
	case 407: return TPT_ES("El proxy requiere autenticación");
	case 408: return TPT_ES("La solicitud agotó el tiempo de espera");
	case 409: return "Conflict";
	case 410: return "Gone";
	case 411: return "Se requiere la longitud del contenido";
	case 412: return TPT_ES("Falló una condición previa");
	case 413: return "El contenido de la solicitud es demasiado grande";
	case 414: return "URI demasiado larga";
	case 415: return "Tipo de contenido incompatible";
	case 416: return "Intervalo de datos no disponible";
	case 417: return "No se pudo cumplir el requisito de la solicitud";
	case 418: return "I'm a teapot";
	case 422: return "No se puede procesar el contenido";
	case 423: return "Locked";
	case 424: return TPT_ES("Falló una dependencia");
	case 425: return TPT_ES("Colección desordenada");
	case 426: return TPT_ES("Se requiere una actualización");
	case 444: return "No Response";
	case 450: return "Bloqueado por el control parental de Windows";
	case 499: return TPT_ES("El cliente cerró la solicitud");
	case 500: return "Error interno del servidor";
	case 501: return TPT_ES("Función sin implementar");
	case 502: return "Bad Gateway";
	case 503: return "Servicio no disponible";
	case 504: return TPT_ES("La puerta de enlace agotó el tiempo de espera");
	case 505: return TPT_ES("Versión de HTTP incompatible");
	case 506: return TPT_ES("La variante también negocia");
	case 507: return "Espacio insuficiente";
	case 509: return TPT_ES("Se superó el límite de ancho de banda");
	case 510: return "Not Extended";
	case 600: return "Error interno del cliente";
	case 601: return "Protocolo incompatible";
	case 602: return "Servidor no encontrado";
	case 603: return "Respuesta con formato incorrecto";
	case 604: return "Red no disponible";
	case 605: return TPT_ES("La solicitud agotó su tiempo de espera");
	case 606: return "Malformed URL";
	case 607: return TPT_ES("Conexión rechazada");
	case 608: return "Servidor proxy no encontrado";
	case 609: return TPT_ES("SSL: estado del certificado no válido");
	case 610: return "Cancelled by Shutdown";
	case 611: return "Demasiadas redirecciones";
	case 612: return TPT_ES("SSL: error de conexión");
	case 613: return TPT_ES("SSL: motor criptográfico no encontrado");
	case 614: return TPT_ES("SSL: no se pudo establecer el motor criptográfico predeterminado");
	case 615: return "SSL: problema con el certificado local";
	case 616: return "SSL: no se pudo usar el cifrado indicado";
	case 617: return TPT_ES("SSL: no se pudo iniciar el motor criptográfico");
	case 618: return "SSL: no se pudo cargar el archivo CACERT";
	case 619: return "SSL: no se pudo cargar el archivo CRL";
	case 620: return "SSL: no se pudo verificar al emisor del certificado";
	case 621: return TPT_ES("SSL: la clave pública no coincide con la esperada");
	default:  return TPT_ES("Código de estado desconocido");
	}
}
