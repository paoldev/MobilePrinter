//// 
//// This code is derived from the sample at
//// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/HttpSSLConfig/cpp/SslConfig.cpp
//// 

#include "pch.h"
#include "SSLUtils.h"
#include "StringUtils.h"
#include "LogUtils.h"

#if IPP_WINHTTP_SSL_SUPPORTED

#include <http.h>
#include <mstcpip.h>

namespace MP::Ssl
{
	/// <summary>
	/// Get certificate hash from the certificate subject name.
	/// </summary>
	/// <param name="CertSubjectName">Subject name of the certificate to find.</param>
	/// <param name="StoreName">Name of the store under Local Machine where certificate is present.</param>
	/// <param name="CertHash">Buffer to return certificate hash.</param>
	/// <param name="CertHashLength">Buffer length on input, hash length on output (element count).</param>
	/// <returns>Status</returns>
	DWORD GetCertificateHash(
		_In_ PCWSTR CertSubjectName,
		_In_ PCWSTR StoreName,
		_Out_writes_bytes_to_(*CertHashLength, *CertHashLength) PBYTE CertHash,
		_Inout_ PDWORD CertHashLength)
	{
		HCERTSTORE SystemStore = nullptr;
		PCCERT_CONTEXT CertContext = nullptr;
		DWORD Error = ERROR_SUCCESS;

		RtlZeroMemory(CertHash, *CertHashLength);

		//
		// Open the store under local machine.
		//
		SystemStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
			0,
			(HCRYPTPROV_LEGACY)nullptr,
			CERT_SYSTEM_STORE_LOCAL_MACHINE,
			StoreName);
		if (SystemStore == nullptr)
		{
			Error = GetLastError();
		}
		else
		{
			//
			// Find the certificate from the subject name and get the hash.
			//
			CertContext = CertFindCertificateInStore(SystemStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, CertSubjectName, nullptr);
			if (CertContext == nullptr)
			{
				Error = GetLastError();
			}
			else if (!CertGetCertificateContextProperty(CertContext, CERT_HASH_PROP_ID, CertHash, CertHashLength))
			{
				Error = GetLastError();
			}
		}

		//
		// Free the certificate context.
		//
		if (CertContext != nullptr)
		{
			CertFreeCertificateContext(CertContext);
		}

		//
		// Close the certificate store if it was opened.
		//
		if (SystemStore != nullptr)
		{
			CertCloseStore(SystemStore, 0);
		}

