#include "pch.h"

//No more needed. See PrinterSourceStream.
#if 0

#include "WSDAttachments.h"
#include "WSDPrinterService.h"
#include "WSDUtils.h"
#include "PrinterInterface.h"
#include "PrintDocumentPdf.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
// CInMemoryWSDInboundAttachment Class
//       
//////////////////////////////////////////////////////////////////////////////
CInMemoryWSDInboundAttachment* CInMemoryWSDInboundAttachment::Create(const BYTE* i_pBuffer, const DWORD i_dwBufferSize, const bool i_bOwnData)
{
	return new CInMemoryWSDInboundAttachment(i_pBuffer, i_dwBufferSize, i_bOwnData);
}

CInMemoryWSDInboundAttachment* CInMemoryWSDInboundAttachment::Create(const std::vector<uint8_t>& i_Buffer, const bool i_bOwnData)
{
	return new CInMemoryWSDInboundAttachment(i_Buffer, i_bOwnData);
}

CInMemoryWSDInboundAttachment* CInMemoryWSDInboundAttachment::Create(std::vector<uint8_t>&& i_Buffer)
{
	return new CInMemoryWSDInboundAttachment(std::move(i_Buffer));
}

CInMemoryWSDInboundAttachment::CInMemoryWSDInboundAttachment(const BYTE* i_pBuffer, const DWORD i_dwBufferSize, const bool i_bOwnData) : m_pBuffer(i_pBuffer), m_dwBufferSize(i_dwBufferSize), m_dwReadPos(0)
{
	if (i_bOwnData)
	{
		m_buffer.assign(i_pBuffer, i_pBuffer + i_dwBufferSize);
		m_pBuffer = m_buffer.data();
	}
}

CInMemoryWSDInboundAttachment::CInMemoryWSDInboundAttachment(const std::vector<uint8_t>& i_Buffer, const bool i_bOwnData) : m_pBuffer(i_Buffer.data()), m_dwBufferSize(static_cast<DWORD>(i_Buffer.size())), m_dwReadPos(0)
{
	if (i_bOwnData)
	{
		m_buffer = i_Buffer;
		m_pBuffer = m_buffer.data();
	}
}

CInMemoryWSDInboundAttachment::CInMemoryWSDInboundAttachment(std::vector<uint8_t>&& i_Buffer) : m_pBuffer(nullptr), m_dwBufferSize(static_cast<DWORD>(i_Buffer.size())), m_dwReadPos(0)
{
	m_buffer = std::move(i_Buffer);
	m_pBuffer = m_buffer.data();
}

CInMemoryWSDInboundAttachment::~CInMemoryWSDInboundAttachment()
{
}

HRESULT CInMemoryWSDInboundAttachment::Read(BYTE *pBuffer, DWORD dwBytesToRead, LPDWORD pdwNumberOfBytesRead)
{
	if (pBuffer == nullptr)
	{
		return E_INVALIDARG;
	}

	if (m_pBuffer == nullptr)
	{
		return E_POINTER;
	}

	DWORD dwReadBytes = std::min(dwBytesToRead, m_dwBufferSize - m_dwReadPos);
	memcpy(pBuffer, &m_pBuffer[m_dwReadPos], dwReadBytes);
	m_dwReadPos += dwReadBytes;
	if (pdwNumberOfBytesRead)
	{
		*pdwNumberOfBytesRead = dwReadBytes;
	}
	return (m_dwReadPos == m_dwBufferSize) ? S_FALSE : S_OK;
}

HRESULT CInMemoryWSDInboundAttachment::Close()
{
	m_pBuffer = nullptr;
	m_dwBufferSize = 0;
	m_dwReadPos = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CSendAttachmentThread Class
//       
//////////////////////////////////////////////////////////////////////////////
CSendAttachmentThread::CSendAttachmentThread() : BufferSize(0), Buffer(nullptr), m_pAttachment(nullptr)
{
}

CSendAttachmentThread::~CSendAttachmentThread()
{
	SAFE_RELEASE(m_pAttachment);
	delete Buffer;
}

IWSDOutboundAttachment* CSendAttachmentThread::SendAsync(const BYTE* lpByte, const DWORD dwSize)
{
	HRESULT hr = S_OK;

	FileName.clear();

	IWSDOutboundAttachment* pAttachment = nullptr;
	hr = WSDCreateOutboundAttachment(&pAttachment);
	if (SUCCEEDED(hr))
	{
		Buffer = new BYTE[dwSize];
		if (!Buffer)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			memcpy(Buffer, lpByte, dwSize);
			BufferSize = dwSize;
		}
	}

	m_pAttachment = pAttachment;	//Owned by this class
	if (pAttachment != nullptr)
	{
		pAttachment->AddRef();		//Owned by caller
	}

	if (SUCCEEDED(hr))
	{
		hr = Start();
	}

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pAttachment);
		SAFE_RELEASE(pAttachment);
	}

	return pAttachment;
}

