#include "pch.h"
#include "PrinterJobs.h"
#include "WSDUtils.h"
#include "WSDPrinterService.h"
#include "src_host/MyPrinterProxy.h"

#pragma comment(lib, "wsdapi.lib")

/*
* According to documentation, WSPrint1.2 and WSPrint2.0 shouldn't be exposed in Relationship section of
* the Device Profile for Web Services metadata for the WS-Print device.
* So, CreateMyPrinterHost currently exposes the main IPrinterServiceType interface, whereas IPrinterServiceType
* metadata exposes all PortTypes (WS-Print 1.0, 1.2 and 2.0).
* Moreover, both WS-Print 1.2 and 2.0 should be associated to the same WS-Print EndPoint: the easiest way to configure
* these services in this way is to have one service (IPrinterServiceType) with multiple ports (1.0, 1.2 and 2.0),
* instead of 3 distinct services/ports pairs.
* See WSPRINT_MULTI_SERVICES macro and wsdl/generate_code.bat usage.
* NOTE: to use WS-Print 2.0, it's not required to support WS-Print 1.1 and 1.2.
*/

template<> JOB_DESCRIPTION_TYPE* CWSDPrinterService::WSDInit<JOB_DESCRIPTION_TYPE>(void* root, const PrinterJob& Job)
{
	JOB_DESCRIPTION_TYPE* JobDescription = WSDAlloc<JOB_DESCRIPTION_TYPE>(root);
	JobDescription->JobName = WSDAlloc(root, Job.GetJobNameW());
	JobDescription->JobOriginatingUserName = WSDAlloc(root, Job.GetJobOriginatingUserNameW());
	JobDescription->Any = nullptr;

	return JobDescription;
}

template<> JOB_STATUS_TYPE* CWSDPrinterService::WSDInit<JOB_STATUS_TYPE>(void* root, const PrinterJob& Job)
{
	JOB_STATUS_TYPE* JobStatus = WSDAlloc<JOB_STATUS_TYPE>(root);
	JobStatus->JobId = Job.GetJobId();
	JobStatus->JobState = WSDAlloc<JOB_STATE_EXT_TYPE>(root);
	JobStatus->JobState->JobStateWKVTypeValue = WSDAlloc(root, Job.GetStatusWSD());
	JobStatus->JobState->KeywordNsExtensionPatternTypeValue = nullptr;
	JobStatus->JobStateReasons = WSDAlloc<JOB_STATE_REASONS_TYPE>(root);
	JobStatus->JobStateReasons->JobStateReason = WSDAlloc<JOB_STATE_REASON_EXT_TYPE_LIST>(root);
	JobStatus->JobStateReasons->JobStateReason->Element = WSDAlloc<JOB_STATE_REASON_EXT_TYPE>(root);
	JobStatus->JobStateReasons->JobStateReason->Element->JobStateReasonsWKVTypeValue = WSDAlloc(root, Job.GetReasonWSD());
	JobStatus->JobStateReasons->JobStateReason->Element->KeywordNsExtensionPatternTypeValue = nullptr;
	JobStatus->JobStateReasons->JobStateReason->Next = nullptr;
	JobStatus->KOctetsProcessed = Job.GetOctetsProcessed();
	JobStatus->MediaSheetsCompleted = Job.GetMediaSheetsCompleted();
	JobStatus->NumberOfDocuments = Job.GetNumberOfDocuments();
	JobStatus->Any = nullptr;

	return JobStatus;
}

template<> JOB_END_STATE_TYPE* CWSDPrinterService::WSDInit<JOB_END_STATE_TYPE>(void* root, const PrinterJob& Job)
{
	JOB_END_STATE_TYPE* JobEndState = WSDAlloc<JOB_END_STATE_TYPE>(root);
	JobEndState = WSDAlloc<JOB_END_STATE_TYPE>(root);
	JobEndState->JobId = Job.GetJobId();
	JobEndState->JobName = WSDAlloc(root, Job.GetJobNameW());
	JobEndState->JobOriginatingUserName = WSDAlloc(root, Job.GetJobOriginatingUserNameW());
	JobEndState->JobCompletedState = WSDAlloc<JOB_STATE_EXT_TYPE>(root);
	JobEndState->JobCompletedState->JobStateWKVTypeValue = WSDAlloc(root, Job.GetStatusWSD());
	JobEndState->JobCompletedState->KeywordNsExtensionPatternTypeValue = nullptr; //WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+"); //nullptr;
	JobEndState->JobCompletedStateReasons = WSDAlloc<JOB_STATE_REASONS_TYPE>(root);
	JobEndState->JobCompletedStateReasons->JobStateReason = WSDAlloc<JOB_STATE_REASON_EXT_TYPE_LIST>(root);
	JobEndState->JobCompletedStateReasons->JobStateReason->Element = WSDAlloc<JOB_STATE_REASON_EXT_TYPE>(root);
	JobEndState->JobCompletedStateReasons->JobStateReason->Element->JobStateReasonsWKVTypeValue = WSDAlloc(root, Job.GetReasonWSD());
	JobEndState->JobCompletedStateReasons->JobStateReason->Element->KeywordNsExtensionPatternTypeValue = nullptr; //WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+"); //nullptr;
	JobEndState->JobCompletedStateReasons->JobStateReason->Next = nullptr;
	JobEndState->KOctetsProcessed = Job.GetOctetsProcessed();
	JobEndState->MediaSheetsCompleted = Job.GetMediaSheetsCompleted();
	JobEndState->NumberOfDocuments = Job.GetNumberOfDocuments();
	JobEndState->Any = nullptr;

	return JobEndState;
}

template<> JOB_SUMMARY_TYPE* CWSDPrinterService::WSDInit<JOB_SUMMARY_TYPE>(void* root, const PrinterJob& Job)
{
	JOB_SUMMARY_TYPE* Summary = WSDAlloc<JOB_SUMMARY_TYPE>(root);
	Summary->JobId = Job.GetJobId();
	Summary->JobState = WSDAlloc<JOB_STATE_EXT_TYPE>(root);
	Summary->JobState->JobStateWKVTypeValue = WSDAlloc(root, Job.GetStatusWSD());
	Summary->JobState->KeywordNsExtensionPatternTypeValue = nullptr; //WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+"); //nullptr;
	Summary->JobStateReasons = WSDAlloc<JOB_STATE_REASONS_TYPE>(root);
	Summary->JobStateReasons->JobStateReason = WSDAlloc<JOB_STATE_REASON_EXT_TYPE_LIST>(root);
	Summary->JobStateReasons->JobStateReason->Element = WSDAlloc<JOB_STATE_REASON_EXT_TYPE>(root);
	Summary->JobStateReasons->JobStateReason->Element->JobStateReasonsWKVTypeValue = WSDAlloc(root, Job.GetReasonWSD());
	Summary->JobStateReasons->JobStateReason->Element->KeywordNsExtensionPatternTypeValue = nullptr; //WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+"); //nullptr;
	Summary->JobStateReasons->JobStateReason->Next = nullptr;
	Summary->JobName = WSDAlloc(root, Job.GetJobNameW());
	Summary->JobOriginatingUserName = WSDAlloc(root, Job.GetJobOriginatingUserNameW());
	Summary->KOctetsProcessed = Job.GetOctetsProcessed();
	Summary->MediaSheetsCompleted = Job.GetMediaSheetsCompleted();
	Summary->NumberOfDocuments = Job.GetNumberOfDocuments();
	Summary->Any = nullptr;

	return Summary;
}

