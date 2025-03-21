// MobilePrinter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "PrinterInfo.h"
#include "PrinterJobs.h"
#include "utils/CmdLineUtils.h"
#include "wsd/WSDPrinterService.h"
#include "dnssd/DnssdPrinterService.h"
#include "ipp/IppPrinterService.h"
#include "PrinterInterface.h"
#if __has_include(<pplx/threadpool.h>) && defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
#include <pplx/threadpool.h>
#if !defined(BOOST_EMUL_ENABLED)	//This macro may be declared through custom threadpool.h inclusion.
#define IPP_THREAD_POOL_SIZE_SUPPORTED 1
#endif
#endif

#ifndef IPP_THREAD_POOL_SIZE_SUPPORTED
#define IPP_THREAD_POOL_SIZE_SUPPORTED 0
#endif

//NOTE:
//if CPPREST_FORCE_HTTP_LISTENER_ASIO is not defined and cpprestsdk\http_server_httpsys.cpp is used, the application
//as to be executed as administrator (requireAdministrator).
//if CPPREST_FORCE_HTTP_LISTENER_ASIO is defined and cpprestsdk\http_server_asio.cpp + boost_emul.h are used, no 
//administrator privileges are required (asInvoker).
//See MobilePrinter Project
//		-> Properties 
//			-> Linker 
//				-> Manifest File 
//					-> UAC Execution Level 
//						-> requireAdministrator (instead of asInvoker default value).

//TODO:
//pnpxDeviceCategoryText: L"Printers Scanners MobilePrinter": specify 'MobilePrinter' for compatibility with not-wsprintv20 old printers.
//pnpxCompatibleIdText_0_0/1/2: L"http://schemas.microsoft.com/windows/2006/08/wdp/print/PrinterServiceType"

struct Options
{
	std::wstring m_printer_name;
	bool m_wsprint = true;
	bool m_ipp = true;
	bool m_ignore_small_xps_elements = true;
	bool m_ignore_small_xps_absolute_elements = false;
	bool m_autodetect_unknown_document_format = true;
	bool m_print_to_raw_printer = false;
	bool m_manage_ipp_firewall = false;
#if IPP_THREAD_POOL_SIZE_SUPPORTED
	int m_ipp_thread_pool_size = 5;
#endif
#if IPP_WINHTTP_SSL_SUPPORTED
	bool m_enable_ssl = false;
	std::wstring m_certificate_hash;
	std::wstring m_certificate_subject;
	std::wstring m_certificate_store;
#endif
	LogType m_log_level = LogType::LogDefault;

	//Debug only
	bool m_force_save_to_file = false;
	bool m_save_ipp_data_to_file = false;
	std::wstring m_output_folder;
};

//Forward declaration
bool parse_commandline(int argc, const wchar_t** argv, Options& o_options);
void print_usage();

bool g_bExit = false;
void wait_for_key_press()
{
	static PHANDLER_ROUTINE Handler = [](DWORD CtrlType)->BOOL { g_bExit = true; return TRUE; };
	SetConsoleCtrlHandler(Handler, TRUE);

	while (!getc(stdin) && !g_bExit)
	{
		Sleep(100);
	}
}

