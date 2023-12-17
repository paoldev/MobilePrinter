#include "pch.h"
#include "PrinterInterface.h"
#include "PrintDocumentMonitor.h"

#include <xpsprint.h>
#include <documenttarget.h>
#include <xpsobjectmodel_1.h>

/***********************************************************************************************************************************/

BasePrinter::BasePrinter()
{
}

BasePrinter::~BasePrinter()
{
}

HRESULT BasePrinter::Read(_Out_writes_bytes_to_(cb, *pcbRead) void *pv, _In_ ULONG cb, _Out_opt_ ULONG *pcbRead)
{
	return E_NOTIMPL;
}

HRESULT BasePrinter::Print(_In_reads_bytes_(cb) const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	const uint8_t* pBuffer = reinterpret_cast<const uint8_t*>(pv);
	
	ULONG dwBytesLeft = cb;

	HRESULT hr = S_OK;
	while (SUCCEEDED(hr) && (dwBytesLeft > 0))
	{
		ULONG dwWritten = 0;

		hr = Write(pBuffer + (cb - dwBytesLeft), dwBytesLeft, &dwWritten);

		if (SUCCEEDED(hr) && (dwWritten == 0))
		{
			hr = E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			dwBytesLeft -= dwWritten;
		}
	}

	if (pcbWritten)
	{
		*pcbWritten = cb - dwBytesLeft;
	}

	return hr;
}

/***********************************************************************************************************************************/
PrintToFile::PrintToFile() : m_hrPrinter(S_OK), m_hFile(INVALID_HANDLE_VALUE)
{
}

PrintToFile::~PrintToFile()
{
	Close(nullptr);
}

HRESULT PrintToFile::Create(LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if ((lpDocumentName == nullptr) || (o_ppPrinter == nullptr))
	{
		return E_POINTER;
	}

	*o_ppPrinter = nullptr;

	PrintToFile* pPrinter = new PrintToFile();
	if (pPrinter == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	CreateDirectoryFromFileName(lpDocumentName);

	HRESULT hr = S_OK;
	HANDLE hFile = ::CreateFileW(lpDocumentName, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		pPrinter->Release();

		DWORD dwErr = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwErr);
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
	}
	if (SUCCEEDED(hr))
	{
		pPrinter->m_hFile = hFile;
		*o_ppPrinter = pPrinter;
	}

	return hr;
}

HRESULT __stdcall PrintToFile::Write(_In_reads_bytes_(cb)  const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	if (FAILED(m_hrPrinter))
	{
		return m_hrPrinter;
	}

	if (pv == nullptr)
	{
		m_hrPrinter = E_POINTER;
		return E_POINTER;
	}

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		if (WriteFile(m_hFile, pv, cb, pcbWritten, nullptr) == 0)
		{
			DWORD dwErr = ::GetLastError();
			m_hrPrinter = HRESULT_FROM_WIN32(dwErr);
		}
	}

	return m_hrPrinter;
}

void __stdcall PrintToFile::Cancel()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = E_ABORT;
		}
	}
}

void __stdcall PrintToFile::Close(HRESULT * pPrintResult)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		if (FAILED(m_hrPrinter))
		{
			FILE_DISPOSITION_INFO info = {};
			info.DeleteFile = TRUE;
			(void)SetFileInformationByHandle(m_hFile, FileDispositionInfo, &info, sizeof(info));
		}

		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	if (pPrintResult != nullptr)
	{
		*pPrintResult = m_hrPrinter;
	}
}



/***********************************************************************************************************************************/
PrintToRawPrinter::PrintToRawPrinter() : m_hrPrinter(S_OK), m_hPrinter(INVALID_HANDLE_VALUE), m_bOwnedHandle(false)
{
}

PrintToRawPrinter::~PrintToRawPrinter()
{
	Close(nullptr);
}

HRESULT PrintToRawPrinter::Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if ((lpPrinterName == nullptr) || (lpDocumentName == nullptr) || (o_ppPrinter == nullptr))
	{
		return E_POINTER;
	}

	*o_ppPrinter = nullptr;

	HRESULT hr = S_OK;
	HANDLE hPrinter = INVALID_HANDLE_VALUE;
	if (!OpenPrinter((LPWSTR)lpPrinterName, &hPrinter, nullptr))
	{
		hPrinter = INVALID_HANDLE_VALUE;
		DWORD dwErr = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwErr);
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = Create(hPrinter, lpDocumentName, o_ppPrinter);
	}

	if (SUCCEEDED(hr))
	{
		PrintToRawPrinter* pPrinter = static_cast<PrintToRawPrinter*>(*o_ppPrinter);
		assert(pPrinter != nullptr);
		pPrinter->m_bOwnedHandle = true;
	}
	else if (hPrinter != INVALID_HANDLE_VALUE)
	{
		AbortPrinter(hPrinter);
		ClosePrinter(hPrinter);
	}

	return hr;
}

