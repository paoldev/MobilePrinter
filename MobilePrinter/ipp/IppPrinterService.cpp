#include "pch.h"
#include "IppPrinterService.h"
#include "PrinterInfo.h"
#include "ipp_server.h"
#include "../utils/SSLUtils.h"
#include "../utils/FirewallUtils.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Networking.Connectivity.h>

constexpr int IPP_PORT = 631;
constexpr const wchar_t* FIREWALL_RULE_631 = L"MobilePrinter-631";

struct IppConfig
{
	bool m_bManageFirewall = false;
	bool m_bEnableSSL = false;
	std::wstring m_wsCertHash;
	std::wstring m_wsCertSubjectName;
	std::wstring m_wsStoreName;
#ifdef _DEBUG
	bool m_bSaveIppDataToFile = false;
#endif
};

IppConfig s_ippConfig = {};

void CIppPrinterService::Config(const bool i_bManageFirewall, const bool i_bDebugSaveIppDataToFile)
{
	s_ippConfig.m_bManageFirewall = i_bManageFirewall;
#ifdef _DEBUG
	s_ippConfig.m_bSaveIppDataToFile = i_bDebugSaveIppDataToFile;
#else
	(void)i_bDebugSaveIppDataToFile;
#endif	//_DEBUG
}

void CIppPrinterService::ConfigSSL(const bool i_bEnableSSL, const std::wstring& i_wsCertHash, const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName)
{
	s_ippConfig.m_bEnableSSL = i_bEnableSSL;
	s_ippConfig.m_wsCertHash = i_wsCertHash;
	s_ippConfig.m_wsCertSubjectName = i_wsCertSubjectName;
	s_ippConfig.m_wsStoreName = i_wsStoreName;
}

IPrinterService* CIppPrinterService::Create()
{
	return new CIppPrinterService();
}

CIppPrinterService::CIppPrinterService() : m_PrinterInfo(nullptr)
{
}

CIppPrinterService::~CIppPrinterService()
{
	Uninit();
}

bool CIppPrinterService::Init(CPrinterInfo* pPrinter)
{
	if (pPrinter == nullptr)
	{
		return false;
	}

	if (pPrinter->GetPrinterJobs() == nullptr)
	{
		return false;
	}

	winrt::Windows::Networking::HostName hostName(nullptr);
	auto connectionProfile = winrt::Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile();
	if ((connectionProfile != nullptr) && (connectionProfile.NetworkAdapter() != nullptr))
	{
		auto hosts = winrt::Windows::Networking::Connectivity::NetworkInformation::GetHostNames();
		for (unsigned int i = 0; i < hosts.Size(); ++i)
		{
			auto name = hosts.GetAt(i);
			//LOG("Host: %ls - %d", name.RawName().data(), (int)name.Type());
			/*if (name.Type() == winrt::Windows::Networking::HostNameType::Ipv4)
			{
				hostName = name;
				break;
			}*/
			if ((name.IPInformation() != nullptr) && (name.IPInformation().NetworkAdapter() != nullptr) && (name.IPInformation().NetworkAdapter().NetworkAdapterId() == connectionProfile.NetworkAdapter().NetworkAdapterId()))
			{
				hostName = name;
				break;
			}
		}
	}
	if (hostName == nullptr)
	{
		return false;
	}

	if (s_ippConfig.m_bManageFirewall)
	{
		const wchar_t* pwszExecutablePath = nullptr;
#ifndef CPPREST_FORCE_HTTP_LISTENER_ASIO
		pwszExecutablePath = L"System"; //WinHTTP requires System as executable path in order to open port 631.
#endif
		CFirewallHelper::AddInboundPrivateTCPRule(FIREWALL_RULE_631, pwszExecutablePath, L"631");
	}

	if (s_ippConfig.m_bEnableSSL)
	{
#if IPP_WINHTTP_SSL_SUPPORTED
		CSSLUtils::Config sslCfg;
		sslCfg.m_type = CSSLUtils::Type::eSsl;
		sslCfg.m_port = 631;
		sslCfg.m_guidAppId = GUID_NULL;
		sslCfg.m_wsCertHash = s_ippConfig.m_wsCertHash;
		sslCfg.m_wsCertSubjectName = s_ippConfig.m_wsCertSubjectName;
		sslCfg.m_wsStoreName = s_ippConfig.m_wsStoreName;
		m_sslUtils.reset(new CSSLUtils(sslCfg));
#endif
	}

	std::string ipp_host = wchar_to_utf8(hostName.RawName().c_str());
	//std::string ipp_host = "localhost";
	//std::string ipp_host = "127.0.0.1";
	//std::string ipp_host = "0.0.0.0";

	ipp_server::config cfg;
	cfg.enable_ssl(s_ippConfig.m_bEnableSSL);
	//cfg.set_backlog(1);
#ifdef _DEBUG
	cfg.save_ipp_data_to_file(s_ippConfig.m_bSaveIppDataToFile);
#endif

	try
	{
		m_ippserver = std::make_unique<ipp_server>(ipp_host, IPP_PORT, cfg, pPrinter);
	}
	catch (...)
	{
		Uninit();
		return false;
	}

	m_PrinterInfo = pPrinter;
	m_PrinterInfo->AddRef();

	DBGLOG("%s", m_ippserver.get()->uri().c_str());

	return true;
}

void CIppPrinterService::Uninit()
{
	Stop();

	m_ippserver.reset();
	SAFE_RELEASE(m_PrinterInfo);

	m_sslUtils.reset();

	if (s_ippConfig.m_bManageFirewall)
	{
		CFirewallHelper::RemoveRule(FIREWALL_RULE_631);
	}
}

void CIppPrinterService::Start()
{
	if (m_ippserver)
	{
		LOG2("[IPP] Starting %s ... ", m_ippserver.get()->uri().c_str());
		m_ippserver->start_listener();
		LOG2("done.\n");
	}
}

void CIppPrinterService::Stop()
{
	if (m_ippserver)
	{
		LOG2("[IPP] Stopping %s ... ", m_ippserver.get()->uri().c_str());
		m_ippserver->stop_listener();
		LOG2("done.\n");
	}
}