int wmain(int argc, const wchar_t** argv)
{
	DBGLOG("Start");

#ifdef _DEBUG
	//extern void callTests();
	//extern void exportConstants();

	//callTests();
	//exportConstants();
#endif
	
	Options options;

	if (!parse_commandline(argc, argv, options))
	{
		print_usage();
		DBGLOG("\nEnd");
		return 0;
	}

	s_minLogType = options.m_log_level;
	global_config::get().setup(options.m_output_folder);

	PrintToXpsPrintDocumentPrinter::Config(options.m_ignore_small_xps_elements, options.m_ignore_small_xps_absolute_elements);
	CPrinterJobs::Config(options.m_autodetect_unknown_document_format, options.m_print_to_raw_printer, options.m_force_save_to_file);
	CIppPrinterService::Config(options.m_manage_ipp_firewall, options.m_save_ipp_data_to_file);
#if IPP_WINHTTP_SSL_SUPPORTED
	CIppPrinterService::ConfigSSL(options.m_enable_ssl, options.m_certificate_hash, options.m_certificate_subject, options.m_certificate_store);
	CDnssdPrinterService::ConfigSSL(options.m_enable_ssl);
#endif

#if IPP_THREAD_POOL_SIZE_SUPPORTED
	if (options.m_ipp)
	{
		//Default cpprestsdk value is m_ipp_thread_pool_size = 40.
		crossplat::threadpool::initialize_with_threads(options.m_ipp_thread_pool_size);
	}
#endif

	if (options.m_wsprint)
	{
		DWORD dwMessageSize = 1024 * 1024;
		HRESULT hr = WSDSetConfigurationOption(WSDAPI_OPTION_MAX_INBOUND_MESSAGE_SIZE, &dwMessageSize, sizeof(dwMessageSize));

		if (options.m_log_level == LogType::LogDebug)
		{
			DWORD dwEnable = 0xFFFFFFFF;
			hr = WSDSetConfigurationOption(WSDAPI_OPTION_TRACE_XML_TO_DEBUGGER, &dwEnable, sizeof(dwEnable));
		}
	}

	const wchar_t* pPrinterName = options.m_printer_name.c_str();

	CPrinterInfo* pPrinter = CPrinterInfo::Create(pPrinterName);
	if (pPrinter != nullptr)
	{
		std::vector<IPrinterService*> services;

		if (options.m_wsprint)
		{
			services.push_back(CWSDPrinterService::Create());
		}

		if (options.m_ipp)
		{
			services.push_back(CIppPrinterService::Create());
			services.push_back(CDnssdPrinterService::Create());
		}

		for (size_t i = 0; i < services.size(); i++)
		{
			IPrinterService* pPS = services.at(i);
			if (pPS != nullptr)
			{
				if (pPS->Init(pPrinter))
				{
					pPS->Start();
				}
				else
				{
					pPS->Release();
					services.at(i) = nullptr;
				}
			}
		}

		LOG2("\nWaiting for printer jobs ... press 'Enter' to quit.\n\n");

		if (std::any_of(services.begin(), services.end(), [](IPrinterService* pPS) { return (pPS != nullptr); }))
		{
			wait_for_key_press();
		}

		LOG2("\nShutting down...\n\n");

		for (size_t i = 0; i < services.size(); i++)
		{
			IPrinterService* pPS = services.at(i);
			if (pPS != nullptr)
			{
				pPS->Stop();

				pPS->Uninit();

				pPS->Release();
			}
		}
		services.clear();

		SAFE_RELEASE(pPrinter);
	}
	else
	{
		ELOG("Error initializing printer %ls", pPrinterName);
	}

	DBGLOG("End");

	return 0;
}