HRESULT PrintToRawPrinter::Create(HANDLE hPrinter, LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if ((lpDocumentName == nullptr) || (o_ppPrinter == nullptr))
	{
		return E_POINTER;
	}

	if (hPrinter == INVALID_HANDLE_VALUE)
	{
		return E_INVALIDARG;
	}

	*o_ppPrinter = nullptr;

	PrintToRawPrinter* pPrinter = new PrintToRawPrinter();
	if (pPrinter == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = S_OK;

	DOC_INFO_1 info = {};
	wchar_t DataType[] = L"RAW";//L"XPS";L"TEXT"
	info.pDocName = const_cast<LPWSTR>(lpDocumentName);
	info.pOutputFile = nullptr;
	info.pDatatype = DataType;
	if (!StartDocPrinter(hPrinter, 1, (LPBYTE)&info))
	{
		DWORD dwErr = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwErr);
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}

		AbortPrinter(hPrinter);
	}
	if (SUCCEEDED(hr))
	{
		pPrinter->m_hPrinter = hPrinter;
		pPrinter->m_bOwnedHandle = false;
		*o_ppPrinter = pPrinter;
	}
	else
	{
		pPrinter->Release();
	}

	return hr;
}

HRESULT __stdcall PrintToRawPrinter::Write(_In_reads_bytes_(cb)  const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	if (FAILED(m_hrPrinter))
	{
		return m_hrPrinter;
	}

	if (pv == nullptr)
	{
		m_hrPrinter = E_POINTER;
		return E_POINTER;
	}

	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		DWORD dwTmp = 0;
		if (WritePrinter(m_hPrinter, const_cast<void*>(pv), cb, pcbWritten ? pcbWritten : &dwTmp) == FALSE)
		{
			DWORD dwErr = ::GetLastError();
			m_hrPrinter = HRESULT_FROM_WIN32(dwErr);
		}
	}

	return m_hrPrinter;
}

void __stdcall PrintToRawPrinter::Cancel()
{
	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		if (SUCCEEDED(m_hrPrinter))
		{
			AbortPrinter(m_hPrinter);
			m_hrPrinter = E_ABORT;
		}
	}
}

void __stdcall PrintToRawPrinter::Close(HRESULT * pPrintResult)
{
	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		if (SUCCEEDED(m_hrPrinter))
		{
			EndDocPrinter(m_hPrinter);
		}
		else
		{
			AbortPrinter(m_hPrinter);
		}

		if (m_bOwnedHandle)
		{
			ClosePrinter(m_hPrinter);
		}
		m_hPrinter = nullptr;
	}
	m_bOwnedHandle = false;

	if (pPrintResult != nullptr)
	{
		*pPrintResult = m_hrPrinter;
	}
}



/***********************************************************************************************************************************/

#pragma comment(lib, "xpsprint.lib")

PrintToXpsPrintApiPrinter::PrintToXpsPrintApiPrinter() : 
	m_hrPrinter(S_OK), m_hProgressEvent(nullptr), m_hCompletionEvent(nullptr), m_xpsPrintJob(nullptr), m_xpsDocumentStream(nullptr), m_bCoInit(false)
{
}

PrintToXpsPrintApiPrinter::~PrintToXpsPrintApiPrinter()
{
	Close(nullptr);
}

HRESULT PrintToXpsPrintApiPrinter::Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if ((lpPrinterName == nullptr) || (lpDocumentName == nullptr) || (o_ppPrinter == nullptr))
	{
		return E_POINTER;
	}

	*o_ppPrinter = nullptr;

	PrintToXpsPrintApiPrinter* pPrinter = new PrintToXpsPrintApiPrinter();
	if (pPrinter == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		pPrinter->m_bCoInit = true;

		//Manual Reset
		pPrinter->m_hCompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!pPrinter->m_hCompletionEvent)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		UINT8* printablePagesOn = nullptr;
		UINT32 printablePagesOnCount = 0;
		LPCWSTR outputFileName = nullptr;
		IXpsPrintJobStream** ppPrintTicketStream = nullptr;
		//As of Win10SDK 10.0.17763.0, StartXpsPrintJob has wrong annotations (different from documentation). outputFileName and ppPrintTicketStream may be nullptr.
		//C6387: 'outputFileName'/'ppPrintTicketStream' could be '0':  this does not adhere to the specification for the function 'StartXpsPrintJob'.
		//C4995: 'StartXpsPrintJob': name was marked as #pragma deprecated
