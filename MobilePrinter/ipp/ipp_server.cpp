#include "pch.h"
#include "ipp_types.h"
#include "ipp_server.h"
#include "ipp_packet.h"
#include "CommonUtils.h"
#include "PrinterInfo.h"
#include "PrinterJobs.h"

#include <cpprest/http_listener.h>

#ifndef CPPREST_FORCE_HTTP_LISTENER_ASIO
#pragma comment(lib, "httpapi.lib")
#endif

//100-continue not supported till cpprestsdk 2.10.13
//#define ENABLE_100_CONTINUE

namespace ipp
{
	template<int OP> struct operation_traits {};
	template<> struct operation_traits<operations::Print_Job>
	{
		typedef PrintJobRequest request;
		typedef PrintJobResponse response;
	};
	template<> struct operation_traits<operations::Validate_Job>
	{
		typedef ValidateJobRequest request;
		typedef ValidateJobResponse response;
	};
	template<> struct operation_traits<operations::Get_Printer_Attributes>
	{
		typedef GetPrinterAttributesRequest request;
		typedef GetPrinterAttributesResponse response;
	};
	template<> struct operation_traits<operations::Get_Jobs>
	{
		typedef GetJobsRequest request;
		typedef GetJobsResponse response;
	};
	template<> struct operation_traits<operations::Cancel_Job>
	{
		typedef CancelJobRequest request;
		typedef CancelJobResponse response;
	};
	template<> struct operation_traits<operations::Get_Job_Attributes>
	{
		typedef GetJobAttributesRequest request;
		typedef GetJobAttributesResponse response;
	};
}

ipp_server::ipp_server(const std::string& i_host, const int32_t i_port, const config& i_config, CPrinterInfo* i_PrinterInfo) : m_PrinterInfo(i_PrinterInfo)
{
	assert(m_PrinterInfo);
	m_PrinterInfo->AddRef();

	m_PrinterJobs = m_PrinterInfo->GetPrinterJobs();
	assert(m_PrinterJobs);
	m_PrinterJobs->AddRef();

	m_config = i_config;

	web::http::experimental::listener::http_listener_config cfg;
	cfg.set_timeout(i_config.get_timeout());
	cfg.set_backlog(i_config.get_backlog());
	//cfg.set_ssl_context_callback(i_config.get_ssl_context_callback());

	web::http::uri ippuri = web::uri_builder{}
		.set_scheme(i_config.is_ssl_enabled() ? _XPLATSTR("https") : _XPLATSTR("http"))
		.set_host(utility::conversions::to_string_t(i_host))
		.set_port(i_port)
		.to_uri();
	m_uri = utility::conversions::to_utf8string(ippuri.to_string());

	try
	{
		m_server = std::make_unique<http_listener>(ippuri, cfg);
	}
	catch (...)
	{
		m_server.reset();
	}
	
	if (!m_server)
	{
		m_uri.clear();
		m_uri.shrink_to_fit();
		SAFE_RELEASE(m_PrinterJobs);
		SAFE_RELEASE(m_PrinterInfo);
		throw new std::exception();
	}

	init();
}

ipp_server::~ipp_server()
{
	SAFE_RELEASE(m_PrinterJobs);
	SAFE_RELEASE(m_PrinterInfo);
}

void ipp_server::start_listener()
{
	if (m_server)
	{
		m_server->open().wait();
	}
}

void ipp_server::stop_listener()
{
	if (m_server)
	{
		m_server->close().wait();
	}
}

template<const int OPERATION, typename Func>
void ipp_server::register_operation(Func i_callback)
{
	auto func = std::bind(i_callback, this, std::placeholders::_1, std::placeholders::_2);
	m_supported_operations[OPERATION] = [this, func](void* p1, void* p2)->int32_t {
		const typename ipp::operation_traits<OPERATION>::request& req = *static_cast<const typename ipp::operation_traits<OPERATION>::request*>(p1);
		typename ipp::operation_traits<OPERATION>::response& resp = *static_cast<typename ipp::operation_traits<OPERATION>::response*>(p2);
		return func(req, resp);
	};
}

void ipp_server::init()
{
	assert(m_server);

	m_server->support([this](web::http::http_request req) { (void)on_generic_request(req); });
	m_server->support(web::http::methods::GET, [this](web::http::http_request req) { (void)on_get_request(req); });
	m_server->support(web::http::methods::POST, [this](web::http::http_request req) { (void)on_post_request(req); });
	m_server->support(web::http::methods::HEAD, [this](web::http::http_request req) { (void)on_head_request(req); });
	m_server->support(web::http::methods::OPTIONS, [this](web::http::http_request req) { (void)on_options_request(req); });

	register_operation<ipp::operations::Print_Job>(&ipp_server::on_print_job);
	register_operation<ipp::operations::Validate_Job>(&ipp_server::on_validate_job);
	register_operation<ipp::operations::Get_Printer_Attributes>(&ipp_server::on_get_printer_attributes);
	register_operation<ipp::operations::Get_Jobs>(&ipp_server::on_get_jobs);
	register_operation<ipp::operations::Cancel_Job>(&ipp_server::on_cancel_job);
	register_operation<ipp::operations::Get_Job_Attributes>(&ipp_server::on_get_job_attributes);
}

Concurrency::task<void> ipp_server::on_generic_request(const web::http::http_request& req) const
{
	DBGLOG2("%ls\n%ls\n", time_now().c_str(), req.to_string().c_str());

	return req.reply(web::http::status_codes::BadRequest);
}

