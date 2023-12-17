#pragma once

#include "CommonUtils.h"
#include <winrt/Windows.Networking.h>
#include <winrt/windows.networking.sockets.h>
#include <winrt/Windows.Networking.ServiceDiscovery.Dnssd.h>

class CDnssdService : public CUnknown<CDnssdService>
{
public:

	static CDnssdService* Create();

	bool Init(const wchar_t* pInstanceName, const wchar_t* pServiceType, const wchar_t* pHostName, const wchar_t* pPort, const std::map<std::wstring, std::wstring>* pTxtAttribs = nullptr);
	void Uninit();

	void Start();
	void Stop();

private:
	CDnssdService();
	virtual ~CDnssdService();

private:
	std::wstring m_instanceName;
	std::wstring m_serviceType;
	std::wstring m_hostName;
	std::wstring m_port;
	std::wstring m_serviceName;
	std::map<std::wstring, std::wstring> m_txtAttribs;
	winrt::Windows::Networking::Sockets::DatagramSocket m_udpSocket;
	winrt::Windows::Networking::Sockets::StreamSocketListener m_tcpSocket;
	winrt::Windows::Networking::ServiceDiscovery::Dnssd::DnssdServiceInstance m_service;
	winrt::event_token m_tcptoken;
	winrt::event_token m_udptoken;
};
