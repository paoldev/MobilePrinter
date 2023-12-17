#pragma once

#include "CommonUtils.h"
#include "PrinterService.h"

class CSSLUtils;
class ipp_server;
class CPrinterInfo;

class CIppPrinterService : CUnknown<CIppPrinterService, IPrinterService>
{
public:
	static IPrinterService* Create();

	static void Config(const bool i_bManageFirewall, const bool i_bDebugSaveIppDataToFile);
	static void ConfigSSL(const bool i_bEnableSSL, const std::wstring& i_wsCertHash, const std::wstring& i_wsCertSubjectName, const std::wstring& i_wsStoreName);

	bool Init(CPrinterInfo* pPrinter);
	void Uninit();

	void Start();
	void Stop();

private:
	CIppPrinterService();
	virtual ~CIppPrinterService();

private:

	CPrinterInfo* m_PrinterInfo;
	std::unique_ptr<ipp_server> m_ippserver;
	std::unique_ptr<CSSLUtils> m_sslUtils;
};
