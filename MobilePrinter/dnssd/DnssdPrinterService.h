#pragma once

#include "CommonUtils.h"
#include "PrinterInfo.h"
#include "PrinterService.h"
#include <vector>

class CDnssdService;

class CDnssdPrinterService : public CUnknown<CDnssdPrinterService, IPrinterService>
{
public:

	static IPrinterService* Create();

	static void ConfigSSL(const bool i_bEnableSSL);

	bool Init(CPrinterInfo* pPrinter);
	void Uninit();

	void Start();
	void Stop();

private:
	CDnssdPrinterService();
	virtual ~CDnssdPrinterService();

private:

	std::wstring m_printerName;
	std::vector<CDnssdService*> m_services;

	CPrinterInfo* m_PrinterInfo;
};