		return Error;
	}

	/// <summary>
	/// Create the SSL endpoint key.
	/// </summary>
	/// <param name="Port">Port for the SSL binding.</param>
	/// <param name="pSockAddress">Socket for the SSL binding, initialized by this function.</param>
	/// <returns>The SSL key</returns>
	HTTP_SERVICE_CONFIG_SSL_KEY CreateSslKey(USHORT Port, _Out_ PSOCKADDR_IN pSockAddress)
	{
		HTTP_SERVICE_CONFIG_SSL_KEY SslKey = {};

		//
		// Create SslKey.
		// N.B. A CCS binding is IP version agnostic but for API purposes we are
		// required to specify the IP address to be (IPv4) 0.0.0.0 in the key.
		//
		SslKey.pIpPort = (PSOCKADDR)pSockAddress;
		IN4ADDR_SETANY((PSOCKADDR_IN)SslKey.pIpPort);
		SS_PORT(SslKey.pIpPort) = htons(Port);

		return SslKey;
	}

	/// <summary>
	/// Set the SSL configuration.
	/// </summary>
	/// <param name="SslKey">SSL endpoint key.</param>
	/// <param name="CertHash">Hash of the certificate to find.</param>
	/// <param name="CertHashLength">Length in bytes of the Hash of the certificate to find.</param>
	/// <param name="StoreName">Name of the store under Local Machine where certificate is present.</param>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter.</param>
	/// <returns>Status.</returns>
	DWORD SetSslConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_KEY SslKey,
		_In_ const BYTE* CertHash,
		_In_ const DWORD CertHashLength,
		_In_ PCWSTR StoreName,
		_In_ const GUID& AppId)
	{
		HTTP_SERVICE_CONFIG_SSL_SET SslConfig = {};

		SslConfig.KeyDesc = *SslKey;
		SslConfig.ParamDesc.AppId = AppId;
		SslConfig.ParamDesc.pSslHash = (PVOID)CertHash;
		SslConfig.ParamDesc.SslHashLength = CertHashLength;
		SslConfig.ParamDesc.pSslCertStoreName = (PWSTR)StoreName;

		DWORD Error = HttpSetServiceConfiguration(nullptr, HttpServiceConfigSSLCertInfo, &SslConfig, sizeof(SslConfig), nullptr);
		return Error;
	}

	/// <summary>
	/// Delete the SSL configuration.
	/// </summary>
	/// <param name="SslKey">SSL endpoint key.</param>
	/// <returns>Status.</returns>
	DWORD DeleteSslConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_KEY SslKey)
	{
		HTTP_SERVICE_CONFIG_SSL_SET SslConfig = {};

		SslConfig.KeyDesc = *SslKey;
		DWORD Error = HttpSetServiceConfiguration(nullptr, HttpServiceConfigSSLCertInfo, &SslConfig, sizeof(SslConfig), nullptr);
		return Error;
	}

	/// <summary>
	/// Initialize and set the SSL configuration.
	/// </summary>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter.</param>
	/// <param name="Port">Port for the SSL binding.</param>
	/// <param name="CertHash">Hash of the certificate to find.</param>
	/// <param name="CertHashLength">Length in bytes of the Hash of the certificate to find.</param>
	/// <param name="StoreName">Name of the store under Local Machine where certificate is present.</param>
	/// <returns>Status.</returns>
	DWORD InitSsl(const GUID& AppId, USHORT Port, _In_ const BYTE* CertHash,
		_In_ const DWORD CertHashLength, _In_ PCWSTR StoreName)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			SOCKADDR_IN SockAddress = {};
			HTTP_SERVICE_CONFIG_SSL_KEY Key = CreateSslKey(Port, &SockAddress);
			res = SetSslConfiguration(&Key, CertHash, CertHashLength, StoreName, AppId);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return res;
	}

	/// <summary>
	/// Uninitialize the SSL configuration.
	/// </summary>
	/// <param name="Port">Port for the SSL binding.</param>
	/// <returns>Status.</returns>
	DWORD UninitSsl(USHORT Port)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			SOCKADDR_IN SockAddress = {};
			HTTP_SERVICE_CONFIG_SSL_KEY Key = CreateSslKey(Port, &SockAddress);
			res = DeleteSslConfiguration(&Key);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return res;
	}



	/// <summary>
	/// Create the CCS SSL endpoint key.
	/// </summary>
	/// <param name="Port"></param>
	/// <returns></returns>
	HTTP_SERVICE_CONFIG_SSL_CCS_KEY CreateCcsKey(USHORT Port)
	{
		HTTP_SERVICE_CONFIG_SSL_CCS_KEY CcsKey = {};

		//
		// Create CcsKey.
		// N.B. A CCS binding is IP version agnostic but for API purposes we are
		// required to specify the IP address to be (IPv4) 0.0.0.0 in the key.
		//
		IN4ADDR_SETANY((PSOCKADDR_IN)&CcsKey.LocalAddress);
		SS_PORT(&CcsKey.LocalAddress) = htons(Port);

		return CcsKey;
	}

	/// <summary>
	/// Set the CCS configuration.
	/// </summary>
	/// <param name="CcsKey">CCS SSL endpoint key.</param>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter.</param>
	/// <returns>Status.</returns>
	DWORD SetCcsConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_CCS_KEY CcsKey, _In_ const GUID& AppId)
	{
		DWORD Error = ERROR_SUCCESS;
		HTTP_SERVICE_CONFIG_SSL_CCS_SET CcsConfig = {};

		CcsConfig.KeyDesc = *CcsKey;
		CcsConfig.ParamDesc.AppId = AppId;

		Error = HttpSetServiceConfiguration(nullptr, HttpServiceConfigSslCcsCertInfo, &CcsConfig, sizeof(CcsConfig), nullptr);

		return Error;
	}

	/// <summary>
	/// Delete the CCS SSL configuration.
	/// </summary>
	/// <param name="CcsKey">CCS SSL endpoint key.</param>
	/// <returns>Status.</returns>
	DWORD DeleteCcsConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_CCS_KEY CcsKey)
	{
		DWORD Error = ERROR_SUCCESS;
		HTTP_SERVICE_CONFIG_SSL_CCS_SET CcsConfig = {};

		CcsConfig.KeyDesc = *CcsKey;

		Error = HttpDeleteServiceConfiguration(nullptr, HttpServiceConfigSslCcsCertInfo, &CcsConfig, sizeof(CcsConfig), nullptr);

		return Error;
	}

	/// <summary>
	/// Initialize and set the CCS SSL configuration.
	/// </summary>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter.</param>
	/// <param name="Port">Port for the CCS SSL binding.</param>
	/// <returns>Status.</returns>
	bool InitCcs(const GUID& AppId, USHORT Port)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			HTTP_SERVICE_CONFIG_SSL_CCS_KEY Key = CreateCcsKey(Port);
			res = SetCcsConfiguration(&Key, AppId);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return (res == ERROR_SUCCESS);
	}

	/// <summary>
	/// Uninitialize the CCS SSL configuration.
	/// </summary>
	/// <param name="Port">Port for the CCS SSL binding.</param>
	/// <returns>Status.</returns>
	bool UninitCcs(USHORT Port)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			HTTP_SERVICE_CONFIG_SSL_CCS_KEY Key = CreateCcsKey(Port);
			res = DeleteCcsConfiguration(&Key);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return (res == ERROR_SUCCESS);
	}

	/// <summary>
	/// Create the SNI SSL endpoint key: host and port.
	/// </summary>
	/// <param name="Port">Port for the hostname based SSL binding.</param>
	/// <param name="Hostname">Hostname for the SSL binding.</param>
	/// <returns>The SNI key.</returns>
	HTTP_SERVICE_CONFIG_SSL_SNI_KEY CreateSniKey(USHORT Port, _In_ PCWSTR Hostname)
	{
		HTTP_SERVICE_CONFIG_SSL_SNI_KEY SniKey = {};

		//
		// Create SniKey.
		// N.B. An SNI binding is IP version agnostic but for API purposes we are
		// required to specify the IP address to be (IPv4) 0.0.0.0 in the key.
		//
		SniKey.Host = (PWSTR)Hostname;
		IN4ADDR_SETANY((PSOCKADDR_IN)&SniKey.IpPort);
		SS_PORT(&SniKey.IpPort) = htons(Port);
		return SniKey;
	}

	/// <summary>
	/// Set the SNI configuration.
	/// </summary>
	/// <param name="SniKey">SSL endpoint key: host and port.</param>
	/// <param name="CertHash">Hash of the certificate to find.</param>
	/// <param name="CertHashLength">Length in bytes of the Hash of the certificate to find.</param>
	/// <param name="StoreName">Name of the store under Local Machine where certificate is present.</param>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter</param>
	/// <returns>Status.</returns>
	DWORD SetSniConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_SNI_KEY SniKey, _In_ const BYTE* CertHash,
		_In_ const DWORD CertHashLength, _In_ PCWSTR StoreName, _In_ const GUID& AppId)
	{
		HTTP_SERVICE_CONFIG_SSL_SNI_SET SniConfig = {};

		SniConfig.KeyDesc = *SniKey;
		SniConfig.ParamDesc.pSslHash = (PVOID)CertHash;
		SniConfig.ParamDesc.SslHashLength = CertHashLength;
		SniConfig.ParamDesc.pSslCertStoreName = (PWSTR)StoreName;
		SniConfig.ParamDesc.AppId = AppId;
		
		DWORD Error = HttpSetServiceConfiguration(nullptr, HttpServiceConfigSslSniCertInfo, &SniConfig, sizeof(SniConfig), nullptr);
		return Error;
	}

	/// <summary>
	/// Delete the SNI configuration.
	/// </summary>
	/// <param name="SniKey">SSL endpoint key: host and port.</param>
	/// <returns>Status.</returns>
	DWORD DeleteSniConfiguration(_In_ PHTTP_SERVICE_CONFIG_SSL_SNI_KEY SniKey)
	{
		DWORD Error = ERROR_SUCCESS;
		HTTP_SERVICE_CONFIG_SSL_SNI_SET SniConfig = {};

		SniConfig.KeyDesc = *SniKey;

		Error = HttpDeleteServiceConfiguration(nullptr, HttpServiceConfigSslSniCertInfo, &SniConfig, sizeof(SniConfig), nullptr);
		return Error;
	}

	/// <summary>
	/// Initialize the hostname based SSL configuration.
	/// </summary>
	/// <param name="AppId">A unique identifier that can be used to identify the app that has set this parameter.</param>
	/// <param name="Port">Port for the hostname based SSL binding.</param>
	/// <param name="Hostname">Hostname for the SSL binding.</param>
	/// <param name="CertHash">Hash of the certificate to find.</param>
	/// <param name="CertHashLength">Length in bytes of the Hash of the certificate to find.</param>
	/// <param name="StoreName">Name of the store under Local Machine where certificate is present.</param>
	/// <returns>Status.</returns>
	DWORD InitSni(const GUID& AppId, USHORT Port, _In_ PCWSTR Hostname, _In_ const BYTE* CertHash,
		_In_ const DWORD CertHashLength, _In_ PCWSTR StoreName)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			HTTP_SERVICE_CONFIG_SSL_SNI_KEY SniKey = CreateSniKey(Port, Hostname);

			//
			// Create the SNI binding.
			//
			res = SetSniConfiguration(&SniKey, CertHash, CertHashLength, StoreName, AppId);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return res;
	}

	/// <summary>
	/// Uninitialize the hostname based SSL configuration.
	/// </summary>
	/// <param name="Port">Port for the hostname based SSL binding.</param>
	/// <param name="Hostname">Hostname for the SSL binding.</param>
	/// <returns>Status.</returns>
	DWORD UninitSni(USHORT Port, PCWSTR Hostname)
	{
		HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;
		DWORD res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, nullptr);
		if (res == ERROR_SUCCESS)
		{
			HTTP_SERVICE_CONFIG_SSL_SNI_KEY Key = CreateSniKey(Port, Hostname);

			res = DeleteSniConfiguration(&Key);

			HttpTerminate(HTTP_INITIALIZE_CONFIG, nullptr);
		}
		return res;
	}
}	//namespace MP::Ssl

