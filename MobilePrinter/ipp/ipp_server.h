#pragma once

#include <cpprest/http_listener.h>

class CPrinterInfo;
class CPrinterJobs;

namespace ipp
{
	typedef std::string keyword;

	class packet;
	class request;
	class response;
	class variant;
	class PrintJobRequest;
	class PrintJobResponse;
	class ValidateJobRequest;
	class ValidateJobResponse;
	class GetPrinterAttributesRequest;
	class GetPrinterAttributesResponse;
	class GetJobsRequest;
	class GetJobsResponse;
	class CancelJobRequest;
	class CancelJobResponse;
	class GetJobAttributesRequest;
	class GetJobAttributesResponse;
}

class ipp_server : public web::http::experimental::listener::http_listener
{
public:

	typedef web::http::experimental::listener::http_listener http_listener;
	
	class config : public web::http::experimental::listener::http_listener_config
	{
#ifdef _DEBUG
	public:
		bool save_ipp_data_to_file() const { return m_bSaveIppDataToFile; }
		void save_ipp_data_to_file(bool bSave) { m_bSaveIppDataToFile = bSave; }

	private:
		bool m_bSaveIppDataToFile = false;
#endif
	};
	
	ipp_server(const web::http::uri& i_address, const config& i_config, CPrinterInfo* i_PrinterInfo);
	~ipp_server();

private:

	void init();

	pplx::task<void> on_get_request(web::http::http_request req) const;
	pplx::task<void> on_post_request(web::http::http_request req) const;
	pplx::task<void> on_head_request(web::http::http_request req) const;
	pplx::task<void> on_options_request(web::http::http_request req) const;
	pplx::task<void> on_generic_request(web::http::http_request req) const;
	pplx::task<void> on_ipp_post_request(web::http::http_request req) const;
	pplx::task<void> validate_and_process_request(web::http::http_request req, const std::function<pplx::task<void>(web::http::http_request)>& on_valid_request) const;

	int32_t on_print_job(const ipp::PrintJobRequest& req, ipp::PrintJobResponse& resp) const;
	int32_t on_validate_job(const ipp::ValidateJobRequest& req, ipp::ValidateJobResponse& resp) const;
	int32_t on_get_printer_attributes(const ipp::GetPrinterAttributesRequest& req, ipp::GetPrinterAttributesResponse& resp) const;
	int32_t on_get_jobs(const ipp::GetJobsRequest& req, ipp::GetJobsResponse& resp) const;
	int32_t on_cancel_job(const ipp::CancelJobRequest& req, ipp::CancelJobResponse& resp) const;
	int32_t on_get_job_attributes(const ipp::GetJobAttributesRequest& req, ipp::GetJobAttributesResponse& resp) const;

	template<const int OPERATION>
	pplx::task<void> on_operation(const ipp::request& i_ipp_req) const;

	template<const int OPERATION, typename Func>
	void register_operation(Func i_callback);
	
	pplx::task<void> send_ipp_error(const ipp::request& req, const int32_t i_status_code) const;

	std::vector<ipp::variant> get_supported_attribute(const ipp::keyword& key) const;

	CPrinterInfo* m_PrinterInfo;
	CPrinterJobs* m_PrinterJobs;
	std::map<int32_t, std::function<int32_t(void*, void*)>> m_supported_operations;

#ifdef _DEBUG
	bool m_bSaveIppDataToFile = false;
#endif
};