IWSDOutboundAttachment* CSendAttachmentThread::SendAsync(const wchar_t* lpFileName)
{
	if (!lpFileName)
	{
		return nullptr;
	}

	FileName = lpFileName;

	HRESULT hr = S_OK;

	IWSDOutboundAttachment* pAttachment = nullptr;
	hr = WSDCreateOutboundAttachment(&pAttachment);
	if (SUCCEEDED(hr))
	{
		DWORD dwSize = 1024 * 1024;
		Buffer = new BYTE[dwSize];
		if (!Buffer)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			BufferSize = dwSize;
		}
	}

	m_pAttachment = pAttachment;	//Owned by this class
	if (pAttachment != nullptr)
	{
		pAttachment->AddRef();		//Owned by caller
	}

	if (SUCCEEDED(hr))
	{
		hr = Start();
	}

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pAttachment);
		SAFE_RELEASE(pAttachment);
	}

	return pAttachment;
}

/////////////////////////////////////////////////////////////////////////////
// Start - Starts the send-attachment thread via thread-pool thread
//////////////////////////////////////////////////////////////////////////////
HRESULT CSendAttachmentThread::Start()
{
	DWORD dwErr = 0;

	//
	// Immediately start a new thread to send the attachment
	//
	// Use QueueUserWorkItem here because the attachment threads are created
	// often and die quickly.  Also, the threadpool cap will help keep from
	// spawning too many threads.
	//
	AddRef();	//It will be release in ThreadProc
	if (0 == ::QueueUserWorkItem(StaticThreadProc, (LPVOID*)this, WT_EXECUTELONGFUNCTION))
	{
		Release();
		dwErr = ::GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}

	return S_OK;
}

DWORD WINAPI CSendAttachmentThread::StaticThreadProc(LPVOID pParams)
{
	if (nullptr != pParams)
	{
		// Ignore result
		(void)((CSendAttachmentThread*)pParams)->ThreadProc();
	}
	return 0;
}

HRESULT CSendAttachmentThread::ThreadProc()
{
	HRESULT hr = S_OK;

	if (FileName.length())
	{
		DBGLOG("Sending file %ls\n", FileName.c_str());

		DWORD dwErr = 0;
		DWORD dwBytesRead = 0;
		HANDLE hFile = ::CreateFileW(FileName.c_str(), FILE_READ_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			hFile = nullptr;
			dwErr = ::GetLastError();
			hr = HRESULT_FROM_WIN32(dwErr);
		}

		// Loop through data in file until finished
		while (S_OK == hr)
		{
			// Read a block from the file
			if (0 == ::ReadFile(hFile, Buffer, BufferSize, &dwBytesRead, 0))
			{
				dwErr = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwErr);
			}

			DWORD dwBytesLeft = dwBytesRead;

			if (0 == dwBytesLeft)
			{
				// No data left--time to bail out
				DBGLOG(" done.\r\n");
				break;
			}

			while (S_OK == hr && 0 < dwBytesLeft)
			{
				DWORD dwBytesWritten = 0;

				// Write multiple times until this block has been consumed
				hr = m_pAttachment->Write(Buffer + (dwBytesRead - dwBytesLeft), dwBytesLeft, &dwBytesWritten);

				dwBytesLeft -= dwBytesWritten;
			}
		}

		// cleanup
		if (nullptr != hFile)
		{
			::CloseHandle(hFile);
			hFile = nullptr;
		}

		if (S_OK == hr)
		{
			hr = m_pAttachment->Close();
		}
		else
		{
			m_pAttachment->Abort();
		}
	}
	else
	{
		DWORD dwBytesWritten = 0;
		hr = m_pAttachment->Write(Buffer, BufferSize, &dwBytesWritten);
		if (SUCCEEDED(hr))
		{
			hr = m_pAttachment->Close();
		}
		else
		{
			m_pAttachment->Abort();
		}
	}

	Release();

	return hr;
}




//////////////////////////////////////////////////////////////////////////////
// CReadAttachmentThread Class
//       Performs worker thread reads an attachment from the client
//////////////////////////////////////////////////////////////////////////////
CReceiveAttachmentThread::CReceiveAttachmentThread() : m_pAttachment(nullptr) /*, Buffer(nullptr), BufferSize(0)*/
, JobId(0), m_hPrinter(INVALID_HANDLE_VALUE)
{
}