#pragma warning(push)
#pragma warning(disable: 6387 4995)	
		hr = StartXpsPrintJob(lpPrinterName, lpDocumentName, outputFileName, pPrinter->m_hProgressEvent, pPrinter->m_hCompletionEvent,
			printablePagesOn, printablePagesOnCount, &pPrinter->m_xpsPrintJob, &pPrinter->m_xpsDocumentStream, ppPrintTicketStream);
#pragma warning(pop)
	}

	if (SUCCEEDED(hr))
	{
		*o_ppPrinter = pPrinter;
	}
	else
	{
		//This close the CompletionEvent and calls CoUninit();
		pPrinter->Release();
	}

	return hr;
}

HRESULT __stdcall PrintToXpsPrintApiPrinter::Write(_In_reads_bytes_(cb)  const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	if (FAILED(m_hrPrinter))
	{
		return m_hrPrinter;
	}

	if (pv == nullptr)
	{
		m_hrPrinter = E_POINTER;
		return E_POINTER;
	}

	m_hrPrinter = m_xpsDocumentStream->Write(pv, cb, pcbWritten);

	return m_hrPrinter;
}

void __stdcall PrintToXpsPrintApiPrinter::Cancel()
{
	if (SUCCEEDED(m_hrPrinter) && (m_xpsPrintJob != nullptr))
	{
		m_xpsPrintJob->Cancel();
		m_hrPrinter = E_ABORT;
	}
}

void __stdcall PrintToXpsPrintApiPrinter::Close(HRESULT * pPrintResult)
{
	if (SUCCEEDED(m_hrPrinter) && (m_xpsDocumentStream != nullptr))
	{
		m_hrPrinter = m_xpsDocumentStream->Close();
	}

	if (FAILED(m_hrPrinter) && (m_xpsPrintJob != nullptr))
	{
		m_xpsPrintJob->Cancel();
	}

	if (SUCCEEDED(m_hrPrinter) && (m_hCompletionEvent != nullptr))
	{
		if (WaitForSingleObject(m_hCompletionEvent, INFINITE) != WAIT_OBJECT_0)
		{
			DWORD dwErr = ::GetLastError();
			m_hrPrinter = HRESULT_FROM_WIN32(dwErr);
		}
	}

	if (SUCCEEDED(m_hrPrinter) && (m_xpsPrintJob != nullptr))
	{
		//C4995: 'XPS_JOB_STATUS': name was marked as #pragma deprecated
#pragma warning(push)
#pragma warning(disable: 4995)	
		XPS_JOB_STATUS jobStatus = {};
#pragma warning(pop)

		m_hrPrinter = m_xpsPrintJob->GetJobStatus(&jobStatus);

		if (SUCCEEDED(m_hrPrinter))
		{
			switch (jobStatus.completion)
			{
			case XPS_JOB_COMPLETED:
				break;

			case XPS_JOB_CANCELLED:
				m_hrPrinter = E_ABORT;
				break;

			case XPS_JOB_FAILED:
				m_hrPrinter = E_FAIL;
				break;

			default:
				m_hrPrinter = E_UNEXPECTED;
				break;
			}
		}
	}

	SAFE_RELEASE(m_xpsPrintJob);
	SAFE_RELEASE(m_xpsDocumentStream);

	if (m_hProgressEvent != nullptr)
	{
		CloseHandle(m_hProgressEvent);
		m_hProgressEvent = nullptr;
	}

	if (m_hCompletionEvent != nullptr)
	{
		CloseHandle(m_hCompletionEvent);
		m_hCompletionEvent = nullptr;
	}

	if (m_bCoInit)
	{
		CoUninitialize();
		m_bCoInit = false;
	}

	if (pPrintResult != nullptr)
	{
		*pPrintResult = m_hrPrinter;
	}
}

/***********************************************************************************************************************************/