IPrinterService* CWSDPrinterService::Create()
{
	return new CWSDPrinterService();
}

CWSDPrinterService::CWSDPrinterService() : 
	m_ThisDeviceMetadata(nullptr),
	m_bSupportsV11(false),
	m_bSupportsV12(false),
	m_bSupportsV20(false),
	m_pHost(nullptr),
	m_pContext(nullptr),
	m_pEvents(nullptr),
	m_pEvents20(nullptr),
	m_hPrinter(INVALID_HANDLE_VALUE),
	m_PrinterInfo(nullptr),
	m_PrinterJobs(nullptr)
{
}

CWSDPrinterService::~CWSDPrinterService()
{
	Uninit();
}

bool CWSDPrinterService::Init(CPrinterInfo* pPrinter)
{
	if (pPrinter == nullptr)
	{
		return false;
	}

	if (pPrinter->GetPrinterJobs() == nullptr)
	{
		return false;
	}

	m_PrinterInfo = pPrinter;
	m_PrinterInfo->AddRef();

	m_PrinterJobs = pPrinter->GetPrinterJobs();
	m_PrinterJobs->AddRef();

	const wchar_t* pPrinterName = m_PrinterInfo->GetPrinterName().c_str();

	if (!OpenPrinter2(pPrinterName, &m_hPrinter, nullptr/*pDefault*/, nullptr/*pOptions*/))
	{
		m_hPrinter = INVALID_HANDLE_VALUE;
		return false;
	}

	m_UniquePrinterName = m_PrinterInfo->GetUniquePrinterName();

	DBGLOG("LogicalAddress %ls - %ls", m_PrinterInfo->GetURNUUIDAddress().c_str(), m_UniquePrinterName.c_str());

	WSD_LOCALIZED_STRING thisDeviceName = { NULL, m_UniquePrinterName.c_str() };
	WSD_LOCALIZED_STRING_LIST thisDeviceNameList = { NULL, &thisDeviceName };
	WSD_THIS_DEVICE_METADATA thisDeviceMetadata = {
		&thisDeviceNameList,        // FriendlyName;
		L"0.1",                   // FirmwareVersion;
		L"010101-101010",   // SerialNumber;

	}; // thisDeviceMetadata

	//Override global thisModelMetadata declared in MyPrinterTypes.cpp
	thisModelMetadata.Manufacturer->Element->String = m_PrinterInfo->GetManufacturer().c_str();
	thisModelMetadata.ManufacturerUrl = m_PrinterInfo->GetUrl().size() ? m_PrinterInfo->GetUrl().c_str() : nullptr;
	thisModelMetadata.ModelName->Element->String = m_PrinterInfo->GetModel().c_str();
	thisModelMetadata.ModelNumber = m_PrinterInfo->GetModelNumber().size() ? m_PrinterInfo->GetModelNumber().c_str() : nullptr;
	thisModelMetadata.ModelUrl = m_PrinterInfo->GetUrl().size() ? m_PrinterInfo->GetUrl().c_str() : nullptr;
	thisModelMetadata.PresentationUrl = m_PrinterInfo->GetUrl().size() ? m_PrinterInfo->GetUrl().c_str() : nullptr;

	//Override hostMetadata.Host to have PrintDeviceType and IppEverywhere declared in Hello Types record. 
	WSD_NAME_LIST IppEverywhere = {	nullptr, NAME_IPP_EVERYWHERE_Print };
	WSD_NAME_LIST PrintDeviceType = { &IppEverywhere, NAME_PRINT_PrintDeviceType };
	WSD_ENDPOINT_REFERENCE EndPoint = { m_PrinterInfo->GetURNUUIDAddress().c_str() };
	WSD_ENDPOINT_REFERENCE_LIST EndPointList = { nullptr, &EndPoint };
	WSD_SERVICE_METADATA host = {};
	host.Types = &PrintDeviceType;
	host.EndpointReference = &EndPointList;
	host.ServiceId = L"WSDMobilePrinter";
	hostMetadata.Host = &host;

	m_bSupportsV11 = WSPRINT_SUPPORTS_1_1;
	m_bSupportsV12 = WSPRINT_SUPPORTS_1_2;
	m_bSupportsV20 = WSPRINT_SUPPORTS_2_0;	//V2.0 is not required if "MobilePrinter" category is specified in pnpxDeviceCategoryText.

#if	WSPRINT_MULTI_SERVICES
	if (hostMetadata.Hosted != nullptr)	//1.0
	{
		if (hostMetadata.Hosted->Next)	//1.2
		{
			WSD_SERVICE_METADATA_LIST* wsd12 = hostMetadata.Hosted->Next;
			WSD_SERVICE_METADATA_LIST* wsd20 = hostMetadata.Hosted->Next->Next;

			if (!m_bSupportsV12 && !m_bSupportsV20)
			{
				hostMetadata.Hosted->Next = nullptr;
			}
			else if (!m_bSupportsV12 /*&& m_bSupportsV20*/)
			{
				hostMetadata.Hosted->Next = hostMetadata.Hosted->Next->Next;	//2.0
			}
			else if (!m_bSupportsV20 /*&& m_bSupportsV12*/)
			{
				hostMetadata.Hosted->Next->Next = nullptr;
			}
		}
	}
#endif

	HRESULT hr = CreateMyPrinterHost(
					m_PrinterInfo->GetURNUUIDAddress().c_str(),
					&thisDeviceMetadata,
					this,
#if	WSPRINT_MULTI_SERVICES
					m_bSupportsV12 ? this : nullptr,
					m_bSupportsV20 ? this : nullptr,
#endif
					&m_pHost,
					&m_pContext);
	if (SUCCEEDED(hr))
	{
		hr = CreateCPrinterServiceTypeEventSource(m_pHost, L"http://schemas.microsoft.com/windows/2006/08/wdp/print/PrinterServiceType0", &m_pEvents);
	}
	if (SUCCEEDED(hr))
	{
		if (m_bSupportsV20)
		{
			hr = CreateCPrinterServiceV20TypeEventSource(m_pHost, L"http://schemas.microsoft.com/windows/2014/04/wdp/printV20/PrinterServiceV20Type0", &m_pEvents20);
		}
	}

	return SUCCEEDED(hr);
}

