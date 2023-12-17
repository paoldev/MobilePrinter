#include "pch.h"
#include "DnssdService.h"

#include <mutex>
#include <atomic>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma comment(lib, "windowsapp.lib")

class scoped_winrt
{
public:
	scoped_winrt()
	{
		winrt::init_apartment();
	}
	~scoped_winrt()
	{
		winrt::uninit_apartment();
	}
};

CDnssdService* CDnssdService::Create()
{
	return new CDnssdService();
}

CDnssdService::CDnssdService() : m_udpSocket(nullptr), m_tcpSocket(nullptr), m_service(nullptr), m_tcptoken{}, m_udptoken{}
{
	static scoped_winrt s_winrt;
}

CDnssdService::~CDnssdService()
{
	Uninit();
}

bool CDnssdService::Init(const wchar_t* pInstanceName, const wchar_t* pServiceType, const wchar_t* pHostName, const wchar_t* pPort, const std::map<std::wstring, std::wstring>* pTxtAttribs)
{
	m_instanceName = pInstanceName;
	m_serviceType = pServiceType;
	m_hostName = pHostName;
	m_port = pPort;
	m_serviceName.clear();
	m_txtAttribs.clear();
	if (pTxtAttribs)
	{
		m_txtAttribs = *pTxtAttribs;
	}
	return true;
}

void CDnssdService::Uninit()
{
	Stop();
}