CReceiveAttachmentThread::~CReceiveAttachmentThread()
{
	if (m_pAttachment)
	{
	//	m_pAttachment->Release();
	}
	SAFE_RELEASE(m_pAttachment);
	//delete Buffer;
}

HRESULT CReceiveAttachmentThread::ReceiveAsyncToPrinter(const wchar_t * lpPrinterName, LONG i_JobId, const wchar_t * i_DocumentName, const wchar_t* i_DocumentFormat, IWSDAttachment * pAttachment, std::function<ProgressFunc> InFunc)
{
	HRESULT hr = S_OK;

	if (NULL == pAttachment)
	{
		return E_INVALIDARG;
	}

	if (NULL == lpPrinterName)
	{
		return E_INVALIDARG;
	}

	FileName.clear();
	PrinterName = lpPrinterName;
	m_hPrinter = INVALID_HANDLE_VALUE;

	if (i_DocumentFormat && (wcscmp(i_DocumentFormat, L"RAW") == 0))
	{
		OpenPrinter2(lpPrinterName, &m_hPrinter, nullptr, nullptr);
		PrinterName.clear();
	}
	
	return ReceiveAsyncInternal(i_JobId, i_DocumentName, i_DocumentFormat, pAttachment, InFunc);
}

HRESULT CReceiveAttachmentThread::ReceiveAsyncToPrinter(HANDLE i_hPrinter, LONG i_JobId, const wchar_t* i_DocumentName, const wchar_t* i_DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc)
{
	HRESULT hr = S_OK;

	if (NULL == pAttachment)
	{
		return E_INVALIDARG;
	}

	FileName.clear();
	PrinterName.clear();
	m_hPrinter = i_hPrinter;

	return ReceiveAsyncInternal(i_JobId, i_DocumentName, i_DocumentFormat, pAttachment, InFunc);
}

HRESULT CReceiveAttachmentThread::ReceiveAsyncToFile(const wchar_t* lpFileName, LONG i_JobId, const wchar_t* i_DocumentName, const wchar_t* i_DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc)
{
	if (!lpFileName)
	{
		return E_INVALIDARG;
	}

	FileName = lpFileName;
	PrinterName.clear();
	m_hPrinter = INVALID_HANDLE_VALUE;

	return ReceiveAsyncInternal(i_JobId, i_DocumentName, i_DocumentFormat, pAttachment, InFunc);
}


HRESULT CReceiveAttachmentThread::ReceiveAsyncToAutoFileName(LONG i_JobId, const wchar_t* i_DocumentName, const wchar_t* i_DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc)
{
	wchar_t szFileName[MAX_PATH];
	const wchar_t* pszExtension = L"dat";
	if (i_DocumentFormat)
	{
		if (wcscmp(i_DocumentFormat, L"application/pdf") == 0)
		{
			pszExtension = L"pdf";
		}
		else if ((wcscmp(i_DocumentFormat, DocumentFormatWKVType_application_vnd_ms_xpsdocument) == 0) ||
			(wcscmp(i_DocumentFormat, L"application/oxps") == 0))
		{
			pszExtension = L"xps";
		}
	}

	SYSTEMTIME st = {};
	GetLocalTime(&st);
	swprintf_s(szFileName, L"Print_%d_%02d_%02d_%02d_%02d_%02d_%03d.%ls",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds,
		pszExtension);

	return ReceiveAsyncToFile(szFileName, i_JobId, i_DocumentName, i_DocumentFormat, pAttachment, InFunc);
}

HRESULT CReceiveAttachmentThread::ReceiveAsyncInternal(LONG i_JobId, const wchar_t * i_DocumentName, const wchar_t* i_DocumentFormat, IWSDAttachment * pAttachment, std::function<ProgressFunc> InFunc)
{
	JobId = i_JobId;
	DocumentName = i_DocumentName ? i_DocumentName : L"";
	DocumentFormat = i_DocumentFormat ? i_DocumentFormat : L"";
	Progress = InFunc;

	m_pAttachment = nullptr;
	//HRESULT hr = pAttachment->QueryInterface(__uuidof(IWSDInboundAttachment), (void**)&m_pAttachment);
	HRESULT hr = S_OK;
	m_pAttachment = pAttachment;

	if (m_pAttachment)
	{
		m_pAttachment->AddRef();
	}

	if (SUCCEEDED(hr))
	{
		hr = Start();
	}

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pAttachment)
	}

	return hr;
}