Concurrency::task<void> ipp_server::on_get_request(const web::http::http_request& req) const
{
	return validate_and_process_request(req, [](const web::http::http_request& req)
		{
			//TODO: web-server
			return req.reply(web::http::status_codes::NotFound);
		});
}

Concurrency::task<void> ipp_server::on_post_request(const web::http::http_request& req) const
{
	return validate_and_process_request(req, [this](const web::http::http_request& req)
		{
			return on_ipp_post_request(req);
		});
}

Concurrency::task<void> ipp_server::on_head_request(const web::http::http_request& req) const
{
	return validate_and_process_request(req, [](const web::http::http_request& req)
		{
			//web-server support
			utility::string_t content_type;
			utility::string_t request_uri = req.request_uri().to_string();
			if (imatch(request_uri, _XPLATSTR("/icon.png")))
			{
				content_type = _XPLATSTR("image/png");
			}
			else if (imatch(request_uri, _XPLATSTR("/")) ||
				imatch(request_uri, _XPLATSTR("/media")) ||
				imatch(request_uri, _XPLATSTR("/supplies")))
			{
				content_type = _XPLATSTR("text/html");
			}

			if (content_type.empty())
			{
				return req.reply(web::http::status_codes::NotFound);
			}

			web::http::http_response response(web::http::status_codes::OK);
			response.headers().add(_XPLATSTR("Content-Type"), content_type);
			return req.reply(response);
		});
}

Concurrency::task<void> ipp_server::on_options_request(const web::http::http_request& req) const
{
	return validate_and_process_request(req, [](const web::http::http_request& req)
		{
			return req.reply(web::http::status_codes::OK);
		});
}

Concurrency::task<void> ipp_server::validate_and_process_request(const web::http::http_request& req,
	const std::function<Concurrency::task<void>(const web::http::http_request&)>& on_valid_request) const
{
	DBGLOG2("%ls\n%ls\n", time_now().c_str(), req.to_string().c_str());

	web::http::status_code result = web::http::status_codes::OK;

	//if ((req.method() != web::http::methods::GET) &&
	//	(req.method() != web::http::methods::POST) &&
	//	(req.method() != web::http::methods::HEAD) &&
	//	(req.method() != web::http::methods::OPTIONS))
	//{
	//	result = web::http::status_codes::BadRequest;
	//}

	//if (req.version() < "1.1")
	//{
	//	result = web::http::status_codes::BadRequest;
	//}

	utility::string_t name;
	if (result == web::http::status_codes::OK)
	{
		if (req.request_uri().to_string().compare(_XPLATSTR("/ipp/print")) != 0)
		{
			result = web::http::status_codes::BadRequest;
		}
	}

	if (result == web::http::status_codes::OK)
	{
		if (!req.headers().match(_XPLATSTR("host"), name))
		{
			result = web::http::status_codes::BadRequest;
		}
	}

	if (result == web::http::status_codes::OK)
	{
		if (req.headers().match(_XPLATSTR("connection"), name) && imatch(name, _XPLATSTR("upgrade")))
		{
			result = web::http::status_codes::NotImplemented;
		}
	}

	if (result == web::http::status_codes::OK)
	{
		if ((!req.headers().match(_XPLATSTR("content-type"), name)) || !imatch(name, _XPLATSTR("application/ipp")))
		{
			result = web::http::status_codes::BadRequest;
		}
	}

	if (result == web::http::status_codes::OK)
	{
		if (req.headers().match(_XPLATSTR("expect"), name))
		{
			if (imatch(name, _XPLATSTR("100-continue")))
			{
#ifdef ENABLE_100_CONTINUE
				result = web::http::status_codes::Continue;
#else
				//result = web::http::status_codes::OK;
#endif
			}
			else
			{
				result = web::http::status_codes::ExpectationFailed;
			}
		}
	}
	
	if ((result != web::http::status_codes::OK) && (result != web::http::status_codes::Continue))
	{
		return req.reply(result);
	}
	else if (result == web::http::status_codes::Continue)
	{
		return req.reply(result).then([req, on_valid_request] { return on_valid_request(req); });
	}

	return on_valid_request(req);
}

Concurrency::task<void> ipp_server::on_ipp_post_request(const web::http::http_request& req) const
{
	//return req.extract_vector().then([req, this](const std::vector<unsigned char>& body)	//required to ignore exceptions from extract_vector: if thrown, this lambda is never called.
	return req.extract_vector().then([req, this](Concurrency::task<std::vector<unsigned char>> r_task)	//required to intercept exceptions from extract_vector: this lambda is always called.
	{
		std::vector<unsigned char> body;
		try
		{
			body = r_task.get();
		}
		catch (...)
		{
		}

		ipp::response response;
		if (process_ipp_request(body, response))
		{
			const std::vector<uint8_t>& content = response.GetData();
			if (content.empty())
			{
				return req.reply(web::http::status_codes::InternalError);
			}
			else
			{
				return req.reply(web::http::status_codes::OK, concurrency::streams::bytestream::open_istream(content), content.size(), _XPLATSTR("application/ipp"));
			}
		}
		else
		{
			return req.reply(web::http::status_codes::BadRequest);
		}
	});
}