void CDnssdService::Start()
{
	if (!m_service)
	{
		winrt::Windows::Networking::HostName hostName(nullptr);
		if (m_hostName.size())
		{
			hostName = std::move(winrt::Windows::Networking::HostName(m_hostName));
		}
		else
		{
			auto hosts = winrt::Windows::Networking::Connectivity::NetworkInformation::GetHostNames();
			for (unsigned int i = 0; i < hosts.Size(); ++i)
			{
				auto name = hosts.GetAt(i);
				//LOG("Host: %ls - %d", name.RawName().data(), (int)name.Type());
				if (name.Type() == winrt::Windows::Networking::HostNameType::DomainName)
				{
					winrt::hstring rawName = name.RawName();
					std::wstring_view view = rawName;
					if (view.find(L"local") != std::string::npos)
					{
						hostName = name;
						break;
					}
				}
			}
		}

		auto connectionProfile = winrt::Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile();

		if ((hostName != nullptr) && (connectionProfile != nullptr))
		{
			bool bUseTcp = (m_serviceType.find(L".tcp.") != std::wstring::npos) || (m_serviceType.find(L".tcp,") != std::wstring::npos);
			if (bUseTcp)
			{
				m_tcpSocket = {};
				m_tcptoken = m_tcpSocket.ConnectionReceived([](winrt::Windows::Networking::Sockets::StreamSocketListener sender, winrt::Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs args) {
					auto socket = args.Socket();
					auto info = socket.Information();
					DBGLOG("ConnectionReceived: %ls:%ls %ls:%ls %ls", info.LocalAddress().DisplayName().c_str(), info.LocalPort().c_str(), info.RemoteAddress().DisplayName().c_str(), info.RemotePort().c_str(), info.RemoteServiceName().c_str());

					//Here, a Soap request of type Probe is received, sent to "POST /StableWSDiscoveryEndpoint/schemas-xmlsoap-org_ws_2005_04_discovery HTTP/1.1"
					//TODO: parse the  request and reply with a ProbeMatch; this is not required if WSDPrinterService is enabled and running.
					//Or:
					//For 'IPP discovery' a socket is required on port 631 (m_port.c_str()) or 443 (https): "POST /printers/this HTTP/1.1"
#if 0
					auto reader = winrt::Windows::Storage::Streams::DataReader(socket.InputStream());
					try
					{
						reader.InputStreamOptions(winrt::Windows::Storage::Streams::InputStreamOptions::Partial);

						uint8_t bytes[1024] = {};
						uint32_t count = reader.UnconsumedBufferLength();
						if (count == 0)
						{
							count = reader.LoadAsync(1024).get();
						}

						winrt::array_view<uint8_t> value(bytes, bytes + std::min(count, 1024u));
						reader.ReadBytes(value);

						for (uint32_t i = 0; i < count; i += 8)
						{
							switch (count - i)
							{
							case 0: DBGLOG2("\n"); break;
							case 1: DBGLOG2("%02x %c\n", bytes[i], bytes[i]); break;
							case 2: DBGLOG2("%02x %02x %c%c\n", bytes[i], bytes[i + 1], bytes[i], bytes[i + 1]); break;
							case 3: DBGLOG2("%02x %02x %02x %c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i], bytes[i + 1], bytes[i + 2]); break;
							case 4: DBGLOG2("%02x %02x %02x %02x %c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3]); break;
							case 5: DBGLOG2("%02x %02x %02x %02x %02x %c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4]); break;
							case 6: DBGLOG2("%02x %02x %02x %02x %02x %02x %c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5]); break;
							case 7: DBGLOG2("%02x %02x %02x %02x %02x %02x %02x %c%c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6]); break;
							default: DBGLOG2("%02x %02x %02x %02x %02x %02x %02x %02x %c%c%c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7]); break;
							}

						}
					}
					catch (...)
					{

					}
#endif	//0
				});

				//For 'WSD discovery', a socket is required on port 80: "POST /StableWSDiscoveryEndpoint/schemas-xmlsoap-org_ws_2005_04_discovery HTTP/1.1"
				//For 'IPP discovery', a socket is required on port 631 (m_port.c_str()) or 443 (https): "POST /printers/this HTTP/1.1"
				auto asyncOp = m_tcpSocket.BindServiceNameAsync(m_port.c_str(), winrt::Windows::Networking::Sockets::SocketProtectionLevel::PlainSocket, connectionProfile.NetworkAdapter());
				asyncOp.get();
			}
			else
			{
				m_udpSocket = {};
				m_udpSocket.Control().MulticastOnly(true);
				m_udptoken = m_udpSocket.MessageReceived([](winrt::Windows::Networking::Sockets::DatagramSocket sender, winrt::Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs args) {
					DBGLOG("MessageReceived: %ls %ls %ls", args.LocalAddress().ToString().c_str(), args.RemoteAddress().ToString().c_str(), args.RemotePort().c_str());
	
#if 0
					auto reader = args.GetDataReader();
					try
					{
						uint8_t bytes[1024] = {};
						uint32_t count = reader.UnconsumedBufferLength();
						if (count == 0)
						{
							count = reader.LoadAsync(1024).get();
						}

						winrt::array_view<uint8_t> value(bytes, bytes + std::min(count, 1024u));
						reader.ReadBytes(value);

						for (uint32_t i = 0; i < count; i += 8)
						{
							switch (count - i)
							{
							case 0: DBGLOG2("\n"); break;
							case 1: DBGLOG2("%02x %c\n", bytes[i], bytes[i]); break;
							case 2: DBGLOG2("%02x %02x %c%c\n", bytes[i], bytes[i + 1], bytes[i], bytes[i + 1]); break;
							case 3: DBGLOG2("%02x %02x %02x %c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i], bytes[i + 1], bytes[i + 2]); break;
							case 4: DBGLOG2("%02x %02x %02x %02x %c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3]); break;
							case 5: DBGLOG2("%02x %02x %02x %02x %02x %c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4]); break;
							case 6: DBGLOG2("%02x %02x %02x %02x %02x %02x %c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5]); break;
							case 7: DBGLOG2("%02x %02x %02x %02x %02x %02x %02x %c%c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6]); break;
							default: DBGLOG2("%02x %02x %02x %02x %02x %02x %02x %02x %c%c%c%c%c%c%c%c\n", bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7], bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3], bytes[i + 4], bytes[i + 5], bytes[i + 6], bytes[i + 7]); break;
							}

						}
					}
					catch (...)
					{

					}
#endif	//0
				});

				//auto mdnsHost = winrt::Windows::Networking::HostName(L"224.0.0.251");
				//auto mdnsPort = L"5353";
				auto asyncOp = m_udpSocket.BindServiceNameAsync(/*mdnsPort*/m_port.c_str(), connectionProfile.NetworkAdapter());
				//auto asyncOp = m_udpSocket.BindEndpointAsync(mdnsHost, m_port.c_str());
				asyncOp.get();

				//m_udpSocket.JoinMulticastGroup(mdnsHost);
			}

			uint16_t port = 0;
			if (bUseTcp)
			{
				port = static_cast<uint16_t>(_wtoi(m_tcpSocket.Information().LocalPort().c_str()));
			}
			else
			{
				port = static_cast<uint16_t>(_wtoi(m_udpSocket.Information().LocalPort().c_str()));
			}

			std::wstring serviceName = m_instanceName;
			//replaceAll(serviceName, L"\\", L"\\\\");
			serviceName += m_serviceType;

			DBGLOG("%ls %ls %d", serviceName.c_str(), hostName.RawName().c_str(), (int)port);

			winrt::Windows::Networking::ServiceDiscovery::Dnssd::DnssdServiceInstance service(serviceName.c_str(), hostName, port);

			m_service = std::move(service);
			m_service.Priority(50);
			m_service.Weight(0);

			if (m_txtAttribs.size())
			{
				auto txtAttribs = m_service.TextAttributes();

				auto itRp = m_txtAttribs.find(L"rp");
				if (itRp != m_txtAttribs.end())
				{
					txtAttribs.Insert(L"rp", itRp->second);
				}

				for (auto it = m_txtAttribs.begin(); it != m_txtAttribs.end(); it++)
				{
					if (!txtAttribs.HasKey(it->first))
					{
						txtAttribs.Insert(it->first, it->second);
					}
				}
			}

			winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Networking::ServiceDiscovery::Dnssd::DnssdRegistrationResult> asyncRes;
			if (bUseTcp)
			{
				asyncRes = m_service.RegisterStreamSocketListenerAsync(m_tcpSocket);
			}
			else
			{
				asyncRes = m_service.RegisterDatagramSocketAsync(m_udpSocket);
			}
			try
			{
				auto res = asyncRes.get();
				m_serviceName = m_service.DnssdServiceInstanceName();
				DBGLOG("%ls %ls %d", serviceName.c_str(), hostName.RawName().c_str(), (int)port);
				LOG2("[DNS] Starting %ls %d ... done.\n", m_serviceName.c_str(), (int)port);
			}
			catch (...)
			{
				ELOG("Error");
			}

			//return res.Status == winrt::Windows::Networking::ServiceDiscovery::Dnssd::DnssdRegistrationStatus::Success;
		}
	}
}

void CDnssdService::Stop()
{
	LOG2("[DNS] Stopping %ls ... ", m_serviceName.c_str());

	if (m_tcpSocket)
	{
		m_tcpSocket.ConnectionReceived(m_tcptoken);
		m_tcpSocket.Close();

		m_tcpSocket = nullptr;
	}

	if (m_udpSocket)
	{
		m_udpSocket.MessageReceived(m_udptoken);
		m_udpSocket.Close();

		m_udpSocket = nullptr;
	}

	m_service = nullptr;
	m_tcptoken = {};
	m_udptoken = {};

	LOG2("done.\n");
}