#endif	//IPP_WINHTTP_SSL_SUPPORTED

CSSLUtils::CSSLUtils() : m_bInitialized(false), m_config{}
{
}

CSSLUtils::CSSLUtils(const CSSLUtils::Config& i_config) : m_bInitialized(false), m_config{}
{
	Init(i_config);
}

CSSLUtils::~CSSLUtils()
{
	Uninit();
}

bool CSSLUtils::Init(const CSSLUtils::Config& i_config)
{
	if (m_bInitialized)
	{
		Uninit();
	}

	DWORD Error = ERROR_INVALID_PARAMETER;

#if	IPP_WINHTTP_SSL_SUPPORTED

	std::vector<uint8_t> CertificateHash;
	if (i_config.m_wsCertHash.size())
	{
		if (!decode_hex_string(i_config.m_wsCertHash, CertificateHash))
		{
			LOG2("Error initializing SSL %d: invalid certificate hash %ls\n", (int)i_config.m_type, i_config.m_wsCertHash.c_str());
			return false;
		}
	}
	else if (i_config.m_wsCertSubjectName.size())
	{
		if (!GetCertificateHash(i_config.m_wsCertSubjectName, i_config.m_wsStoreName, CertificateHash))
		{
			LOG2("Error initializing SSL %d: invalid certificate subject \"%ls\" or store \"%ls\"\n", (int)i_config.m_type, 
				i_config.m_wsCertSubjectName.c_str(), i_config.m_wsStoreName.c_str());
			return false;
		}
	}

	switch (i_config.m_type)
	{
	case Type::eSsl:
		Error = MP::Ssl::InitSsl(i_config.m_guidAppId, i_config.m_port, CertificateHash.data(),
			static_cast<DWORD>(CertificateHash.size()), i_config.m_wsStoreName.c_str());
		break;

	case Type::eSslCcs:
		Error = MP::Ssl::InitCcs(i_config.m_guidAppId, i_config.m_port);
		break;

	case Type::eSslSni:
		Error = MP::Ssl::InitSni(i_config.m_guidAppId, i_config.m_port,
			i_config.m_wsHostName.c_str(),
			CertificateHash.data(),
			static_cast<DWORD>(CertificateHash.size()),
			i_config.m_wsStoreName.c_str());
		break;
	}
#endif	//IPP_WINHTTP_SSL_SUPPORTED

	if (Error == ERROR_SUCCESS)
	{
		m_config = i_config;
		m_bInitialized = true;
	}
	else
	{
		LOG2("Error initializing SSL %d: %08X\n", (int)i_config.m_type, Error);
	}

	return (Error == ERROR_SUCCESS);
}