bool ipp_server::process_ipp_request(const std::span<const uint8_t>& i_ipp_request, ipp::response& o_ipp_res) const
{
	ipp::request ippPacket;
	if (!ippPacket.parse(i_ipp_request.data(), i_ipp_request.size()))
	{
		return false;
	}

#ifdef _DEBUG
	if (m_config.save_ipp_data_to_file())
	{
		static time_t session_id = time(nullptr);
		wchar_t wszFileName[MAX_PATH];
		swprintf_s(wszFileName, L"Ipp_%llx\\Ipp_%llx_%d_%d.bin", uint64_t(session_id), uint64_t(session_id), ippPacket.GetRequestId(), ippPacket.GetOperationId());

		std::filesystem::path FileName = global_config::get().get_output_folder();
		FileName.append(L"ippdata");
		FileName.append(wszFileName);
		SaveFile(FileName.c_str(), i_ipp_request.data(), i_ipp_request.size());
	}
#endif

	DBGLOG("IPP %d.%d - request %d - operation %d (%s)", ippPacket.GetIppMajor(), ippPacket.GetIppMinor(), ippPacket.GetRequestId(), ippPacket.GetOperationId(), ipp::operations::get_description(ippPacket.GetOperationId()));

	o_ipp_res = {};

	int32_t status_code = ipp::status_codes::successful_ok;

	//See rfc3196 pag.
#if IPP_SUPPORTS_2_0
	if ((ippPacket.GetIppMajor() < 1) || (ippPacket.GetIppMajor() > 2))
#else
	if (ippPacket.GetIppMajor() != 1)
#endif
	{
		status_code = ipp::status_codes::server_error_version_not_supported;
	}
	//else if (ippPacket.GetRequestId() <= 0)	//See rfc3196
	//{
	//	status_code = ipp::status_codes::client_error_bad_request;
	//}

	if (status_code == ipp::status_codes::successful_ok)
	{
		//Validate operations before validating attributes (rfc3196)
		if (m_supported_operations.find(ippPacket.GetOperationId()) == m_supported_operations.end())
		{
			status_code = ipp::status_codes::server_error_operation_not_supported;
		}
	}

	if (status_code == ipp::status_codes::successful_ok)
	{
		//Validate sttributes
		if (!ippPacket.attributes().size())
		{
			status_code = ipp::status_codes::client_error_bad_request;
		}
		else if (ippPacket.attributes().find(ipp::tags::operation_attributes_tag) == ippPacket.attributes().end())
		{
			status_code = ipp::status_codes::client_error_bad_request;
		}
		/*else if (!ippPacked.AreAttributesInOrder())
		{
			status_code = ipp::status_codes::client_error_bad_request;
		}*/
	}

	if (status_code == ipp::status_codes::successful_ok)
	{
		switch (ippPacket.GetOperationId())
		{
		case ipp::operations::Print_Job:
			on_operation<ipp::operations::Print_Job>(ippPacket, o_ipp_res);
			break;

		case ipp::operations::Validate_Job:
			on_operation<ipp::operations::Validate_Job>(ippPacket, o_ipp_res);
			break;

		case ipp::operations::Get_Printer_Attributes:
			on_operation<ipp::operations::Get_Printer_Attributes>(ippPacket, o_ipp_res);
			break;

		case ipp::operations::Get_Jobs:
			on_operation<ipp::operations::Get_Jobs>(ippPacket, o_ipp_res);
			break;

		case ipp::operations::Cancel_Job:
			on_operation<ipp::operations::Cancel_Job>(ippPacket, o_ipp_res);
			break;

		case ipp::operations::Get_Job_Attributes:
			on_operation<ipp::operations::Get_Job_Attributes>(ippPacket, o_ipp_res);
			break;

		default:
			status_code = ipp::status_codes::server_error_operation_not_supported;
			break;
		}
	}
	
	if (status_code != ipp::status_codes::successful_ok)
	{
		fill_ipp_error_response(ippPacket, status_code, o_ipp_res);
	}
	
	return true;
}

void ipp_server::fill_ipp_error_response(const ipp::request& req, const int32_t i_status_code, ipp::response& o_ipp_res) const
{
	ipp::ErrorResponse resp;
	resp.opAttribs.attributes_charset = "utf-8";
	resp.opAttribs.attributes_natural_language = "en";

	req.build_response(i_status_code, resp, o_ipp_res);
}

namespace dbg
{
	static std::string ToString(const ipp::date_time& i_value);
	static std::string ToString(const ipp::variant& i_value);
	static std::string ToString(const ipp::collection& i_value);
	static std::string ToString(const std::vector<ipp::variant>& i_value);

	static std::string ToString(const ipp::date_time& i_value)
	{
		std::ostringstream oss;
		oss << i_value.year << "/" << i_value.month << "/" << i_value.day << ","
			<< i_value.hour << ":" << i_value.minute << ":" << i_value.second << "." << i_value.millisecond << ","
			<< (i_value.isPositive ? "+" : "-") << i_value.TZHour << ":" << i_value.TZMinute;
		return oss.str();
	};

	static std::string ToString(const ipp::collection& i_value)
		{
		std::ostringstream oss;
		oss << "{";
		for (auto& it : i_value)
			{
			oss << "{" << it.first << ": " << ToString(it.second) << "}";
		}
		oss << "}";
		return oss.str();
	};

	static std::string ToString(const std::vector<ipp::variant>& i_values)
	{
		std::ostringstream oss;
		oss << "[";
		for (auto it2 = i_values.begin(); it2 != i_values.end(); it2++)
		{
			oss << ToString(*it2);
			if ((it2 + 1) != i_values.end())
			{
				oss << ", ";
			}
		}
		oss << "]";
		return oss.str();
	}