HRESULT CReceiveAttachmentThread::Start()
{
	//
	// Immediately start a new thread to send the attachment
	//
	// Use QueueUserWorkItem here because the attachment threads are created
	// often and die quickly.  Also, the threadpool cap will help keep from
	// spawning too many threads.
	//
#if 1
	//If this function runs in the caller thread, before returning from SendDocument,
	//the file to be printed is correctly received.
	AddRef();
	return StaticThreadProc(this);
#else

	AddRef();
	if (0 == ::QueueUserWorkItem(StaticThreadProc, (LPVOID*)this, WT_EXECUTELONGFUNCTION))
	{
		Release();
		DWORD dwErr = ::GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}

	return S_OK;
#endif
}

DWORD WINAPI CReceiveAttachmentThread::StaticThreadProc(LPVOID pParams)
{
	if (NULL != pParams)
	{
		// Ignore result
		(void)((CReceiveAttachmentThread*)pParams)->ThreadProc();
	}
	return 0;
}


HRESULT CReceiveAttachmentThread::ThreadProc()
{
	DWORD dwBytesWritten = 0;

	DWORD dwTotalRead = 0;
	DWORD dwBytesRead = 0;
	DWORD dwBytesLeft = 0;
	DWORD dwWritten = 0;
	DWORD dwErr = 0;

	HRESULT hr = S_OK;

	IWSDInboundAttachment* pStream = nullptr;
	hr = m_pAttachment->QueryInterface(__uuidof(IWSDInboundAttachment), (void**)&pStream);
	if (FAILED(hr))
	{
		Progress(JobId, JobStateWKVType_Completed, JobStateReasonsWKVType_JobCompletedWithErrors, 0);
		Release();
		return hr;
	}

	Progress(JobId, JobStateWKVType_Started, JobStateReasonsWKVType_None, dwTotalRead);
	//Progress(JobId, JobStateWKVType_Pending, JobStateReasonsWKVType_JobIncoming, dwTotalRead);

	BasePrinter* pPrinter = nullptr;
	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		hr = PrintToRawPrinter::Create(m_hPrinter, DocumentName.c_str(), &pPrinter);
	}
	else if (PrinterName.size() > 0)
	{
		if (DocumentFormat.compare(L"application/pdf")==0)
		{
			hr = PrintPdf::Create(PrinterName.c_str(), DocumentName.c_str(), &pPrinter);
		}
		else
		{
			//hr = PrintToXpsPrintApiPrinter::Create(PrinterName.c_str(), DocumentName.c_str(), &pPrinter);
			hr = PrintToXpsPrintDocumentPrinter::Create(PrinterName.c_str(), DocumentName.c_str(), &pPrinter);
		}
	}
	else if (FileName.size() > 0)
	{
		hr = PrintToFile::Create(FileName.c_str(), &pPrinter);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DWORD BUFFER_SIZE = 81920;
	std::unique_ptr<BYTE[]> buffer;
	if (SUCCEEDED(hr))
	{
		buffer = std::make_unique<BYTE[]>(BUFFER_SIZE);
		if (buffer.get() == nullptr)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		bool bLastChunk = false;
		do
		{
			hr = pStream->Read(buffer.get(), BUFFER_SIZE, &dwBytesRead);
			if (hr == S_FALSE)
			{
				bLastChunk = true;
				hr = S_OK;
			}

			dwBytesLeft = dwBytesRead;

			DBGLOG("Read %u - 0x%08x", dwBytesRead, hr);
			while (SUCCEEDED(hr) && (dwBytesLeft > 0))
			{
				dwWritten = 0;
				
				hr = pPrinter->Write(buffer.get() + (dwBytesRead - dwBytesLeft), dwBytesLeft, &dwWritten);
				
				if (SUCCEEDED(hr) && (dwWritten == 0))
				{
					hr = E_FAIL;
				}
				
				if (SUCCEEDED(hr))
				{
					dwBytesLeft -= dwWritten;
				}
			}

			if (FAILED(hr))
			{
				DBGLOG("hr 0x%08x\n", hr);
			}
			
			dwTotalRead += (dwBytesRead - dwBytesLeft);
			Progress(JobId, JobStateWKVType_Processing, JobStateReasonsWKVType_None, dwTotalRead);
		}
		while ((hr == S_OK) && (!bLastChunk));

		if (FAILED(hr))
		{
			pPrinter->Cancel();
		}

		HRESULT hrPrinter = S_OK;
		pPrinter->Close(&hrPrinter);
		if (SUCCEEDED(hr))
		{
			hr = hrPrinter;
		}
	}

	pStream->Close();

	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pPrinter);

	Progress(JobId, JobStateWKVType_Completed, SUCCEEDED(hr) ? JobStateReasonsWKVType_JobCompletedSuccessfully : JobStateReasonsWKVType_JobCompletedWithErrors, dwTotalRead);

	Release();

	if (FAILED(hr))
	{
		ELOG("Error 0x%08x\n", hr);
	}

	return hr;
}

#endif	//0