void CSSLUtils::Uninit()
{
	if (m_bInitialized)
	{
#if IPP_WINHTTP_SSL_SUPPORTED
		switch (m_config.m_type)
		{
		case Type::eSsl:
			MP::Ssl::UninitSsl(m_config.m_port);
			break;

		case Type::eSslCcs:
			MP::Ssl::UninitCcs(m_config.m_port);
			break;

		case Type::eSslSni:
			MP::Ssl::UninitSni(m_config.m_port, m_config.m_wsHostName.c_str());
			break;
		}
#endif	//IPP_WINHTTP_SSL_SUPPORTED

		m_config = CSSLUtils::Config();
		m_bInitialized = false;
	}
}

bool CSSLUtils::GetCertificateHash(const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName, std::vector<uint8_t>& o_hash)
{
	o_hash.clear();

	DWORD Error = ERROR_INVALID_PARAMETER;

#if IPP_WINHTTP_SSL_SUPPORTED
	BYTE CertHash[50] = {};
	DWORD CertHashLength = ARRAYSIZE(CertHash);
	Error = MP::Ssl::GetCertificateHash(i_wsCertSubjectName.c_str(), i_wsStoreName.c_str(), CertHash, &CertHashLength);
	if (Error == ERROR_SUCCESS)
	{
		o_hash.assign(CertHash, CertHash + CertHashLength);
	}
#endif

	return (Error == ERROR_SUCCESS);
}

bool CSSLUtils::GetCertificateHash(const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName, std::wstring& o_hash)
{
	o_hash.clear();

	std::vector<uint8_t> hash;
	if (!GetCertificateHash(i_wsCertSubjectName, i_wsStoreName, hash))
	{
		return false;
	}

	encode_hex_string(hash, o_hash);
	return true;
}
