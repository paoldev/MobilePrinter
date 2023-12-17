#pragma once

class CPrinterInfo;

class __declspec(uuid("11E00A69-82EA-45ED-B321-B96ECB6851FD")) IPrinterService : public IUnknown
{
public:

	virtual bool Init(CPrinterInfo* pPrinter) = 0;
	virtual void Uninit() = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;
};