	static std::string ToString(const ipp::variant& i_value)
	{
		std::ostringstream oss;
		switch (i_value.GetType())
		{
		case ipp::variant::Uninitialized: break;
		case ipp::variant::Unsupported: oss << "<unsupported>"; break;
		case ipp::variant::Unknown: oss << "<unknown>"; break;
		case ipp::variant::NoValue: oss << "<novalue>"; break;
			//case ipp::variant::Default: oss << "<default>"; break;
			//case ipp::variant::NotSettable: oss << "<notsettable>"; break;
			//case ipp::variant::DeleteAttr: oss << "<deleteattr>"; break;
			//case ipp::variant::AdminDefine: oss << "<admindefine>"; break;
		case ipp::variant::Int32: oss << i_value.GetInt32(); break;
		case ipp::variant::Bool: oss << i_value.GetBool(); break;
		case ipp::variant::Int32Range: oss << "(" << i_value.GetRange().first << "x" << i_value.GetRange().second << ")"; break;
		case ipp::variant::Resolutions: oss << "(" << std::get<0>(i_value.GetResolution()) << "x" << std::get<1>(i_value.GetResolution()) << ", " << std::get<2>(i_value.GetResolution()) << ")"; break;
		case ipp::variant::DateTime: oss << ToString(i_value.GetDate()); break;
		case ipp::variant::String: oss << i_value.GetString(); break;
		case ipp::variant::StringWithLanguage: oss << "(" << i_value.GetStringWithLanguage().text() << ", " << i_value.GetStringWithLanguage().language() << ")"; break;
		case ipp::variant::Collection: oss << ToString(i_value.GetCollection()); break;
		default: break;
		}

		return oss.str();
	}
}

template<const int OPERATION>
void ipp_server::on_operation(const ipp::request& i_ipp_req, ipp::response& o_ipp_res) const
{
	typename ipp::operation_traits<OPERATION>::request ippreq;
	int32_t status_code = i_ipp_req.to_request_description(ippreq);
	if (status_code != ipp::status_codes::successful_ok)
	{
		fill_ipp_error_response(i_ipp_req, status_code, o_ipp_res);
		return;
	}

	auto operation_func_it = m_supported_operations.find(OPERATION);
	if (operation_func_it == m_supported_operations.end())
	{
		status_code = ipp::status_codes::server_error_operation_not_supported;
		fill_ipp_error_response(i_ipp_req, status_code, o_ipp_res);
		return;
	}

	typename ipp::operation_traits<OPERATION>::response ippresp;
	status_code = operation_func_it->second(&ippreq, &ippresp);
	i_ipp_req.build_response(status_code, ippresp, o_ipp_res);
}

int32_t ipp_server::on_print_job(const ipp::PrintJobRequest& req, ipp::PrintJobResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	int32_t status_code = ipp::status_codes::successful_ok;

	auto Job = m_PrinterJobs->CreatePrinterJob(m_PrinterInfo->GetPrinterName(), req.opAttribs.job_name.c_str(), req.opAttribs.requesting_user_name.c_str(), [&](const PrinterJob&) {});

	if (Job)
	{
		// PrintJob may be executed on an async task, since docData contains all document data.
		const std::vector<uint8_t>& docData = req.docData;
		Job = m_PrinterJobs->PrintJob(Job->GetJobId(), docData, false, utf8_to_wchar(req.opAttribs.document_format).c_str());
		if (Job)
		{
			resp.jobAttribs.job_id = Job->GetJobId();
			resp.jobAttribs.job_state = Job->GetStatusIPP();
			resp.jobAttribs.job_state_message.assign(ipp::job_state::get_description(Job->GetStatusIPP()));
			resp.jobAttribs.job_state_reasons.push_back(Job->GetReasonIPP());
			//resp.jobAttribs.job_uri = "";
			//resp.jobAttribs.number_of_intervening_jobs = 0;
		}
		else
		{
			//Job not found! But it was created above!
			status_code = ipp::status_codes::server_error_internal_error;
		}
	}
	else
	{
		status_code = ipp::status_codes::server_error_internal_error;
	}

	return status_code;
}

int32_t ipp_server::on_validate_job(const ipp::ValidateJobRequest& req, ipp::ValidateJobResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	DBGLOG2("attributes_charset %s\n", req.opAttribs.attributes_charset.c_str());
	DBGLOG2("attributes_natural_language %s\n", req.opAttribs.attributes_natural_language.c_str());
	DBGLOG2("printer_uri %s\n", req.opAttribs.printer_uri.c_str());
	DBGLOG2("requesting_user_name %s\n", req.opAttribs.requesting_user_name.c_str());
	DBGLOG2("job_name %s\n", req.opAttribs.job_name.c_str());
	DBGLOG2("ipp_attribute_fidelity %s\n", req.opAttribs.ipp_attribute_fidelity ? "true" : "false");
	DBGLOG2("document_name %s\n", req.opAttribs.document_name.c_str());
	DBGLOG2("compression %s\n", req.opAttribs.compression.c_str());
	DBGLOG2("document_format %s\n", req.opAttribs.document_format.c_str());
	DBGLOG2("document_natural_language %s\n", req.opAttribs.document_natural_language.c_str());
	DBGLOG2("job_k_octets %d\n", (int)req.opAttribs.job_k_octets);
	DBGLOG2("job_impressions %d\n", (int)req.opAttribs.job_impressions);
	DBGLOG2("job_media_sheets %d\n", (int)req.opAttribs.job_media_sheets);

	return ipp::status_codes::successful_ok;
}

