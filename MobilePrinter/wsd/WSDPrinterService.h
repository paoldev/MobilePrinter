#pragma once

#include "CommonUtils.h"
#include "PrinterInfo.h"
#include "PrinterService.h"
#include "src_host/MyPrinter_h.h"

class PrinterJob;
class CPrinterServiceTypeEventSource;
class CPrinterServiceV20TypeEventSource;

class CWSDPrinterService : public CUnknown<CWSDPrinterService, IPrinterService, IPrinterServiceType, IPrinterServiceV12Type, IPrinterServiceV20Type>
{
public:

	static IPrinterService* Create();
	
	virtual ~CWSDPrinterService();

	bool Init(CPrinterInfo* pPrinter);
	void Uninit();

	void Start();
	void Stop();

private:

	virtual HRESULT __stdcall CreatePrintJob(CREATE_PRINT_JOB_REQUEST_TYPE * body, CREATE_PRINT_JOB_RESPONSE_TYPE_0 ** bodyOut) override;

	virtual HRESULT __stdcall SendDocument(SEND_DOCUMENT_REQUEST_TYPE * body, SEND_DOCUMENT_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall AddDocument(ADD_DOCUMENT_REQUEST_TYPE * body, ADD_DOCUMENT_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall CancelJob(CANCEL_JOB_REQUEST_TYPE * body, CANCEL_JOB_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall GetPrinterElements(GET_PRINTER_ELEMENTS_REQUEST_TYPE * body, GET_PRINTER_ELEMENTS_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall GetJobElements(GET_JOB_ELEMENTS_REQUEST_TYPE * body, GET_JOB_ELEMENTS_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall GetActiveJobs(GET_ACTIVE_JOBS_REQUEST_TYPE * body, GET_ACTIVE_JOBS_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall GetJobHistory(GET_JOB_HISTORY_REQUEST_TYPE * body, GET_JOB_HISTORY_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall SetEventRate(SET_EVENT_RATE_REQUEST_TYPE * body, SET_EVENT_RATE_RESPONSE_TYPE ** bodyOut) override;


	// Inherited via IPrinterServiceV12Type
	virtual HRESULT __stdcall SetPrinterElements(SET_PRINTER_ELEMENTS_REQUEST_TYPE * body, SET_PRINTER_ELEMENTS_RESPONSE_TYPE ** bodyOut) override;


	// Inherited via IPrinterServiceV20Type
	virtual HRESULT __stdcall GetPrintDeviceCapabilities(GET_PRINT_DEVICE_CAPABILITIES_REQUEST_TYPE * body, GET_PRINT_DEVICE_CAPABILITIES_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall GetPrintDeviceResources(GET_PRINT_DEVICE_RESOURCES_REQUEST_TYPE * body, GET_PRINT_DEVICE_RESOURCES_RESPONSE_TYPE ** bodyOut) override;

	virtual HRESULT __stdcall CreatePrintJob2(CREATE_PRINT_JOB2_REQUEST_TYPE * body, CREATE_PRINT_JOB_RESPONSE_TYPE_1 ** bodyOut) override;

	virtual HRESULT __stdcall PrepareToPrint(PREPARE_TO_PRINT_REQUEST_TYPE * body) override;

	virtual HRESULT __stdcall GetBidiSchemaExtensions(GET_BIDI_SCHEMA_EXTENSIONS_REQUEST_TYPE * body, GET_BIDI_SCHEMA_EXTENSIONS_RESPONSE_TYPE ** bodyOut) override;

private:
	CWSDPrinterService();

	friend class CReceiveAttachmentThread;

	void SendJobStatus(const PrinterJob& Job);

	template<typename T> static T* WSDInit(void* root, const PrinterJob& Job);

private:

	WSD_THIS_DEVICE_METADATA* m_ThisDeviceMetadata;

	bool m_bSupportsV11;
	bool m_bSupportsV12;
	bool m_bSupportsV20;

	IWSDDeviceHost* m_pHost;
	IWSDXMLContext* m_pContext;
	CPrinterServiceTypeEventSource* m_pEvents;
	CPrinterServiceV20TypeEventSource* m_pEvents20;
	HANDLE m_hPrinter;

	std::wstring m_UniquePrinterName;

	CPrinterInfo* m_PrinterInfo;
	CPrinterJobs* m_PrinterJobs;
};