struct XpsConfig
{
	bool m_bIgnoreXpsSmallElements;
	bool m_bIgnoreXpsSmallAbsoluteElements;
};

XpsConfig s_xpsConfig = { true, false };

void PrintToXpsPrintDocumentPrinter::Config(const bool i_bIgnoreXpsSmallElements, const bool i_bIgnoreXpsSmallAbsoluteElements)
{
	s_xpsConfig.m_bIgnoreXpsSmallElements = i_bIgnoreXpsSmallElements;
	s_xpsConfig.m_bIgnoreXpsSmallAbsoluteElements = i_bIgnoreXpsSmallAbsoluteElements;
}

PrintToXpsPrintDocumentPrinter::PrintToXpsPrintDocumentPrinter() : m_hrPrinter(S_OK), m_xpsDocPackageTarget(nullptr), m_xpsJobStream(nullptr), m_bCoInit(false)
{
}

PrintToXpsPrintDocumentPrinter::~PrintToXpsPrintDocumentPrinter()
{
	Close(nullptr);
}

HRESULT PrintToXpsPrintDocumentPrinter::Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if ((lpPrinterName == nullptr) || (lpDocumentName == nullptr) || (o_ppPrinter == nullptr))
	{
		return E_POINTER;
	}

	*o_ppPrinter = nullptr;

	PrintToXpsPrintDocumentPrinter* pPrinter = new PrintToXpsPrintDocumentPrinter();
	if (pPrinter == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	IPrintDocumentPackageTargetFactory* documentTargetFactory = nullptr;
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		pPrinter->m_bCoInit = true;

		// Create a factory for document print job.
		hr = ::CoCreateInstance(
				__uuidof(PrintDocumentPackageTargetFactory),
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&documentTargetFactory)
		);
	}

	if (SUCCEEDED(hr))
	{
		// Allocate stream.
		hr = CreateStreamOnHGlobal(nullptr, TRUE, &pPrinter->m_xpsJobStream);
	}

	// Initialize the print subsystem and get a package target.
	if (SUCCEEDED(hr))
	{
		hr = documentTargetFactory->CreateDocumentPackageTargetForPrintJob(
			lpPrinterName,                      // printer name
			lpDocumentName,						// job name
			nullptr,         // job output stream; when nullptr, send to printer
			nullptr,    // job print ticket
			&pPrinter->m_xpsDocPackageTarget    // result IPrintDocumentPackageTarget object
		);
	}

	SAFE_RELEASE(documentTargetFactory);

	if (SUCCEEDED(hr))
	{
		*o_ppPrinter = pPrinter;
	}
	else
	{
		//This releases m_xpsJobStream and m_xpsDocPackageTarget, then it calls CoUninit();
		pPrinter->Release();
	}

	return hr;
}

HRESULT __stdcall PrintToXpsPrintDocumentPrinter::Write(_In_reads_bytes_(cb)  const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	if (FAILED(m_hrPrinter))
	{
		return m_hrPrinter;
	}

	if (pv == nullptr)
	{
		m_hrPrinter = E_POINTER;
		return E_POINTER;
	}

	m_hrPrinter = m_xpsJobStream->Write(pv, cb, pcbWritten);

	return m_hrPrinter;
}

void __stdcall PrintToXpsPrintDocumentPrinter::Cancel()
{
	if (SUCCEEDED(m_hrPrinter) && (m_xpsDocPackageTarget != nullptr))
	{
		m_xpsDocPackageTarget->Cancel();
		m_hrPrinter = E_ABORT;
	}
}

HRESULT ParseVisualCollection(IXpsOMVisualCollection* pIXpsOMVisualCollection, IXpsOMDictionary* pIXpsOMDictionary, const XPS_MATRIX& ParentMatrix);