int32_t ipp_server::on_get_printer_attributes(const ipp::GetPrinterAttributesRequest& req, ipp::GetPrinterAttributesResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	int32_t status_code = ipp::status_codes::successful_ok;

	for (auto it = req.opAttribs.requested_attributes.cbegin(); it != req.opAttribs.requested_attributes.end(); it++)
	{
		const ipp::keyword& key = *it;

		auto values = get_supported_attribute(key);

		if (values.size())//is_attribute_supported(key))
		{
			resp.printerAttributes.attribs[key] = values;
		}
		else
		{
			resp.unsupportedAttribs.attribs.insert(std::make_pair(key, ipp::variant(ipp::variant::unsupported)));
			status_code = ipp::status_codes::successful_ok_ignored_or_substituted_attributes;
		}
	}

	return status_code;
}

static bool is_my_job_or_not(const ipp::GetJobsRequest::OperationAttributes& opAttribs, const PrinterJob& Job)
{
	return (!opAttribs.my_jobs || (Job.GetJobOriginatingUserName().compare(opAttribs.requesting_user_name) == 0));
}

static bool is_requested_job(const ipp::GetJobsRequest::OperationAttributes& opAttribs, const PrinterJob& Job)
{
	if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Completed) == 0)
	{
		if (Job.IsCompleted() && is_my_job_or_not(opAttribs, Job))
		{
#if IPP_SUPPORTS_2_0
			if (Job.GetStatusIPP() == ipp::job_state::completed)
#endif
			{
				return true;
			}
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_NotCompleted) == 0)
	{
		if ((!Job.IsCompleted()) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
#if IPP_SUPPORTS_2_0
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_All) == 0)
	{
		if (is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Aborted) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::aborted) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Canceled) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::canceled) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Completed) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::completed) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Fetchable) == 0)
	{
		if (((Job.GetStatusIPP() == ipp::job_state::pending) || (Job.GetStatusIPP() == ipp::job_state::pending_held)) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Pending) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::pending) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_PendingHeld) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::pending_held) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Processing) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::processing) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_ProcessingStopped) == 0)
	{
		if ((Job.GetStatusIPP() == ipp::job_state::processing_stopped) && is_my_job_or_not(opAttribs, Job))
		{
			return true;
		}
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_ProofPrint) == 0)
	{
		return false;
	}
	else if (opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Saved) == 0)
	{
		return false;
	}
#endif	//IPP_SUPPORTS_2_0
	else
	{
		ELOG("Unexpected which_jobs: %s", opAttribs.which_jobs.c_str());	//???
	}

	return false;
}

int32_t ipp_server::on_get_jobs(const ipp::GetJobsRequest& req, ipp::GetJobsResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	std::vector<std::shared_ptr<const PrinterJob>> allJobs;

#if IPP_SUPPORTS_2_0
	if ((req.opAttribs.which_jobs.compare(KEYWORD_WhichJobs_All) == 0) ||
		(req.opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Aborted) == 0) ||
		(req.opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Canceled) == 0) ||
		(req.opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Completed) == 0))
#else
	if (req.opAttribs.which_jobs.compare(KEYWORD_WhichJobs_Completed) == 0)
#endif
	{
		allJobs = m_PrinterJobs->GetAllJobs();
	}
	else
	{
		allJobs = m_PrinterJobs->GetActiveJobs();
	}

	int32_t status_code = ipp::status_codes::successful_ok;

	for (size_t j = 0; j < allJobs.size(); j++)
	{
		if (resp.jobsAttributes.size() >= static_cast<size_t>(req.opAttribs.limit))
		{
			break;
		}

		auto& job = *allJobs[j];
		
		if (is_requested_job(req.opAttribs, job))
		{
			bool bAddJob = false;
			ipp::GetJobsResponse::JobAttributes theJobAttributes;
			for (size_t a = 0; a < req.opAttribs.requested_attributes.size(); a++)
			{
				const std::string& attribute = req.opAttribs.requested_attributes[a];
				if (attribute == "job-id")
				{
					theJobAttributes.job_id = job.GetJobId();
					bAddJob = true;
				}
				//else if (attribute == "job-uri")
				//{
				//	theJobAttributes.job_uri = job.job_uri;
				//	bAddJob = true;
				//}
				else if (attribute == "job-state")
				{
					theJobAttributes.job_state = job.GetStatusIPP();
					bAddJob = true;
				}
				else if (attribute == "job-state-reasons")
				{
					theJobAttributes.job_state_reasons.push_back(job.GetReasonIPP());
					bAddJob = true;
				}
				else if (attribute == "job-state-message")
				{
					theJobAttributes.job_state_message.assign(ipp::job_state::get_description(job.GetStatusIPP()));
					bAddJob = true;
				}
				//else if (attribute == "number-of-intervening-jobs")
				//{
				//	theJobAttributes.job_uri = job.number_of_intervening_jobs;
				//	bAddJob = true;
				//}
				else
				{
					resp.unsupportedAttribs.attribs.insert(std::make_pair(attribute, ipp::variant(ipp::variant::unsupported)));
					status_code = ipp::status_codes::successful_ok_ignored_or_substituted_attributes;
				}
			}
			if (bAddJob)
			{
				resp.jobsAttributes.push_back(std::move(theJobAttributes));
			}
		}
	}

	return status_code;
}

int32_t ipp_server::on_cancel_job(const ipp::CancelJobRequest& req, ipp::CancelJobResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	int32_t status_code = ipp::status_codes::successful_ok;

	auto Job = m_PrinterJobs->CancelJob(req.opAttribs.job_id);
	if (Job)
	{
	}
	else
	{
		ELOG("Job not found: %d", (int)req.opAttribs.job_id);
		status_code = ipp::status_codes::client_error_bad_request;	//???
	}

	return status_code;
}

