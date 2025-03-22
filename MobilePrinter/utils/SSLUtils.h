#pragma once

/// <summary>
/// SSL helper
/// </summary>
/// <usage>
/// Before using this class, a valid certificate has to be registered in the Store:
/// from an administrator command prompt, for a self-signed certificate, run the command
///		makecert.exe -sr LocalMachine -ss <StoreName> -a sha512 -n "CN=<my_specific_subject>" -sky exchange -pe -r
/// where StoreName may be Root, My or another value.
///
/// CSSLUtils::Init() is equivalent to following commands
///		eSsl: netsh http add sslcert ipport=0.0.0.0:<Port> certhash=<CertificateThumbprint> appid={guidAppId} certstorename=<StoreName>
///		eSslCcs: netsh http add sslcert ccs=<Port> appid={guidAppId}
///		eSslSni: netsh http add sslcert hostnameport=<HostName:Port> certhash=<CertificateThumbprint> appid={guidAppId} certstorename=<StoreName>
///
/// CSSLUtils::Uninit() is equivalent to following commands
///		netsh http delete sslcert ipport=0.0.0.0:<Port>
///		netsh http delete sslcert ccs=<Port>
///		netsh http delete sslcert hostnameport=<HostName:Port>
/// </usage>
class CSSLUtils
{
public:

	/// <summary>
	/// Select a specific SSL configuration mode.
	/// </summary>
	/// <param name="eSsl">Sets a specified SSL certificate record.</param>
	/// <param name="eSslCcs">Sets the SSL certificate record that specifies that Http.sys should consult the Centralized Certificate Store (CCS) store to find certificates if the port receives a Transport Layer Security (TLS) handshake.</param>
	/// <param name="eSslSni">Sets a specified SSL Server Name Indication (SNI) certificate record.</param>
	enum class Type
	{
		eSsl,
		eSslCcs,
		eSslSni
	};

	/// <summary>
	/// SSL configuration.
	/// </summary>
	/// <param name="m_type">Type of the desired SSL configuration.</param>
	/// <param name="m_port">Port for the SSL binding (required by SSL, CCS, SNI).</param>
	/// <param name="m_wsHostName">Hostname for the SSL binding (required by SNI).</param>
	/// <param name="m_wsCertHash">Hexadecimal hash string of the certificate to use (required by SSL, SNI).</param>
	/// <param name="m_wsCertSubjectName">Subject name of the certificate to find, if CertHash string is not specified (required by SSL, SNI).</param>
	/// <param name="m_wsStoreName">Name of the store under Local Machine where certificate is present (required by SSL, SNI).</param>
	/// <param name="m_guidAppId">A unique identifier that can be used to identify the app that has set this parameter (required by SSL, CCS, SNI).</param>
	struct Config
	{
		CSSLUtils::Type m_type = CSSLUtils::Type::eSsl;
		uint16_t		m_port = 0;
		std::wstring	m_wsHostName;
		std::wstring	m_wsCertHash;
		std::wstring	m_wsCertSubjectName;
		std::wstring	m_wsStoreName;
		GUID			m_guidAppId = {};
	};

	CSSLUtils();
	CSSLUtils(const CSSLUtils::Config& i_config);
	~CSSLUtils();

	/// <summary>
	/// SSL initialization.
	/// </summary>
	/// <param name="i_config">SSL configuration</param>
	/// <returns>true on success</returns>
	bool Init(const CSSLUtils::Config& i_config);

	/// <summary>
	/// SSL Uninitialization.
	/// </summary>
	void Uninit();

	/// <summary>
	/// Get if SSL initialization succeeded.
	/// </summary>
	/// <returns>SSL initialization succeess</returns>
	bool IsInitialized() const { return m_bInitialized; }

	/// <summary>
	/// Retrieve the hash of the specified certificate.
	/// </summary>
	/// <param name="i_wsCertSubjectName">Subject name of the certificate to find.</param>
	/// <param name="i_wsStoreName">Name of the store under Local Machine where certificate is present</param>
	/// <param name="o_hash">The hash, if succeeded</param>
	/// <returns>true if succeeded, false otherwise</returns>
	static bool GetCertificateHash(const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName, std::vector<uint8_t>& o_hash);

	/// <summary>
	/// Retrieve the hash of the specified certificate, as an hexadecimal string.
	/// </summary>
	/// <param name="i_wsCertSubjectName">Subject name of the certificate to find.</param>
	/// <param name="i_wsStoreName">Name of the store under Local Machine where certificate is present</param>
	/// <param name="o_hash">The hexadecimal string of the hash, if succeeded</param>
	/// <returns>true if succeeded, false otherwise</returns>
	static bool GetCertificateHash(const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName, std::wstring& o_hash);

private:
	bool	m_bInitialized;
	Config	m_config;
};