void __stdcall PrintToXpsPrintDocumentPrinter::Close(HRESULT * pPrintResult)
{
	if (SUCCEEDED(m_hrPrinter) && (m_xpsJobStream != nullptr) && (m_xpsDocPackageTarget != nullptr))
	{
		m_hrPrinter = m_xpsJobStream->Seek(LARGE_INTEGER{}, 0, nullptr);

		XpsPrintJobChecker* pXpsPrintJobChecker = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = XpsPrintJobChecker::Create(&pXpsPrintJobChecker);
		}

		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = pXpsPrintJobChecker->Initialize(m_xpsDocPackageTarget);
		}

		UINT32 NumTargets = 0;
		GUID* TargetGuids = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = m_xpsDocPackageTarget->GetPackageTargetTypes(&NumTargets, &TargetGuids);
		}

		IXpsDocumentPackageTarget* pTarget = nullptr;
		if (SUCCEEDED(m_hrPrinter) && (NumTargets > 0))
		{
			m_hrPrinter = m_xpsDocPackageTarget->GetPackageTarget(TargetGuids[0], __uuidof(IXpsDocumentPackageTarget), (void**)&pTarget);
		}

		if (TargetGuids != nullptr)
		{
			CoTaskMemFree(TargetGuids);
			TargetGuids = nullptr;
		}

		IXpsOMObjectFactory* xpsFactory = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = pTarget->GetXpsOMFactory(&xpsFactory);
		}

		//IXpsOMObjectFactory1* xpsFactory1 = nullptr;
		//if (SUCCEEDED(m_hrPrinter))
		//{
		//	xpsFactory->QueryInterface(&xpsFactory1);
		//}

		IXpsOMPackage* package = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			//if (xpsFactory1)
			//{
			//	m_hrPrinter = xpsFactory1->CreatePackageFromStream1(m_xpsJobStream, FALSE, (IXpsOMPackage1**)&package);
			//}
			//else
			//{
				m_hrPrinter = xpsFactory->CreatePackageFromStream(m_xpsJobStream, FALSE, &package);
			//}
		}

		SAFE_RELEASE(xpsFactory);
		//SAFE_RELEASE(xpsFactory1);

		IXpsOMDocumentSequence* documentSequence = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = package->GetDocumentSequence(&documentSequence);
		}

		IOpcPartUri* docSeqUri = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = documentSequence->GetPartName(&docSeqUri);
		}

		IOpcPartUri* discPartUri = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = package->GetDiscardControlPartName(&discPartUri);
		}

		SAFE_RELEASE(package);

		IXpsOMPackageWriter* pPackageWriter = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = pTarget->GetXpsOMPackageWriter(docSeqUri, discPartUri, &pPackageWriter);
		}

		SAFE_RELEASE(discPartUri);
		SAFE_RELEASE(docSeqUri);

		IXpsOMDocumentCollection* pDocColl = nullptr;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = documentSequence->GetDocuments(&pDocColl);
		}

		SAFE_RELEASE(documentSequence);

		UINT32 docCount = 0;
		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = pDocColl->GetCount(&docCount);
		}

		if (SUCCEEDED(m_hrPrinter))
		{
			for (UINT32 i = 0; SUCCEEDED(m_hrPrinter) && (i < docCount); i++)
			{
				IXpsOMDocument* pDoc = nullptr;
				m_hrPrinter = pDocColl->GetAt(i, &pDoc);

				IOpcPartUri *documentPartName = nullptr;
				IXpsOMPrintTicketResource *documentPrintTicket = nullptr;
				IXpsOMDocumentStructureResource *documentStructure = nullptr;
				IXpsOMSignatureBlockResourceCollection *signatureBlockResources = nullptr;
				IXpsOMPartUriCollection *restrictedFonts = nullptr;
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pDoc->GetPartName(&documentPartName);
				}

				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pDoc->GetPrintTicketResource(&documentPrintTicket);
				}

				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pDoc->GetDocumentStructureResource(&documentStructure);
				}

				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pDoc->GetSignatureBlockResources(&signatureBlockResources);
				}
				//restrictedFonts?

				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pPackageWriter->StartNewDocument(documentPartName, documentPrintTicket, documentStructure, signatureBlockResources, restrictedFonts);
				}

				SAFE_RELEASE(documentPartName);
				SAFE_RELEASE(documentPrintTicket);
				SAFE_RELEASE(documentStructure);
				SAFE_RELEASE(signatureBlockResources);
				SAFE_RELEASE(restrictedFonts);

				IXpsOMPageReferenceCollection* pageReferences = nullptr;
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pDoc->GetPageReferences(&pageReferences);
				}

				SAFE_RELEASE(pDoc);

				UINT32 pageCount = 0;
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = pageReferences->GetCount(&pageCount);
				}

				if (SUCCEEDED(m_hrPrinter))
				{
					for (UINT32 p = 0; SUCCEEDED(m_hrPrinter) && (p < pageCount); p++)
					{
						IXpsOMPageReference* pPageRef = nullptr;
						m_hrPrinter = pageReferences->GetAt(p, &pPageRef);

						IXpsOMPage *page = nullptr;
						XPS_SIZE advisoryPageDimensions = {};
						IXpsOMPartUriCollection *discardableResourceParts = nullptr;
						IXpsOMStoryFragmentsResource *storyFragments = nullptr;
						IXpsOMPrintTicketResource *pagePrintTicket = nullptr;
						IXpsOMImageResource *pageThumbnail = nullptr;
						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPageRef->GetPage(&page);
						}

						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPageRef->GetPrintTicketResource(&pagePrintTicket);
						}

						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPageRef->GetStoryFragmentsResource(&storyFragments);
						}

						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPageRef->GetThumbnailResource(&pageThumbnail);
						}

						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPageRef->GetAdvisoryPageDimensions(&advisoryPageDimensions);
						}
						//discardableResourceParts?

						SAFE_RELEASE(pPageRef);

						if (SUCCEEDED(m_hrPrinter))
						{
							IXpsOMDictionary* pIXpsOMDictionary = nullptr;
							IXpsOMVisualCollection* pIXpsOMVisualCollection = nullptr;
							HRESULT hh = page->GetVisuals(&pIXpsOMVisualCollection);
							if (SUCCEEDED(hh))
							{
								hh = page->GetDictionary(&pIXpsOMDictionary);
							}
							if (SUCCEEDED(hh))
							{
								//Remove Visuals that are too small for being rendered (not-invertible matrices, according to oxps standard).
								//TODO: compensate such matrices by scaling referenced paths and geometries if possible.
								hh = ParseVisualCollection(pIXpsOMVisualCollection, pIXpsOMDictionary, { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f });
							}
							
							SAFE_RELEASE(pIXpsOMVisualCollection);
							SAFE_RELEASE(pIXpsOMDictionary);
						}

						if (SUCCEEDED(m_hrPrinter))
						{
							m_hrPrinter = pPackageWriter->AddPage(page, &advisoryPageDimensions, discardableResourceParts, storyFragments, pagePrintTicket, pageThumbnail);
						}

						SAFE_RELEASE(page);
						SAFE_RELEASE(discardableResourceParts);
						SAFE_RELEASE(storyFragments);
						SAFE_RELEASE(pagePrintTicket);
						SAFE_RELEASE(pageThumbnail);
					}
				}

				SAFE_RELEASE(pageReferences);
			}
		}

		SAFE_RELEASE(pDocColl);

		if (SUCCEEDED(m_hrPrinter))
		{
			m_hrPrinter = pPackageWriter->Close();
		}

		if (FAILED(m_hrPrinter))
		{
			m_xpsDocPackageTarget->Cancel();
		}

		SAFE_RELEASE(pTarget);
		SAFE_RELEASE(pPackageWriter);

		if (pXpsPrintJobChecker != nullptr)
		{
			pXpsPrintJobChecker->WaitForCompletion();

			PrintDocumentPackageStatus jobStatus = pXpsPrintJobChecker->GetStatus();
			switch (jobStatus.Completion)
			{
			case PrintDocumentPackageCompletion_Completed:
				break;

			case PrintDocumentPackageCompletion_Canceled:
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = E_ABORT;
				}
				break;

			case PrintDocumentPackageCompletion_Failed:
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = E_FAIL;
				}
				break;

			default:
				if (SUCCEEDED(m_hrPrinter))
				{
					m_hrPrinter = E_UNEXPECTED;
				}
				break;
			}
		}

		SAFE_RELEASE(pXpsPrintJobChecker);
	}

	SAFE_RELEASE(m_xpsDocPackageTarget);
	SAFE_RELEASE(m_xpsJobStream);

	if (m_bCoInit)
	{
		CoUninitialize();
		m_bCoInit = false;
	}

	if (pPrintResult != nullptr)
	{
		*pPrintResult = m_hrPrinter;
	}
}