void CWSDPrinterService::Uninit()
{
	Stop();

	if (m_pHost != nullptr)
	{
		m_pHost->Terminate();
	}

	SAFE_RELEASE(m_pEvents20);
	SAFE_RELEASE(m_pEvents);
	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pHost);
	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		ClosePrinter(m_hPrinter);
		m_hPrinter = INVALID_HANDLE_VALUE;
	}

	SAFE_RELEASE(m_PrinterJobs);
	SAFE_RELEASE(m_PrinterInfo);

	m_UniquePrinterName.clear();
}

void CWSDPrinterService::Start()
{
	LOG2("[WSD] Starting %ls ... ", m_UniquePrinterName.c_str());
	if (m_pHost != nullptr)
	{
		m_pHost->Start(0, nullptr, nullptr);
	}
	LOG2("done.\n");
}

void CWSDPrinterService::Stop()
{
	LOG2("[WSD] Stopping %ls ... ", m_UniquePrinterName.c_str());
	if (m_pHost != nullptr)
	{
		m_pHost->Stop();
	}
	LOG2("done.\n");
}

void CWSDPrinterService::SendJobStatus(const PrinterJob& Job)
{
	if (m_pEvents)
	{
		bool bCompleted = Job.IsCompleted();

		if (bCompleted)
		{
			JOB_END_STATE_EVENT_TYPE* pBody = WSDAlloc<JOB_END_STATE_EVENT_TYPE>(nullptr);
			pBody->JobEndState = WSDInit<JOB_END_STATE_TYPE>(pBody, Job);

			m_pEvents->JobEndStateEvent(pBody);

			WSDFreeLinkedMemory(pBody);
		}
		else
		{
			JOB_STATUS_EVENT_TYPE* pBody = WSDAlloc<JOB_STATUS_EVENT_TYPE>(nullptr);
			pBody->JobStatus = WSDInit<JOB_STATUS_TYPE>(pBody, Job);
			pBody->Any = nullptr;

			m_pEvents->JobStatusEvent(pBody);

			WSDFreeLinkedMemory(pBody);
		}

		DBGLOG("JobId %d: %ls - Reason: %ls", Job.GetJobId(), Job.GetStatusWSD(), Job.GetReasonWSD());
	}
}

