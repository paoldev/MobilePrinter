#pragma once

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

namespace web::http
{
	class http_request;
	namespace experimental::listener
	{
		class http_listener;
	}
}

class ipp_server
{
public:

	class config
	{
	public:
		void set_timeout(const std::chrono::seconds& i_timeout) { m_timeout = i_timeout; }
		void set_backlog(const int i_backlog) { m_backlog = i_backlog; }
		void enable_ssl(bool i_enable) { m_ssl_enabled = i_enable; }	//experimental; for winhttp only.

		const std::chrono::seconds& get_timeout() const { return m_timeout; }
		int get_backlog() const { return m_backlog; }
		bool is_ssl_enabled() const { return m_ssl_enabled; }	//experimental; for winhttp only.

	private:
		std::chrono::seconds m_timeout = std::chrono::seconds(120);
		int m_backlog = 0;
		bool m_ssl_enabled = false;	//experimental; for winhttp only.

#ifdef _DEBUG
	public:
		bool save_ipp_data_to_file() const { return m_bSaveIppDataToFile; }
		void save_ipp_data_to_file(bool bSave) { m_bSaveIppDataToFile = bSave; }

	private:
		bool m_bSaveIppDataToFile = false;
#endif
	};
	
	ipp_server(const std::string& i_host, const int32_t i_port, const config& i_config, CPrinterInfo* i_PrinterInfo);
	~ipp_server();

	void start_listener();
	void stop_listener();

	const std::string& uri() const { return m_uri; }

private:

	void init();

	Concurrency::task<void> on_get_request(const web::http::http_request& req) const;
	Concurrency::task<void> on_post_request(const web::http::http_request& req) const;
	Concurrency::task<void> on_head_request(const web::http::http_request& req) const;
	Concurrency::task<void> on_options_request(const web::http::http_request& req) const;
	Concurrency::task<void> on_generic_request(const web::http::http_request& req) const;
	Concurrency::task<void> on_ipp_post_request(const web::http::http_request& req) const;
	Concurrency::task<void> validate_and_process_request(const web::http::http_request& req, const std::function<Concurrency::task<void>(const web::http::http_request&)>& on_valid_request) const;

	int32_t on_print_job(const ipp::PrintJobRequest& req, ipp::PrintJobResponse& resp) const;
	int32_t on_validate_job(const ipp::ValidateJobRequest& req, ipp::ValidateJobResponse& resp) const;
	int32_t on_get_printer_attributes(const ipp::GetPrinterAttributesRequest& req, ipp::GetPrinterAttributesResponse& resp) const;
	int32_t on_get_jobs(const ipp::GetJobsRequest& req, ipp::GetJobsResponse& resp) const;
	int32_t on_cancel_job(const ipp::CancelJobRequest& req, ipp::CancelJobResponse& resp) const;
	int32_t on_get_job_attributes(const ipp::GetJobAttributesRequest& req, ipp::GetJobAttributesResponse& resp) const;

	template<const int OPERATION>
	void on_operation(const ipp::request& i_ipp_req, ipp::response& o_ipp_res) const;

	template<const int OPERATION, typename Func>
	void register_operation(Func i_callback);

	bool process_ipp_request(const std::span<const uint8_t>& i_ipp_request, ipp::response& o_ipp_res) const;
	void fill_ipp_error_response(const ipp::request& req, const int32_t i_status_code, ipp::response& o_ipp_res) const;

	std::vector<ipp::variant> get_supported_attribute(const ipp::keyword& key) const;

	CPrinterInfo* m_PrinterInfo = nullptr;
	CPrinterJobs* m_PrinterJobs = nullptr;
	std::map<int32_t, std::function<int32_t(void*, void*)>> m_supported_operations;

	using http_listener = web::http::experimental::listener::http_listener;

	std::string m_uri;
	config m_config;
	std::unique_ptr<http_listener> m_server;
};