float MatrixDeterminant(const XPS_MATRIX& Matrix)
{
	return (Matrix.m11 * Matrix.m22 - Matrix.m12 * Matrix.m21);
}

bool IsMatrixDeterminantTooSmall(const XPS_MATRIX& Matrix)
{
	return MatrixDeterminant(Matrix) < (1.2e-7f);
}

XPS_MATRIX MultMatrix(const XPS_MATRIX& Parent, const XPS_MATRIX& Child)
{
	XPS_MATRIX Absolute = {
		Parent.m11 * Child.m11 + Parent.m12 * Child.m21,
		Parent.m11 * Child.m12 + Parent.m12 * Child.m22,
		Parent.m21 * Child.m11 + Parent.m22 * Child.m21,
		Parent.m21 * Child.m12 + Parent.m22 * Child.m22,
		Parent.m31 * Child.m11 + Parent.m32 * Child.m21 + Child.m31,
		Parent.m31 * Child.m12 + Parent.m32 * Child.m22 + Child.m32
	};
	
	return Absolute;
}

HRESULT ParseVisual(IXpsOMVisual* pIXpsOMVisual, IXpsOMDictionary* pIXpsOMDictionary, const XPS_MATRIX& ParentMatrix, bool& o_bTooSmall)
{
	XPS_MATRIX matrix = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
	XPS_MATRIX matrixLocal = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
	XPS_MATRIX matrixShared = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
	IXpsOMMatrixTransform* pIXpsOMMatrixTransform = nullptr;
	IXpsOMMatrixTransform* pIXpsOMMatrixTransformLocal = nullptr;
	IXpsOMMatrixTransform* pIXpsOMMatrixTransformShared = nullptr;
	XPS_OBJECT_TYPE xpsType = {};

	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
		hr = pIXpsOMVisual->GetType(&xpsType);
	}

	//GetTransform returns GetTransformLocal or GetTransformLookup, according to IXpsOMVisual configuration.
	//It should be composed with parent matrix.

	if (SUCCEEDED(hr))
	{
		hr = pIXpsOMVisual->GetTransform(&pIXpsOMMatrixTransform);
	}

	if (SUCCEEDED(hr))
	{
		hr = pIXpsOMVisual->GetTransformLocal(&pIXpsOMMatrixTransformLocal);
	}

	if (SUCCEEDED(hr) && pIXpsOMDictionary)
	{
		LPWSTR lookupTransform = nullptr;
		hr = pIXpsOMVisual->GetTransformLookup(&lookupTransform);

		if (SUCCEEDED(hr) && lookupTransform)
		{
			IXpsOMShareable* pLookupEntry = nullptr;
			hr = pIXpsOMDictionary->GetByKey(lookupTransform, nullptr, &pLookupEntry);
			if (SUCCEEDED(hr))
			{
				hr = pLookupEntry->QueryInterface(&pIXpsOMMatrixTransformShared);
			}
			SAFE_RELEASE(pLookupEntry);
		}

		if (lookupTransform)
		{
			CoTaskMemFree(lookupTransform);
			lookupTransform = nullptr;
		}
	}

	if (SUCCEEDED(hr) && pIXpsOMMatrixTransform)
	{
		hr = pIXpsOMMatrixTransform->GetMatrix(&matrix);
	}

	if (SUCCEEDED(hr) && pIXpsOMMatrixTransformLocal)
	{
		hr = pIXpsOMMatrixTransformLocal->GetMatrix(&matrixLocal);
	}

	if (SUCCEEDED(hr) && pIXpsOMMatrixTransformShared)
	{
		hr = pIXpsOMMatrixTransformShared->GetMatrix(&matrixShared);
	}