// WS-Print 1.0 Interfaces
HRESULT __stdcall CWSDPrinterService::CreatePrintJob(CREATE_PRINT_JOB_REQUEST_TYPE * body, CREATE_PRINT_JOB_RESPONSE_TYPE_0 ** bodyOut)
{
	DBGLOG("");

	const wchar_t* JobName = nullptr;
	const wchar_t* JobOriginatingUserName = nullptr;

	if (body->PrintTicket && body->PrintTicket->JobDescription)
	{
		VLOG("JobName: %ls", body->PrintTicket->JobDescription->JobName);
		VLOG("JobUser: %ls", body->PrintTicket->JobDescription->JobOriginatingUserName);

		JobName = body->PrintTicket->JobDescription->JobName;
		JobOriginatingUserName = body->PrintTicket->JobDescription->JobOriginatingUserName;
	}

	auto Job = m_PrinterJobs->CreatePrinterJob(m_PrinterInfo->GetPrinterName(), JobName, JobOriginatingUserName, [&](const PrinterJob& Job) { SendJobStatus(Job); });

	CREATE_PRINT_JOB_RESPONSE_TYPE_0* pResponse = WSDAlloc<CREATE_PRINT_JOB_RESPONSE_TYPE_0>(nullptr);
	pResponse->JobId = Job->GetJobId();
	*bodyOut = pResponse;

	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::SendDocument(SEND_DOCUMENT_REQUEST_TYPE * body, SEND_DOCUMENT_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("JobId %d", (int)body->JobId);

	HRESULT hr = S_OK;

	bool bIsLastDocument = body->LastDocument != 0;
	const wchar_t* DocumentFormat = DocumentFormatWKVType_application_vnd_ms_xpsdocument;	//Default

	if (body->DocumentDescription)
	{
		//Consider "unknown" as vnd.ms-xpsdocument (the value supported by this service).
		if (body->DocumentDescription->Format->DocumentFormatWKVTypeValue && (_wcsicmp(DocumentFormatWKVType_unknown, body->DocumentDescription->Format->DocumentFormatWKVTypeValue) != 0))
		{
			DocumentFormat = body->DocumentDescription->Format->DocumentFormatWKVTypeValue;
		}
			
		//return ClientErrorDocumentFormatNotSupported;

		if (body->DocumentDescription->Compression && (wcscmp(body->DocumentDescription->Compression->CompressionWKVTypeValue, CompressionWKVType_None) != 0))
		{
			//return ClientErrorCompressionNotSupported;
		}
	}

	auto JobStatus = m_PrinterJobs->PrintJob(body->JobId, body->DocumentData, DocumentFormat);
	if (!JobStatus)
	{
		//Job not found
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT __stdcall CWSDPrinterService::AddDocument(ADD_DOCUMENT_REQUEST_TYPE * body, ADD_DOCUMENT_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	return E_NOTIMPL;
}

HRESULT __stdcall CWSDPrinterService::CancelJob(CANCEL_JOB_REQUEST_TYPE * body, CANCEL_JOB_RESPONSE_TYPE ** bodyOut)
{
	HRESULT hr = S_OK;
	auto JobStatus = m_PrinterJobs->CancelJob(body->JobId);
	if (!JobStatus)
	{
		//Job not found
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT __stdcall CWSDPrinterService::GetPrinterElements(GET_PRINTER_ELEMENTS_REQUEST_TYPE * body, GET_PRINTER_ELEMENTS_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");

	GET_PRINTER_ELEMENTS_RESPONSE_TYPE* response = WSDAlloc<GET_PRINTER_ELEMENTS_RESPONSE_TYPE>(nullptr);
	response->PrinterElements = WSDAlloc<PRINTER_ELEMENTS_TYPE>(response);

	PRINTER_ELEMENT_DATA_TYPE_LIST** pNextData = &response->PrinterElements->ElementData;

	WSDXML_NAME_LIST* StartList = body->RequestedElements->Name;
	for (; StartList != nullptr; StartList = StartList->Next)
	{
		PRINTER_ELEMENT_DATA_TYPE_LIST* pNewElement = WSDAlloc<PRINTER_ELEMENT_DATA_TYPE_LIST>(response);
		pNewElement->Element = WSDAlloc<PRINTER_ELEMENT_DATA_TYPE>(response);
		pNewElement->Next = nullptr;

		*pNextData = pNewElement;
		pNextData = &pNewElement->Next;

		PRINTER_ELEMENT_DATA_TYPE* root = pNewElement->Element;
		const wchar_t* ElementName = StartList->Element->LocalName;
		const wchar_t* PreferredNameSpace = (StartList->Element->Space && StartList->Element->Space->PreferredPrefix) ? StartList->Element->Space->PreferredPrefix : nullptr;

		DBGLOG("\t%ls%ls%ls", PreferredNameSpace ? PreferredNameSpace : L"", PreferredNameSpace ? L":" : L"", ElementName);

		if (AreSameName(StartList->Element, NAME_PRINT_PrinterDescription))
		{
			root->Name = NAME_PRINT_PrinterDescription;
			root->Valid = TRUE;

			root->PrinterDescription = WSDAlloc<PRINTER_DESCRIPTION_TYPE>(root);
			root->PrinterDescription->ColorSupported = m_PrinterInfo->ColorSupported() ? TRUE : FALSE;
			root->PrinterDescription->DeviceId = WSDAlloc(root, m_PrinterInfo->GetDeviceId());
			root->PrinterDescription->MultipleDocumentJobsSupported = WSDAlloc<BOOLEAN>(root, FALSE);
			root->PrinterDescription->PagesPerMinute = m_PrinterInfo->GetPrintRatePPM();
			root->PrinterDescription->PagesPerMinuteColor = WSDAlloc<LONG>(root, m_PrinterInfo->GetPrintRatePPM());
			WSDAppend(root, root->PrinterDescription->PrinterName, m_PrinterInfo->GetPrinterName());
			WSDAppend(root, root->PrinterDescription->PrinterInfo, m_PrinterInfo->GetPrinterInfo());
			WSDAppend(root, root->PrinterDescription->PrinterLocation, m_PrinterInfo->GetPrinterLocation());
			if (m_bSupportsV11)
			{
				WSDAppend(root->PrinterDescription->Any, WSDAllocElementWithText(root, NAME_PRINTV11_SupportsWSPrintv11, L"true"));
				WSDAppend(root->PrinterDescription->Any, WSDAllocElementWithText(root, NAME_PRINTV11_DeviceModelId, GuidToString(m_PrinterInfo->GetUUID())));
			}
			if (m_bSupportsV12)
			{
				WSDAppend(root->PrinterDescription->Any, WSDAllocElementWithText(root, NAME_PRINTV12_SupportsWSPrintV12, L"true"));
			}
			if (m_bSupportsV20)
			{
				WSDAppend(root->PrinterDescription->Any, WSDAllocElementWithText(root, NAME_PRINTV20_SupportsWSPrintV20, L"true"));
			}
		}
		else if (AreSameName(StartList->Element, NAME_PRINT_PrinterConfiguration))
		{
			root->Name = NAME_PRINT_PrinterConfiguration;
			root->Valid = TRUE;

			/*	[PRINTER_CONFIGURATION_TYPE]
			LONG* PrinterEventRate;
			STORAGE_BASE_TYPE* Storage; // optional
			CONSUMABLES_TYPE* Consumables; // optional
			INPUT_BINS_TYPE* InputBins; // optional
			FINISHINGS_TYPE* Finishings; // optional
			OUTPUT_BINS_TYPE* OutputBins; // optional
			WSDXML_ELEMENT* Any;
			*/
			root->PrinterConfiguration = WSDAlloc<PRINTER_CONFIGURATION_TYPE>(root);
			root->PrinterConfiguration->PrinterEventRate = WSDAlloc<LONG>(root, m_PrinterJobs->GetEventRate());
			root->PrinterConfiguration->Any = nullptr;

			//Consumable: NAME_PRINTV11_ColorRepresentation
		}
		else if (AreSameName(StartList->Element, NAME_PRINT_PrinterStatus))
		{
			root->Name = NAME_PRINT_PrinterStatus;
			root->Valid = TRUE;
			/*	[PRINTER_STATUS_TYPE]
			WSD_DATETIME* PrinterCurrentTime;
			PRINTER_STATE_BASE_TYPE* PrinterState;
			PRINTER_STATE_REASON_BASE_TYPE* PrinterPrimaryStateReason;
			PRINTER_STATE_REASONS_TYPE* PrinterStateReasons; // optional
			LONG QueuedJobCount;
			ACTIVE_CONDITION_TABLE_TYPE* ActiveCondition; // optional
			CONDITION_HISTORY_TABLE_TYPE* ConditionHistory; // optional
			WSDXML_ELEMENT* Any;
			*/

			LONG QueuedJobCount = m_PrinterJobs->GetNumActiveJobs();

			SYSTEMTIME st = {};
			//GetSystemTime(&st);
			GetLocalTime(&st);
			WSD_DATETIME dt = { TRUE, st.wYear, (UCHAR)st.wMonth, (UCHAR)st.wDay, (UCHAR)st.wHour, (UCHAR)st.wMinute, (UCHAR)st.wSecond, st.wMilliseconds, TRUE };
			root->PrinterStatus = WSDAlloc<PRINTER_STATUS_TYPE>(root);
			root->PrinterStatus->PrinterCurrentTime = WSDAlloc<WSD_DATETIME>(root, dt);
			root->PrinterStatus->PrinterState = WSDAlloc<PRINTER_STATE_BASE_TYPE>(root);
			root->PrinterStatus->PrinterState->KeywordNsExtensionPatternTypeValue = nullptr;// WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+");
			root->PrinterStatus->PrinterState->PrinterStateWKVTypeValue = WSDAlloc(root, (QueuedJobCount > 0) ? PrinterStateWKVType_Processing : PrinterStateWKVType_Idle);
			root->PrinterStatus->PrinterPrimaryStateReason = WSDAlloc<PRINTER_STATE_REASON_BASE_TYPE>(root);
			root->PrinterStatus->PrinterPrimaryStateReason->KeywordNsExtensionPatternTypeValue = nullptr;//WSDAlloc(root, L"\\w+:[\\w_\\-\\.]+");
			root->PrinterStatus->PrinterPrimaryStateReason->PrinterStateReasonsWKVTypeValue = WSDAlloc(root, PrinterStateReasonsWKVType_None);
			root->PrinterStatus->QueuedJobCount = QueuedJobCount;
			root->PrinterStatus->Any = nullptr;
		}
		else if (AreSameName(StartList->Element, NAME_PRINT_DefaultPrintTicket))
		{
			root->Name = NAME_PRINT_DefaultPrintTicket;
			root->Valid = TRUE;

			/*	[PRINT_TICKET_TYPE]
			JOB_DESCRIPTION_TYPE* JobDescription; // optional
			JOB_PROCESSING_TYPE* JobProcessing; // optional
			DOCUMENT_PROCESSING_TYPE* DocumentProcessing; // optional
			WSDXML_ELEMENT* Any;
			*/
			root->DefaultPrintTicket = WSDAlloc<PRINT_TICKET_TYPE>(root);

			root->DefaultPrintTicket->DocumentProcessing = WSDAlloc<DOCUMENT_PROCESSING_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->MustHonorMediaSizeName = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->DocumentProcessing->MediaSizeName = WSDAlloc<MEDIA_SIZE_NAME_EXT_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->MediaSizeName->MediaSizeSelfDescribingNameWKVTypeValue = WSDAlloc(root, MediaSizeSelfDescribingNameWKVType_iso_a4_210x297mm);
			root->DefaultPrintTicket->DocumentProcessing->MediaSizeName->MediaSizeNameExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->MustHonorMediaType = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->DocumentProcessing->MediaType = WSDAlloc<MEDIA_TYPE_EXT_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->MediaType->MediaTypeWKVTypeValue = WSDAlloc(root, MediaTypeWKVType_unknown);
			root->DefaultPrintTicket->DocumentProcessing->MediaType->MediaTypeExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->MustHonorMediaColor = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->DocumentProcessing->MediaColor = WSDAlloc<MEDIA_COLOR_EXT_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->MediaColor->MediaColorWKVTypeValue = WSDAlloc(root, MediaColorWKVType_unknown);
			root->DefaultPrintTicket->DocumentProcessing->MediaColor->MediaColorExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->NumberUp = nullptr; // optional
			root->DefaultPrintTicket->DocumentProcessing->MustHonorOrientation = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->DocumentProcessing->Orientation = WSDAlloc<ORIENTATION_BASE_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->Orientation->OrientationWKVTypeValue = WSDAlloc(root, OrientationWKVType_Portrait);
			root->DefaultPrintTicket->DocumentProcessing->Orientation->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->Resolution = nullptr; // optional
			root->DefaultPrintTicket->DocumentProcessing->MustHonorPrintQuality = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->DocumentProcessing->PrintQuality = WSDAlloc<PRINT_QUALITY_BASE_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->PrintQuality->PrintQualityWKVTypeValue = WSDAlloc(root, PrintQualityWKVType_Normal);
			root->DefaultPrintTicket->DocumentProcessing->PrintQuality->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->MustHonorSides = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->DocumentProcessing->Sides = WSDAlloc<SIDES_BASE_TYPE>(root);
			root->DefaultPrintTicket->DocumentProcessing->Sides->SidesWKVTypeValue = WSDAlloc(root, SidesWKVType_OneSided);
			root->DefaultPrintTicket->DocumentProcessing->Sides->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->DocumentProcessing->Any = nullptr;

			root->DefaultPrintTicket->JobDescription = WSDAlloc<JOB_DESCRIPTION_TYPE>(root);
			root->DefaultPrintTicket->JobDescription->JobName = WSDAlloc(root, L"MyJobName");
			root->DefaultPrintTicket->JobDescription->JobOriginatingUserName = WSDAlloc(root, L"MyJobOriginatingUserName");
			root->DefaultPrintTicket->JobDescription->Any = nullptr;

			root->DefaultPrintTicket->JobProcessing = WSDAlloc<JOB_PROCESSING_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->MustHonorCopies = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->JobProcessing->Copies = WSDAlloc<LONG>(root, 1L);
			root->DefaultPrintTicket->JobProcessing->JobFinishings = WSDAlloc<JOB_FINISHINGS_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch = WSDAlloc<HOLE_PUNCH_DETAILS_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Edge = WSDAlloc<HOLE_PUNCH_EDGE_EXT_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Edge->HolePunchEdgeWKVTypeValue = WSDAlloc(root, HolePunchEdgeWKVType_None);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Edge->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Pattern = WSDAlloc<HOLE_PUNCH_PATTERN_EXT_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Pattern->HolePunchPatternWKVTypeValue = WSDAlloc(root, HolePunchPatternWKVType_NotApplicable);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Pattern->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->MustHonor = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->HolePunch->Any = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->JogOffset = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->MustHonorCollate = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->MustHonorJogOffset = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple = WSDAlloc<STAPLE_DETAILS_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->MustHonorAngle = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Location = WSDAlloc<STAPLE_LOCATION_EXT_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Location->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Location->StapleLocationWKVTypeValue = WSDAlloc(root, StapleLocationWKVType_None);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->MustHonorLocation = FALSE;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Angle = WSDAlloc<STAPLE_ANGLE_EXT_TYPE>(root);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Angle->KeywordNsExtensionPatternTypeValue = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Angle->StapleAngleWKVTypeValue = WSDAlloc(root, StapleAngleWKVType_NotApplicable);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Staple->Any = nullptr;
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Collate = WSDAlloc<BOOLEAN>(root, FALSE);
			root->DefaultPrintTicket->JobProcessing->JobFinishings->Any = nullptr;
			root->DefaultPrintTicket->JobProcessing->MustHonorPriority = WSDAlloc<BOOLEAN>(root, TRUE);
			root->DefaultPrintTicket->JobProcessing->Priority = WSDAlloc<LONG>(root, 1L);
			root->DefaultPrintTicket->JobProcessing->Any = nullptr;

			root->DefaultPrintTicket->Any = nullptr;
		}
		else if (AreSameName(StartList->Element, NAME_PRINT_PrinterCapabilities))
		{
			root->Name = NAME_PRINT_PrinterCapabilities;
			root->Valid = TRUE;

			/*	[PRINTER_CAPABILITIES_TYPE]
			JOB_VALUES_TYPE* JobValues; // optional
			DOCUMENT_VALUES_TYPE* DocumentValues; // optional
			WSDXML_ELEMENT* Any;
			*/

			//TODO: usare CreateDocumentPackageTargetForPrintJob per ricevere sempre xps e mandarli alla stampante
			root->PrinterCapabilities = WSDAlloc<PRINTER_CAPABILITIES_TYPE>(root);
			root->PrinterCapabilities->DocumentValues = WSDAlloc<DOCUMENT_VALUES_TYPE>(root);
			root->PrinterCapabilities->DocumentValues->DocumentDescription = WSDAlloc<DOCUMENT_DESCRIPTION_0>(root);
			root->PrinterCapabilities->DocumentValues->DocumentDescription->Format = WSDAllocStringList(root,
				{
					/*DocumentFormatWKVType_application_octet_stream,
					DocumentFormatWKVType_application_postscript,*/
					//DocumentFormatWKVType_application_vnd_hp_PCL,
					DocumentFormatWKVType_application_vnd_ms_xpsdocument,
					/*DocumentFormatWKVType_application_vnd_pwg_xhtml_print_xml,
					DocumentFormatWKVType_image_g3fax,
					DocumentFormatWKVType_image_gif,
					DocumentFormatWKVType_image_jpeg,
					DocumentFormatWKVType_image_png,
					DocumentFormatWKVType_image_tiff,
					DocumentFormatWKVType_image_tiff_fx,
					DocumentFormatWKVType_text_html,
					DocumentFormatWKVType_text_plain,
					DocumentFormatWKVType_text_plain_charset_utf_8,
					DocumentFormatWKVType_unknown,*/

					//L"application/oxps",
					//L"application/vnd.ms-xpsdocument,
					//L"image/pwg-raster",
					//L"application/pclm"

				});
			root->PrinterCapabilities->DocumentValues->DocumentDescription->Compression = WSDAllocTokenList(root, { CompressionWKVType_None/*,
				CompressionWKVType_Gzip , CompressionWKVType_Compress , CompressionWKVType_Deflate*/ });
			root->PrinterCapabilities->DocumentValues->Any = nullptr;
			{
				root->PrinterCapabilities->JobValues = WSDAlloc<JOB_VALUES_TYPE>(root);
				root->PrinterCapabilities->JobValues->DocumentProcessing = WSDAlloc<DOCUMENT_PROCESSING_0>(root);
				{
					root->PrinterCapabilities->JobValues->DocumentProcessing->MediaSizeName = WSDAllocTokenList(root,
						{ MediaSizeSelfDescribingNameWKVType_iso_a4_210x297mm/*, MediaSizeSelfDescribingNameWKVType_iso_c5_162x229mm,
							MediaSizeSelfDescribingNameWKVType_iso_dl_110x220mm , MediaSizeSelfDescribingNameWKVType_jis_b4_257x364mm ,
							MediaSizeSelfDescribingNameWKVType_na_legal_8_5x14in , MediaSizeSelfDescribingNameWKVType_na_letter_8_5x11in,
							MediaSizeSelfDescribingNameWKVType_pwg_letter_or_a4_choice*/ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->MediaType = WSDAllocTokenList(root, { /*MediaTypeWKVType_cardstock, MediaTypeWKVType_envelope ,
						MediaTypeWKVType_labels , MediaTypeWKVType_photographic , MediaTypeWKVType_photographic_glossy , MediaTypeWKVType_photographic_matte ,*/
						MediaTypeWKVType_stationery ,MediaTypeWKVType_stationery_inkjet/*, MediaTypeWKVType_transparency, MediaTypeWKVType_other,MediaTypeWKVType_unknown */ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->MediaColor = WSDAllocTokenList(root, { MediaColorWKVType_unknown/*,
						MediaColorWKVType_white, MediaColorWKVType_pink, MediaColorWKVType_yellow,MediaColorWKVType_buff, MediaColorWKVType_goldenrod,
						MediaColorWKVType_blue, MediaColorWKVType_green, MediaColorWKVType_red, MediaColorWKVType_gray, MediaColorWKVType_ivory,
						MediaColorWKVType_orange, MediaColorWKVType_other, MediaColorWKVType_no_color */ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp = WSDAlloc<NUMBER_UP_0>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->PagesPerSheet = WSDAlloc<VALUE_INT_LIST_TYPE>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->PagesPerSheet->AllowedValue = WSDAlloc<LONG_LIST>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->PagesPerSheet->AllowedValue->Element = 1;
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->PagesPerSheet->AllowedValue->Next = nullptr;
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->Direction = WSDAllocTokenList(root, { NUpDirectionRestrictionType_RightDown/*, NUpDirectionRestrictionType_DownRight ,
						NUpDirectionRestrictionType_LeftDown , NUpDirectionRestrictionType_DownLeft */ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->NumberUp->Any = nullptr;
					root->PrinterCapabilities->JobValues->DocumentProcessing->Orientation = WSDAllocTokenList(root, { OrientationWKVType_Landscape, OrientationWKVType_Portrait ,
						/*	OrientationWKVType_ReverseLandscape , OrientationWKVType_ReversePortrait*/ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution = WSDAlloc<RESOLUTION_0>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution->AllowedValue = WSDAlloc<RESOLUTION_ENTRY_TYPE_LIST>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution->AllowedValue->Element = WSDAlloc<RESOLUTION_ENTRY_TYPE>(root);
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution->AllowedValue->Element->Width = 300;
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution->AllowedValue->Element->Height = WSDAlloc<LONG>(root, 300);
					root->PrinterCapabilities->JobValues->DocumentProcessing->Resolution->AllowedValue->Next = nullptr;
					root->PrinterCapabilities->JobValues->DocumentProcessing->PrintQuality = WSDAllocTokenList(root, { PrintQualityWKVType_Draft, PrintQualityWKVType_High,
						PrintQualityWKVType_Normal, PrintQualityWKVType_Photo });
					root->PrinterCapabilities->JobValues->DocumentProcessing->Sides = WSDAllocTokenList(root, { SidesWKVType_OneSided/*, SidesWKVType_TwoSidedLongEdge ,SidesWKVType_TwoSidedShortEdge*/ });
					root->PrinterCapabilities->JobValues->DocumentProcessing->Any = nullptr;
				}

				root->PrinterCapabilities->JobValues->JobProcessing = WSDAlloc<JOB_PROCESSING_0>(root);
				{
					root->PrinterCapabilities->JobValues->JobProcessing->Copies = WSDAlloc<VALUE_INT_RANGE_TYPE>(root);
					root->PrinterCapabilities->JobValues->JobProcessing->Copies->MinValue = WSDAlloc<LONG>(root, m_PrinterInfo->GetMinNumCopies());
					root->PrinterCapabilities->JobValues->JobProcessing->Copies->MaxValue = m_PrinterInfo->GetMaxNumCopies();
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings = WSDAlloc<JOB_FINISHINGS_0>(root);
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->Staple = WSDAlloc<STAPLE_0>(root);
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->Staple->Location = WSDAllocTokenList(root, { StapleLocationWKVType_None /*,
						StapleLocationWKVType_StapleBottomLeft , StapleLocationWKVType_StapleBottomRight ,StapleLocationWKVType_StapleTopLeft,
						StapleLocationWKVType_StapleTopRight , StapleLocationWKVType_StapleDualBottom , StapleLocationWKVType_StapleDualLeft ,
						StapleLocationWKVType_StapleDualRight , StapleLocationWKVType_StapleDualTop , StapleLocationWKVType_SaddleStitch ,
						StapleLocationWKVType_other ,StapleLocationWKVType_unknown*/ });
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->Staple->Angle = WSDAllocTokenList(root, { /*StapleAngleWKVType_unknown,*/ StapleAngleWKVType_NotApplicable/*,
						StapleAngleWKVType_Any , StapleAngleWKVType_Horizontal , StapleAngleWKVType_Slanted , StapleAngleWKVType_Vertical*/ });
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->Staple->Any = nullptr;
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->HolePunch = WSDAlloc<HOLE_PUNCH_0>(root);
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->HolePunch->Edge = WSDAllocTokenList(root, { HolePunchEdgeWKVType_None/*, HolePunchEdgeWKVType_Top ,
						HolePunchEdgeWKVType_Bottom , HolePunchEdgeWKVType_Left , HolePunchEdgeWKVType_Right*/ });
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->HolePunch->Pattern = WSDAllocTokenList(root, { /*HolePunchPatternWKVType_unknown, */
						HolePunchPatternWKVType_NotApplicable/*, HolePunchPatternWKVType_TwoHoleUSTop , HolePunchPatternWKVType_ThreeHoleUS ,
						HolePunchPatternWKVType_TwoHoleDIN , HolePunchPatternWKVType_FourHoleDIN , HolePunchPatternWKVType_TwentyTwoHoleUS ,
						HolePunchPatternWKVType_NineteenHoleUS , HolePunchPatternWKVType_TwoHoleMetric , HolePunchPatternWKVType_Swedish4Hole ,
						HolePunchPatternWKVType_TwoHoleUSSide , HolePunchPatternWKVType_FiveHoleUS , HolePunchPatternWKVType_SevenHoleUS ,
						HolePunchPatternWKVType_Mixed7H4S , HolePunchPatternWKVType_Norweg6Hole , HolePunchPatternWKVType_Metric26Hole , HolePunchPatternWKVType_Metric30Hole */ });
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->HolePunch->Any = nullptr;
					root->PrinterCapabilities->JobValues->JobProcessing->JobFinishings->Any = nullptr;
					root->PrinterCapabilities->JobValues->JobProcessing->Priority = WSDAlloc<VALUE_INT_RANGE_TYPE>(root);
					root->PrinterCapabilities->JobValues->JobProcessing->Priority->MinValue = WSDAlloc<LONG>(root, 1);
					root->PrinterCapabilities->JobValues->JobProcessing->Priority->MaxValue = 100;

					root->PrinterCapabilities->JobValues->JobProcessing->Any = nullptr;
				}

				root->PrinterCapabilities->JobValues->Any = nullptr;
			}
			root->PrinterCapabilities->Any = nullptr;
			{
				root->PrinterCapabilities->Any = WSDAlloc<WSDXML_ELEMENT>(root);
				root->PrinterCapabilities->Any->Node = { WSDXML_NODE::ElementType, nullptr, nullptr };
				root->PrinterCapabilities->Any->Name = NAME_PRINTV20_PrinterFormats;
				root->PrinterCapabilities->Any->FirstAttribute = nullptr;
				root->PrinterCapabilities->Any->PrefixMappings = nullptr;

				/*		static WSDXML_NAME ExtraNames_PrintV20[] =
						{ { &Namespace_PrintV20, L"PrintDeviceCapabilitiesFormats" }
							,{ &Namespace_PrintV20, L"PrintJobTicketFormats" }
							,{ &Namespace_PrintV20, L"PrintDeviceResourceFormats" }
						};*/

			//	WSDAllocElementWithTokenList(root, root->PrinterCapabilities->Any, NAME_PRINTV20_PrintDeviceCapabilitiesFormats, { L"application/vnd.ms-PrintDeviceCapabilities+xml" });
			//	WSDAllocElementWithTokenList(root, root->PrinterCapabilities->Any, NAME_PRINTV20_PrintJobTicketFormats, { L"application/vnd.ms-PrintSchemaTicket+xml" });
			//	WSDAllocElementWithTokenList(root, root->PrinterCapabilities->Any, NAME_PRINTV20_PrintDeviceResourceFormats, { L"application/vnd.ms-resx+xml" });
			}

			root->Any = NULL;
		}
		else
		{
			if (AreSameName(StartList->Element, NAME_PRINTV11_DriverConfiguration))
			{
			}
			root->Name = StartList->Element;
			root->Valid = FALSE;
		}
	}

	*bodyOut = response;

	return S_OK;
}



HRESULT __stdcall CWSDPrinterService::GetJobElements(GET_JOB_ELEMENTS_REQUEST_TYPE * body, GET_JOB_ELEMENTS_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("JobId %d", (int)body->JobId);

	auto Job = m_PrinterJobs->FinishJob(body->JobId);
	if (!Job)
	{
		//return ClientErrorJobIdNotFound;
	}

	GET_JOB_ELEMENTS_RESPONSE_TYPE* pResponse = WSDAlloc<GET_JOB_ELEMENTS_RESPONSE_TYPE>(nullptr);
	pResponse->JobElements = WSDAlloc<JOB_ELEMENTS_TYPE>(pResponse);

	JOB_ELEMENT_DATA_TYPE_LIST** pNext = &pResponse->JobElements->ElementData;

	if (body->RequestedElements)
	{
		auto Name = body->RequestedElements->Name;
		while (Name)
		{
			if (Name->Element && Name->Element->LocalName)
			{
				DBGLOG("JobId %d - Element %ls", (int)body->JobId, Name->Element->LocalName);

				JOB_ELEMENT_DATA_TYPE_LIST* pElementList = WSDAlloc<JOB_ELEMENT_DATA_TYPE_LIST>(pResponse);
				*pNext = pElementList;
				pNext = &pElementList->Next;

				JOB_ELEMENT_DATA_TYPE* pElement = WSDAlloc<JOB_ELEMENT_DATA_TYPE>(pResponse);
				pElementList->Element = pElement;
				pElement->Name = Name->Element;

				if (Job && AreSameName(Name->Element, NAME_PRINT_JobStatus))
				{
					pElement->Valid = TRUE;
					pElement->JobStatus = WSDInit<JOB_STATUS_TYPE>(pResponse, *Job.get());
				}
				else if (Job && AreSameName(Name->Element, NAME_PRINT_PrintTicket))
				{
					pElement->Valid = TRUE;
					if (pElement->Valid)
					{
						pElement->PrintTicket = WSDAlloc<PRINT_TICKET_TYPE>(pResponse);
						pElement->PrintTicket->JobDescription = WSDInit<JOB_DESCRIPTION_TYPE>(pResponse, *Job.get());
						pElement->PrintTicket->DocumentProcessing = nullptr;
						pElement->PrintTicket->JobProcessing = nullptr;
					}
				}
				else if (Job && AreSameName(Name->Element, NAME_PRINT_Documents))
				{
					pElement->Valid = TRUE;
				}
				else
				{
					pElement->Valid = FALSE;
				}
			}
			Name = Name->Next;
		}
	}

	*bodyOut = pResponse;

	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::GetActiveJobs(GET_ACTIVE_JOBS_REQUEST_TYPE * body, GET_ACTIVE_JOBS_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");

	GET_ACTIVE_JOBS_RESPONSE_TYPE* pResponse = WSDAlloc<GET_ACTIVE_JOBS_RESPONSE_TYPE>(nullptr);
	pResponse->ActiveJobs = WSDAlloc<LIST_OF_SUMMARYS_TYPE>(pResponse);
	pResponse->ActiveJobs->JobSummary = WSDAlloc<JOB_SUMMARY_TYPE_LIST>(pResponse);

	JOB_SUMMARY_TYPE_LIST** pCurrent = &pResponse->ActiveJobs->JobSummary;

	auto jobs = m_PrinterJobs->GetActiveJobs();

	for (auto it = jobs.cbegin(); it != jobs.end(); it++)
	{
		const PrinterJob& Job = *it->get();

		JOB_SUMMARY_TYPE_LIST* pSummary = WSDAlloc<JOB_SUMMARY_TYPE_LIST>(pResponse);
		*pCurrent = pSummary;

		pSummary->Element = WSDInit<JOB_SUMMARY_TYPE>(pResponse, Job);

		pCurrent = &pSummary->Next;
	}

	*bodyOut = pResponse;

	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::GetJobHistory(GET_JOB_HISTORY_REQUEST_TYPE * body, GET_JOB_HISTORY_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	
	GET_JOB_HISTORY_RESPONSE_TYPE* pResponse = WSDAlloc<GET_JOB_HISTORY_RESPONSE_TYPE>(nullptr);
	pResponse->JobHistory = WSDAlloc<LIST_OF_SUMMARYS_TYPE>(pResponse);
	pResponse->JobHistory->JobSummary = WSDAlloc<JOB_SUMMARY_TYPE_LIST>(pResponse);

	JOB_SUMMARY_TYPE_LIST** pCurrent = &pResponse->JobHistory->JobSummary;

	auto jobs = m_PrinterJobs->GetJobsHistory();

	for (auto it = jobs.cbegin(); it != jobs.end(); it++)
	{
		const PrinterJob& Job = *it->get();

		JOB_SUMMARY_TYPE_LIST* pSummary = WSDAlloc<JOB_SUMMARY_TYPE_LIST>(pResponse);
		*pCurrent = pSummary;

		pSummary->Element = WSDInit<JOB_SUMMARY_TYPE>(pResponse, Job);

		pCurrent = &pSummary->Next;
	}

	*bodyOut = pResponse;

	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::SetEventRate(SET_EVENT_RATE_REQUEST_TYPE * body, SET_EVENT_RATE_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("%d ", (int)body->EventRate);

	m_PrinterJobs->SetEventRate(body->EventRate);
	return S_OK;
}

// WS-Print 1.2 Interfaces
HRESULT __stdcall CWSDPrinterService::SetPrinterElements(SET_PRINTER_ELEMENTS_REQUEST_TYPE * body, SET_PRINTER_ELEMENTS_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	return E_NOTIMPL;
}

// WS-Print 2.0 Interfaces
HRESULT __stdcall CWSDPrinterService::GetPrintDeviceCapabilities(GET_PRINT_DEVICE_CAPABILITIES_REQUEST_TYPE * body, GET_PRINT_DEVICE_CAPABILITIES_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	return E_NOTIMPL;
}

HRESULT __stdcall CWSDPrinterService::GetPrintDeviceResources(GET_PRINT_DEVICE_RESOURCES_REQUEST_TYPE * body, GET_PRINT_DEVICE_RESOURCES_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	return E_NOTIMPL;
}

HRESULT __stdcall CWSDPrinterService::CreatePrintJob2(CREATE_PRINT_JOB2_REQUEST_TYPE * body, CREATE_PRINT_JOB_RESPONSE_TYPE_1 ** bodyOut)
{
	DBGLOG("");

	const wchar_t* JobName = nullptr;
	const wchar_t* JobOriginatingUserName = nullptr;

	if (body->JobDescription2)
	{
		VLOG("JobName: %ls", body->JobDescription2->JobName);
		VLOG("JobUser: %ls", body->JobDescription2->JobOriginatingUserName);

		JobName = body->JobDescription2->JobName;
		JobOriginatingUserName = body->JobDescription2->JobOriginatingUserName;
	}

	auto Job = m_PrinterJobs->CreatePrinterJob(m_PrinterInfo->GetPrinterName(), JobName, JobOriginatingUserName, [&](const PrinterJob& Job) { SendJobStatus(Job); });

	CREATE_PRINT_JOB_RESPONSE_TYPE_1* pResponse = WSDAlloc<CREATE_PRINT_JOB_RESPONSE_TYPE_1>(nullptr);
	pResponse->JobId = Job->GetJobId();
	*bodyOut = pResponse;

	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::PrepareToPrint(PREPARE_TO_PRINT_REQUEST_TYPE * body)
{
	DBGLOG("");
	//Warmup the printer. No response required.
	return S_OK;
}

HRESULT __stdcall CWSDPrinterService::GetBidiSchemaExtensions(GET_BIDI_SCHEMA_EXTENSIONS_REQUEST_TYPE * body, GET_BIDI_SCHEMA_EXTENSIONS_RESPONSE_TYPE ** bodyOut)
{
	DBGLOG("");
	return E_NOTIMPL;
}