int32_t ipp_server::on_get_job_attributes(const ipp::GetJobAttributesRequest& req, ipp::GetJobAttributesResponse& resp) const
{
	resp.opAttribs.attributes_charset = req.opAttribs.attributes_charset;
	resp.opAttribs.attributes_natural_language = req.opAttribs.attributes_natural_language;
	//resp.opAttribs.status_message;
	//resp.opAttribs.detailed_status_message;

	int32_t status_code = ipp::status_codes::successful_ok;

	auto Job = m_PrinterJobs->FindJob(req.opAttribs.job_id);
	if (Job)
	{
		resp.jobAttributes.job_id = Job->GetJobId();
		resp.jobAttributes.job_state = Job->GetStatusIPP();
		resp.jobAttributes.job_state_message.assign(ipp::job_state::get_description(Job->GetStatusIPP()));
		resp.jobAttributes.job_state_reasons.push_back(Job->GetReasonIPP());
		//resp.jobAttributes.job_uri = "";
		//resp.jobAttributes.number_of_intervening_jobs = 0;

		if (Job->IsCompleted())
		{
			(void)m_PrinterJobs->FinishJob(req.opAttribs.job_id);
		}
	}
	else
	{
		ELOG("Job not found: %d", (int)req.opAttribs.job_id);
		status_code = ipp::status_codes::client_error_bad_request;	//???
	}

	return status_code;
}

std::vector<ipp::variant> ipp_server::get_supported_attribute(const ipp::keyword& key) const
{
	using namespace ipp;

	const int DEFAULT_MARGIN = 10;

	std::vector<ipp::variant> value;

	if (imatch(key, "ipp-versions-supported"))
	{
		value.push_back(variant("1.0"));
		value.push_back(variant("1.1"));
#if IPP_SUPPORTS_2_0
		value.push_back(variant("2.0"));
		//value.push_back(variant("2.1"));
		//value.push_back(variant("2.2"));
#endif
	}
	else if (imatch(key, "printer-make-and-model"))
	{
		std::wstring make_and_model;
		make_and_model = m_PrinterInfo->GetManufacturer();
		make_and_model += L" ";
		make_and_model += m_PrinterInfo->GetModel();

		value.push_back(variant(wchar_to_utf8(make_and_model)));
	}
	else if (imatch(key, "printer-name"))
	{
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetPrinterName())));
	}
	else if (imatch(key, "printer-uuid"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetURNUUIDAddress())));