/*
	if (IsMatrixDeterminantTooSmall(matrix))
	{
		DBGLOG("Transform: %f, %f, %f, %f, %f, %f", matrix.m11, matrix.m12, matrix.m21, matrix.m22, matrix.m31, matrix.m32);
	}
	if (IsMatrixDeterminantTooSmall(matrixLocal))
	{
		DBGLOG("TransformLocal: %f, %f, %f, %f, %f, %f", matrixLocal.m11, matrixLocal.m12, matrixLocal.m21, matrixLocal.m22, matrixLocal.m31, matrixLocal.m32);
	}
	if (IsMatrixDeterminantTooSmall(matrixShared))
	{
		DBGLOG("TransformShared: %f, %f, %f, %f, %f, %f", matrixShared.m11, matrixShared.m12, matrixShared.m21, matrixShared.m22, matrixShared.m31, matrixShared.m32);
	}
*/

	SAFE_RELEASE(pIXpsOMMatrixTransformShared);
	SAFE_RELEASE(pIXpsOMMatrixTransformLocal);
	SAFE_RELEASE(pIXpsOMMatrixTransform);

	XPS_MATRIX AbsoluteMatrix = MultMatrix(ParentMatrix, matrix);

	//Remove Visuals that are too small for being rendered (not-invertible matrices, according to oxps standard).
	//TODO: compensate such matrices by scaling referenced paths and geometries if possible.
	o_bTooSmall = false;
	if (s_xpsConfig.m_bIgnoreXpsSmallElements && IsMatrixDeterminantTooSmall(matrix))
	{
		//IsMatrixDeterminantTooSmall(matrixShared) || IsMatrixDeterminantTooSmall(matrixLocal)
		o_bTooSmall = true;
	}
	else if (s_xpsConfig.m_bIgnoreXpsSmallAbsoluteElements && IsMatrixDeterminantTooSmall(AbsoluteMatrix))
	{
		o_bTooSmall = true;
	}

	if (SUCCEEDED(hr))
	{
		switch (xpsType)
		{
		case XPS_OBJECT_TYPE_CANVAS:
		{
			IXpsOMCanvas* pIXpsOMCanvas = nullptr;
			IXpsOMVisualCollection* pIXpsOMVisualCollection = nullptr;

			hr = pIXpsOMVisual->QueryInterface(&pIXpsOMCanvas);

			if (SUCCEEDED(hr))
			{
				hr = pIXpsOMCanvas->GetVisuals(&pIXpsOMVisualCollection);
			}

			if (SUCCEEDED(hr))
			{
				//Remove Visuals that are too small for being rendered (not-invertible matrices, according to oxps standard).
				//TODO: compensate such matrices by scaling referenced paths and geometries if possible.
				hr = ParseVisualCollection(pIXpsOMVisualCollection, pIXpsOMDictionary, AbsoluteMatrix);
			}

			SAFE_RELEASE(pIXpsOMVisualCollection);
			SAFE_RELEASE(pIXpsOMCanvas);
		}
		break;

		case XPS_OBJECT_TYPE_GLYPHS:
		{
			IXpsOMGlyphs* pIXpsOMGlyphs = nullptr;
			hr = pIXpsOMVisual->QueryInterface(&pIXpsOMGlyphs);
			if (SUCCEEDED(hr))
			{
			}
			SAFE_RELEASE(pIXpsOMGlyphs);
		}
		break;

		case XPS_OBJECT_TYPE_PATH:
		{
			IXpsOMPath* pIXpsOMPath = nullptr;
			hr = pIXpsOMVisual->QueryInterface(&pIXpsOMPath);
			if (SUCCEEDED(hr))
			{
			}
			SAFE_RELEASE(pIXpsOMPath);
		}
		break;

		case XPS_OBJECT_TYPE_MATRIX_TRANSFORM:
			break;

		case XPS_OBJECT_TYPE_GEOMETRY:
			break;

		case XPS_OBJECT_TYPE_SOLID_COLOR_BRUSH:
			break;

		case XPS_OBJECT_TYPE_IMAGE_BRUSH:
			break;

		case XPS_OBJECT_TYPE_LINEAR_GRADIENT_BRUSH:
			break;

		case XPS_OBJECT_TYPE_RADIAL_GRADIENT_BRUSH:
			break;

		case XPS_OBJECT_TYPE_VISUAL_BRUSH:
			break;
		}
	}

	return hr;
}