bool parse_commandline(int argc, const wchar_t** argv, Options& o_options)
{
	if (argc < 2)
	{
		return false;
	}

	//Reset options to default values.
	o_options = {};
	o_options.m_printer_name = argv[1];

	bool bEnable = false;
	const wchar_t* pParam = nullptr;

	for (int i = 2; i < argc; i++)
	{
		const wchar_t* option = &argv[i][0];
		if (option[0] == L'-')
		{
#if IPP_THREAD_POOL_SIZE_SUPPORTED
			if (cmdline::has_int32_param(option, L"-ipp-pool-size", &o_options.m_ipp_thread_pool_size))
			{
				o_options.m_ipp_thread_pool_size = std::min(std::max(o_options.m_ipp_thread_pool_size, 5), 100);
				continue;
			}
#endif	//IPP_THREAD_POOL_SIZE_SUPPORTED
#if IPP_WINHTTP_SSL_SUPPORTED
			if (cmdline::has_param_value(option, L"-certificate-hash", &pParam))
			{
				o_options.m_certificate_hash.assign(pParam);
				continue;
			}
			else if (cmdline::has_param_value(option, L"-certificate-subject", &pParam))
			{
				o_options.m_certificate_subject.assign(pParam);
				continue;
			}
			else if (cmdline::has_param_value(option, L"-certificate-store", &pParam))
			{
				o_options.m_certificate_store.assign(pParam);
				continue;
			}
#endif
			if (cmdline::is_bool_switch(option, L"-wsprint", &bEnable))
			{
				o_options.m_wsprint = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-ipp", &bEnable))
			{
				o_options.m_ipp = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-ignore-small-xps-elements", &bEnable))
			{
				o_options.m_ignore_small_xps_elements = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-ignore-small-xps-abs-elements", &bEnable))
			{
				o_options.m_ignore_small_xps_absolute_elements = bEnable;
			}
			else if (cmdline::has_param_value(option, L"-log", &pParam))
			{
				if (_wcsicmp(pParam, L"default") == 0)
				{
					o_options.m_log_level = LogType::LogDefault;
				}
				else if (_wcsicmp(pParam, L"debug") == 0)
				{
					o_options.m_log_level = LogType::LogDebug;
				}
				else if (_wcsicmp(pParam, L"verbose") == 0)
				{
					o_options.m_log_level = LogType::LogVerbose;
				}
				else
				{
					LOG2("Invalid option: %ls\n\n", option);
					return false;
				}
			}
			else if (cmdline::is_bool_switch(option, L"-print-to-raw-printer", &bEnable))
			{
				o_options.m_print_to_raw_printer = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-autodetect-unknown-document-format", &bEnable))
			{
				o_options.m_autodetect_unknown_document_format = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-manage-ipp-firewall", &bEnable))
			{
				o_options.m_manage_ipp_firewall = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-force-save-to-file", &bEnable))
			{
				o_options.m_force_save_to_file = bEnable;
			}
			else if (cmdline::is_bool_switch(option, L"-save-ipp-data-to-file", &bEnable))
			{
				o_options.m_save_ipp_data_to_file = bEnable;
			}
#if IPP_WINHTTP_SSL_SUPPORTED
			else if (cmdline::is_bool_switch(option, L"-ssl", &bEnable))
			{
				o_options.m_enable_ssl = bEnable;
			}
#endif
			else if (cmdline::has_param_value(option, L"-output-folder", &pParam))
			{
				o_options.m_output_folder = pParam;
			}
			else
			{
				LOG2("Invalid option: %ls\n\n", option);
				return false;
			}
		}
		else
		{
			LOG2("Invalid option: %ls\n\n", option);
			return false;
		}
	}

	if (!o_options.m_wsprint && !o_options.m_ipp)
	{
		return false;
	}

	if (o_options.m_output_folder.empty())
	{
		o_options.m_output_folder = GetExecutablePath();
	}

#ifdef _DEBUG
	o_options.m_log_level = LogType::LogDebug;
#endif

	return true;
}

void print_usage()
{
	std::wstring usage;
	usage = L"\n\nUsage: MobilePrinter \"printer name\" [-options]\n\n";
	auto printers = CPrinterInfo::EnumeratePrinters();
	if (printers.size() == 1)
	{
		usage += L"1 printer available\n";
	}
	else
	{
		wchar_t num[32] = L"";
		_itow_s(static_cast<int>(printers.size()), num, 10);
		usage += num;
		usage += L" printers available\n";
	}

	for (auto it = printers.cbegin(); it != printers.cend(); it++)
	{
		usage += L"\t\"" + (*it) + L"\"\n";
	}

	usage += L"\n"
		L"-(no-)wsprint: enable/disable wsprint service\n"
		L"-(no-)ipp: enable/disable ipp service\n"
		L"-(no-)autodetect-unknown-document-format: try to auto-detect document format if unknown (currently, only pdf and xps files).\n"
		L"-(no-)ignore-small-xps-elements: some xps can't be printed if they contain very small elements.\n"
		L"-(no-)ignore-small-xps-abs-elements: some xps can't be printed if they contain very small elements.\n"
		L"-(no-)print-to-raw-printer: directly send the document to the printer queue as raw printer, regardless of the document format.\n"
		L"-(no-)manage-ipp-firewall: automatically open and close Firewall inbound private TCP traffic on port 631 for ipp service.\n"
#if IPP_THREAD_POOL_SIZE_SUPPORTED
		L"-ipp-pool-size=[5,100]: number of threads used by the ipp service (advanced)\n"
#endif
#if IPP_WINHTTP_SSL_SUPPORTED
		L"-(no-)ssl: enable or disable ssl support for ipp service.\n"
		L"-certificate-hash=<certhash>: hexadecimal hash of the certificate to find, for SSL binding. If missing, certificate-subject is used instead.\n"
		L"-certificate-subject=<certname>: subject name of the certificate to find, for SSL binding. If missing, SSL has to be configured before running the application.\n"
		L"-certificate-store=<certstore>: name of the store under Local Machine where certificate is present, for SSL binding (Root, My, ...). If missing, SSL has to be configured before running the application.\n"
#endif
		L"-log=[debug/verbose/default]: enable different log levels.\n"
		L"-output-folder=<folder where to save temporary data>\n"
		L"\n"
		L"-no-wsprint and -no-ipp can't be declared together (otherwise no printer service is enabled).\n"
		L"\n"
		L"Default parameters if not specified:\n"
		L"\t-wsprint\n"
		L"\t-ipp\n"
		L"\t-autodetect-unknown-document-format\n"
		L"\t-ignore-small-xps-elements\n"
		L"\t-no-ignore-small-xps-abs-elements\n"
		L"\t-no-print-to-raw-printer\n"
		L"\t-no-manage-ipp-firewall\n"
#if IPP_THREAD_POOL_SIZE_SUPPORTED
		L"\t-ipp-pool-size=5\n"
#endif
#if IPP_WINHTTP_SSL_SUPPORTED
		L"\t-no-ssl\n"
#endif
		L"\t-log=default\n";

	LOG2("%ls\n", usage.c_str());
}
