#include "pch.h"
#include "DnssdPrinterService.h"
#include "DnssdService.h"

struct DnssdPrinterConfig
{
	bool m_bEnableSSL = false;
};

DnssdPrinterConfig s_dnssdPrinterConfig = {};

void CDnssdPrinterService::ConfigSSL(const bool i_bEnableSSL)
{
	s_dnssdPrinterConfig.m_bEnableSSL = i_bEnableSSL;
}

IPrinterService* CDnssdPrinterService::Create()
{
	return new CDnssdPrinterService();
}

CDnssdPrinterService::CDnssdPrinterService() : m_PrinterInfo(nullptr)
{
}

CDnssdPrinterService::~CDnssdPrinterService()
{
	Uninit();
}

bool CDnssdPrinterService::Init(CPrinterInfo* pPrinter)
{
	if (pPrinter == nullptr)
	{
		return false;
	}

	m_PrinterInfo = pPrinter;
	m_PrinterInfo->AddRef();

	m_printerName = m_PrinterInfo->GetPrinterName();

	const wchar_t* pInstanceName = m_PrinterInfo->GetUniquePrinterName().c_str();

	// See https://tools.ietf.org/html/rfc7472
	struct PortType
	{
		const wchar_t* ServiceAndDomain;
		const wchar_t* Port;
	} ports[] =
	{
		{ L"._printer._tcp.local.", L"515"},
		{ L"._ipp._tcp.local.", L"631"}, //For IPP discovery
		{ L"._ipp._tcp.local.", L"80"},	//For WSD discovery
		{ L"._ipp._tcp,_print.local.", L"631"},	//ipp everywhere
		{ L"._ipps._tcp.local.", L"631"},//ipps 
		{ L"._ipps._tcp,_print.local.", L"631"},//ipps everywhere
		{ L"._pdl-datastream._tcp.local.", L"9100"},
		{ L"._http._tcp,_printer.local.", L"631" /*L"80"*/},	//web server
	};

	int serviceTypes[] = { s_dnssdPrinterConfig.m_bEnableSSL ? 4 : 1 /*, 2*/ };

	std::map<std::wstring, std::wstring> txtAttribs;
	txtAttribs.insert(std::make_pair(L"rp", L"ipp/print"));
	txtAttribs.insert(std::make_pair(L"txtvers", L"1")); //deprecated
	txtAttribs.insert(std::make_pair(L"qtotal", L"1")); //deprecated
	txtAttribs.insert(std::make_pair(L"ty", m_PrinterInfo->GetManufacturer() + L" " + m_PrinterInfo->GetModel())); //deprecated
	txtAttribs.insert(std::make_pair(L"UUID", GuidToString(m_PrinterInfo->GetUUID())));
	txtAttribs.insert(std::make_pair(L"DUUID", GuidToString(m_PrinterInfo->GetDUUID())));
	txtAttribs.insert(std::make_pair(L"pdl", L"application/pdf,application/vnd.ms-xpsdocument"));
	txtAttribs.insert(std::make_pair(L"usb_CMD", m_PrinterInfo->GetCommandSet())); //deprecated
	txtAttribs.insert(std::make_pair(L"usb_MFG", m_PrinterInfo->GetManufacturer())); //deprecated
	txtAttribs.insert(std::make_pair(L"usb_MDL", m_PrinterInfo->GetModel())); //deprecated
	txtAttribs.insert(std::make_pair(L"Color", m_PrinterInfo->ColorSupported() ? L"T" : L"F"));
	txtAttribs.insert(std::make_pair(L"Duplex", m_PrinterInfo->DuplexSupported() ? L"T" : L"F"));
	if (m_PrinterInfo->GetPrinterLocation().size())
	{
		txtAttribs.insert(std::make_pair(L"note", m_PrinterInfo->GetPrinterLocation().c_str()));
	}

	bool res = false;
	for (size_t i = 0; i < _countof(serviceTypes); i++)
	{
		CDnssdService* pService = CDnssdService::Create();
		if (pService)
		{
			if (pService->Init(pInstanceName, ports[serviceTypes[i]].ServiceAndDomain, L"", ports[serviceTypes[i]].Port, &txtAttribs))
			{
				m_services.push_back(pService);
				
				// At least one succeeded.
				res = true;
			}
			else
			{
				pService->Release();
			}
		}
	}

	return res;
}

void CDnssdPrinterService::Uninit()
{
	for (auto it = m_services.crbegin(); it != m_services.crend(); it++)
	{
		CDnssdService* pService = *it;

		pService->Stop();
		pService->Uninit();
		pService->Release();
	}
	m_services.clear();

	SAFE_RELEASE(m_PrinterInfo);
}

void CDnssdPrinterService::Start()
{
	for (auto it = m_services.cbegin(); it != m_services.cend(); it++)
	{
		CDnssdService* pService = *it;
		pService->Start();
	}
}

void CDnssdPrinterService::Stop()
{
	for (auto it = m_services.crbegin(); it != m_services.crend(); it++)
	{
		CDnssdService* pService = *it;
		pService->Stop();
	}
}