HRESULT ParseVisualCollection(IXpsOMVisualCollection* pIXpsOMVisualCollection, IXpsOMDictionary* pIXpsOMDictionary, const XPS_MATRIX& ParentMatrix)
{
	HRESULT hr = S_OK;
	if (pIXpsOMVisualCollection)
	{
		uint32_t Count = 0;
		hr = pIXpsOMVisualCollection->GetCount(&Count);

		if (SUCCEEDED(hr))
		{
			for (uint32_t v = 0; SUCCEEDED(hr) && (v < Count); v++)
			{
				IXpsOMVisual* pIXpsOMVisual = nullptr;
				hr = pIXpsOMVisualCollection->GetAt(v, &pIXpsOMVisual);

				bool bTooSmall = false;
				if (SUCCEEDED(hr))
				{
					hr = ParseVisual(pIXpsOMVisual, pIXpsOMDictionary, ParentMatrix, bTooSmall);
				}

				if (SUCCEEDED(hr) && bTooSmall)
				{
					//Remove Visuals that are too small for being rendered (not-invertible matrices, according to oxps standard).
					//TODO: compensate such matrices by scaling referenced paths and geometries if possible.
					if (SUCCEEDED(pIXpsOMVisualCollection->RemoveAt(v)))
					{
						v--;
						Count--;
					}
				}

				SAFE_RELEASE(pIXpsOMVisual);
			}
		}
	}
	return hr;
}