#endif
	}
	else if (imatch(key, "printer-location"))
	{
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetPrinterLocation())));
	}
	else if (imatch(key, "printer-info"))
	{
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetPrinterInfo())));
	}
	else if (imatch(key, "printer-icons"))
	{
		//std::string icon = wchar_to_utf8(uri().to_string());
		//if (!ends_with(icon, "\\") && !ends_with(icon, "/"))
		//{
		//	icon += "/";
		//}
		//icon.append("icon.jpg");
		//value.push_back(variant(icon));
	}
	else if (imatch(key, "printer-uri-supported"))
	{
		value.push_back(variant(m_uri));
		//TODO: additional uri
	}
	else if (imatch(key, "uri-security-supported"))
	{
		value.push_back(variant(KEYWORD_UriSecurity_None));
		//TODO: additional uri-security
	}
	else if (imatch(key, "uri-authentication-supported"))
	{
		value.push_back(variant("none"));
		//TODO: additional uri-authentication
	}
	else if (imatch(key, "color-supported"))
	{
		value.push_back(variant(m_PrinterInfo->ColorSupported()));
	}
	else if (imatch(key, "copies-supported"))
	{
		range_of_integers range;
		range.first = m_PrinterInfo->GetMinNumCopies();
		range.second = m_PrinterInfo->GetMaxNumCopies();
		value.push_back(variant(range));
	}
	else if (imatch(key, "document-format-supported"))
	{
		value.push_back(variant("application/pdf"));
		//value.push_back(variant("image/pwg-raster"));
		//value.push_back(variant("application/PCLm"));
		//value.push_back(variant("application/vnd.ms-xpsdocument"));
		//value.push_back(variant("application/oxps"));
		//value.push_back(variant("image/jpeg"));
	}
	else if (imatch(key, "finishings-supported"))
	{
		value.push_back(variant(finishings::none));
	}
	else if (imatch(key, "job-account-id-supported"))
	{
		value.push_back(variant(false));
	}
	else if (imatch(key, "job-accounting-user-id-supported"))
	{
		value.push_back(variant(false));
	}
	else if (imatch(key, "job-password-supported"))
	{
		value.push_back(variant(0));//integer(0,255)
	}
	else if (imatch(key, "job-password-encryption-supported"))
	{
		value.push_back(variant("none"));//keyword
	}
	else if (imatch(key, "media-col-default"))
	{
		collection coll;
		collection mediasize;
		std::string media_size_name("iso_a4_210x297mm"), media_key;
		mediasize["x-dimension"].push_back(variant(21000));
		mediasize["y-dimension"].push_back(variant(29700));
		coll["media-size"].push_back(variant(mediasize));
		coll["media-left-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-right-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-top-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-bottom-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-size-name"].push_back(variant(media_size_name.c_str()));
		
		media_key = media_size_name;
#if IPP_SUPPORTS_2_0
		coll["media-source"].push_back(variant(KEYWORD_MediaSource_Main));
		coll["media-type"].push_back(variant(KEYWORD_MediaType_Auto));
		media_key += "_";
		media_key += KEYWORD_MediaSource_Main;
		media_key += "_";
		media_key += KEYWORD_MediaType_Auto;
#endif
		if (DEFAULT_MARGIN == 0)
		{
			media_key += "_borderless";
		}
		coll["media-key"].push_back(variant(media_key.c_str()));

		value.push_back(variant(coll));
	}
	else if (imatch(key, "media-col-ready"))
	{
		collection coll;
		collection mediasize;
		std::string media_size_name("iso_a4_210x297mm"), media_key;
		mediasize["x-dimension"].push_back(variant(21000));
		mediasize["y-dimension"].push_back(variant(29700));
		coll["media-size"].push_back(variant(mediasize));
		coll["media-left-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-right-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-top-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-bottom-margin"].push_back(variant(DEFAULT_MARGIN));
		coll["media-size-name"].push_back(variant(media_size_name.c_str()));

		media_key = media_size_name;
#if IPP_SUPPORTS_2_0
		coll["media-source"].push_back(variant(KEYWORD_MediaSource_Main));
		coll["media-type"].push_back(variant(KEYWORD_MediaType_Auto));
		media_key += "_";
		media_key += KEYWORD_MediaSource_Main;
		media_key += "_";
		media_key += KEYWORD_MediaType_Auto;
#endif
		if (DEFAULT_MARGIN == 0)
		{
			media_key += "_borderless";
		}
		coll["media-key"].push_back(variant(media_key.c_str()));
		value.push_back(variant(coll));
	}
	else if (imatch(key, "media-default"))
	{
		value.push_back(variant("iso_a4_210x297mm"));
	}
	else if (imatch(key, "media-left-margin-supported"))
	{
		value.push_back(variant(DEFAULT_MARGIN));
	}
	else if (imatch(key, "media-right-margin-supported"))
	{
		value.push_back(variant(DEFAULT_MARGIN));
	}
	else if (imatch(key, "media-top-margin-supported"))
	{
		value.push_back(variant(DEFAULT_MARGIN));
	}
	else if (imatch(key, "media-bottom-margin-supported"))
	{
		value.push_back(variant(DEFAULT_MARGIN));
	}
	else if (imatch(key, "media-size-supported"))
	{
		collection coll;
		coll["x-dimension"].push_back(variant(21000));
		coll["y-dimension"].push_back(variant(29700));
		value.push_back(variant(coll));
	}
	else if (imatch(key, "marker-colors"))
	{
		value.push_back(variant("#000000"));
		value.push_back(variant("#00ffff"));
		value.push_back(variant("#ff00ff"));
		value.push_back(variant("#ffff00"));
	}
	else if (imatch(key, "marker-high-levels"))
	{
		value.push_back(variant(100));
		value.push_back(variant(100));
		value.push_back(variant(100));
		value.push_back(variant(100));
	}
	else if (imatch(key, "marker-levels"))
	{
		value.push_back(variant(100));
		value.push_back(variant(100));
		value.push_back(variant(100));
		value.push_back(variant(100));
	}
	else if (imatch(key, "marker-low-levels"))
	{
		value.push_back(variant(0));
		value.push_back(variant(0));
		value.push_back(variant(0));
		value.push_back(variant(0));
	}
	else if (imatch(key, "marker-names"))
	{
		value.push_back(variant("black"));
		value.push_back(variant("cyan"));
		value.push_back(variant("magenta"));
		value.push_back(variant("yellow"));
	}
	else if (imatch(key, "marker-types"))
	{
		value.push_back(variant("ink-cartridge"));
		value.push_back(variant("ink-cartridge"));
		value.push_back(variant("ink-cartridge"));
		value.push_back(variant("ink-cartridge"));
	}
	else if (imatch(key, "media-source-supported"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_MediaSource_Main));
#endif
	}
	else if (imatch(key, "media-supported"))
	{
		value.push_back(variant("iso_a4_210x297mm"));
	}
	else if (imatch(key, "media-ready"))
	{
		value.push_back(variant("iso_a4_210x297mm"));
	}
	else if (imatch(key, "media-type-supported"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_MediaType_Auto));
#endif
	}
	else if (imatch(key, "output-bin-supported"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_OutputBin_FaceDown));
#endif
	}
	else if (imatch(key, "output-bin-default"))
	{
#if IPP_SUPPORTS_2_0
		//new
		value.push_back(variant(KEYWORD_OutputBin_FaceDown));
#endif
	}
	else if (imatch(key, "printer-output-tray"))
	{
#if IPP_SUPPORTS_2_0
	//new
	//Mandatory if "output-bin" is supported
	//value.push_back(variant(KEYWORD_OutputBin_FaceDown));
#endif
	}
	else if (imatch(key, "printer-input-tray"))
	{
#if IPP_SUPPORTS_2_0
	//new
	//Mandatory if media-source or media-source-properties are supported
	//value.push_back(variant(...));
#endif
	}
	else if (imatch(key, "printer-dns-sd-name"))
	{
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetUniquePrinterName())));
	}
	else if (imatch(key, "print-color-mode-supported"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_PrintColorMode_Auto));
		value.push_back(variant(KEYWORD_PrintColorMode_Color));
		value.push_back(variant(KEYWORD_PrintColorMode_Monochrome));
#endif
	}
	else if (imatch(key, "print-quality-supported"))
	{
		value.push_back(variant(print_quality::draft));
		value.push_back(variant(print_quality::normal));
		value.push_back(variant(print_quality::high));
	}
	else if (imatch(key, "print-quality-default"))
	{
		value.push_back(variant(print_quality::normal));
	}
	else if (imatch(key, "printer-resolution-supported"))
	{
		value.push_back(variant(std::make_tuple<int32_t, int32_t, int32_t>(300, 300, ipp::dot_per_inch)));
		value.push_back(variant(std::make_tuple<int32_t, int32_t, int32_t>(600, 600, ipp::dot_per_inch)));
	}
	else if (imatch(key, "sides-supported"))
	{
		value.push_back(variant("one-sided"));
	}
	else if (imatch(key, "printer-device-id"))
	{
		value.push_back(variant(wchar_to_utf8(m_PrinterInfo->GetCommandSet/*GetDeviceId*/())));
	}
	else if (imatch(key, "epcl-version-supported"))
	{
		value.push_back(variant("1.0"));
	}
	else if (imatch(key, "pclm-raster-back-side"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_PclmRasterBackSide_Normal));
#endif
	}
	else if (imatch(key, "pclm-strip-height-preferred"))
	{
		value.push_back(variant(255));	//??
	}
	else if (imatch(key, "pclm-compression-method-preferred"))
	{
		value.push_back(variant(KEYWORD_Compression_None));	//??
	}
	else if (imatch(key, "pclm-source-resolution-supported"))
	{
		value.push_back(variant(std::make_tuple<int32_t, int32_t, int32_t>(300, 300, ipp::dot_per_inch)));
		value.push_back(variant(std::make_tuple<int32_t, int32_t, int32_t>(600, 600, ipp::dot_per_inch)));
	}
	else if (imatch(key, "document-format-details-supported"))
	{
#if IPP_SUPPORTS_2_0 && defined(KEYWORD_DocumentFormatDetails_DocumentFormat)
		//"document-format-details" is now deprecated.
		value.push_back(variant(KEYWORD_DocumentFormatDetails_DocumentFormat));
#endif
	}
	else if (imatch(key, "number-up-supported"))
	{
		value.push_back(variant(range_of_integers(1, 1)));	//integer(1:MAX) | rangeOfInteger(1:MAX)
		//value.push_back(variant(1));
		//value.push_back(variant(2));
		//value.push_back(variant(4));
	}
	else if (imatch(key, "number-up-default"))
	{
		value.push_back(variant(1));
	}
	else if (imatch(key, "presentation-direction-number-up-supported"))
	{
		value.push_back(variant("toright-tobottom"));
	}
	else if (imatch(key, "presentation-direction-number-up-default"))
	{
		value.push_back(variant("toright-tobottom"));
	}
	else if (imatch(key, "pwg-raster-document-sheet-back"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_PwgRasterDocumentSheetBack_Normal));
#endif
	}
	else if (imatch(key, "print-scaling-supported"))
	{
#if IPP_SUPPORTS_2_0
		value.push_back(variant(KEYWORD_PrintScaling_None));
#endif
	}
	else if (imatch(key, "multiple-document-handling-supported"))
	{
		//new
		//value.push_back(variant("separate-documents-uncollated-copies"));
		//value.push_back(variant("separate-documents-collated-copies"));
		//value.push_back(variant("single-document"));
		//value.push_back(variant("single-document-new-sheet"));
	}
	else if (imatch(key, "multiple-document-handling-default"))
	{
		//new
		//value.push_back(variant("single-document"));
	}
	else if (imatch(key, "pdf-k-octets-supported"))
	{
		//new
		//value.push_back(variant(range_of_integers(0, INT_MAX)));
	}
	else if (imatch(key, "mopria-certified") || imatch(key, "mopria_certified"))
	{
		//new
		//value.push_back(variant(true));
		//value.push_back(variant("1.0"));
	}
	else if (imatch(key, "job-constraints-supported"))
	{
		//new
		//Mandatory if “job-resolvers-supported"
		//value.push_back(variant(false));
	}
	else if (imatch(key, "job-pages-per-set-supported"))
	{
		//new
		//value.push_back(variant(false));
	}
	else if (imatch(key, "job-creation-attributes-supported"))
	{
		//new
		//value.push_back(variant(false));
	}
	else if (imatch(key, "printer-state"))
	{
		value.push_back(variant(printer_state::idle));
	}
	else if (imatch(key, "printer-state-message"))
	{
		value.push_back(variant(printer_state::get_description(printer_state::idle)));
	}
	else if (imatch(key, "printer-state-reasons"))
	{
		value.push_back(variant(KEYWORD_PrinterStateReasons_None));
	}
	//Begin: sent by "Windows Internet Print Protocol Provider" (Windows 10 - 18363)
#if IPP_SUPPORTS_2_0
	else if (imatch(key, "printer-firmware-name"))
	{
		//value.push_back(variant("IPP"));	//Just a placeholder. PWG 5110.1
	}
	else if (imatch(key, "printer-firmware-string-version"))
	{
		//value.push_back(variant("2.0"));	//Just a placeholder. PWG 5110.1
	}
	else if (imatch(key, "print_wfds"))
	{
		//value.push_back(variant(true));	//Device discovery; to be added to dnssd service.
	}
	else if (imatch(key, "document-format-preferred") || imatch(key, "document-format-default"))
	{
		//value.push_back(variant("application/pdf"));
	}
#endif
	//End "Windows Internet Print Protocol Provider"
	
	DBGLOG("\t%s: %s", key.c_str(), dbg::ToString(value).c_str());

	return value;
}
