#pragma once

//No more needed. See PrinterSourceStream.
#if 0

#include "CommonUtils.h"

class CWSDPrinterService;

class CInMemoryWSDInboundAttachment : public CUnknown<CInMemoryWSDInboundAttachment, IWSDInboundAttachment>
{
public:
	
	static CInMemoryWSDInboundAttachment* Create(const BYTE* i_pBuffer, const DWORD i_dwBufferSize, const bool i_bOwnData);
	static CInMemoryWSDInboundAttachment* Create(const std::vector<uint8_t>& i_Buffer, const bool i_bOwnData);
	static CInMemoryWSDInboundAttachment* Create(std::vector<uint8_t>&& i_Buffer);

	virtual HRESULT STDMETHODCALLTYPE Read(BYTE *pBuffer, DWORD dwBytesToRead, LPDWORD pdwNumberOfBytesRead);
	virtual HRESULT STDMETHODCALLTYPE Close();

private:
	CInMemoryWSDInboundAttachment(const BYTE* i_pBuffer, const DWORD i_dwBufferSize, const bool i_bOwnData);
	CInMemoryWSDInboundAttachment(const std::vector<uint8_t>& i_Buffer, const bool i_bOwnData);
	CInMemoryWSDInboundAttachment(std::vector<uint8_t>&& i_Buffer);

	~CInMemoryWSDInboundAttachment();

private:
	const BYTE* m_pBuffer;
	DWORD m_dwBufferSize;
	DWORD m_dwReadPos;
	std::vector<uint8_t> m_buffer;
};

//////////////////////////////////////////////////////////////////////////////
// CSendAttachmentThread Class
//       Performs worker thread sends an attachment back to the client
//////////////////////////////////////////////////////////////////////////////
class CSendAttachmentThread : public CUnknown<CSendAttachmentThread>
{
public:
	CSendAttachmentThread();

	IWSDOutboundAttachment* SendAsync(const BYTE* lpByte, const DWORD dwSize);
	IWSDOutboundAttachment* SendAsync(const wchar_t* lpFileName);

private:

	~CSendAttachmentThread();

	HRESULT Start();
	HRESULT ThreadProc();
	static DWORD WINAPI StaticThreadProc(LPVOID pParams);

private:

	DWORD BufferSize;
	BYTE* Buffer;
	std::wstring FileName;
	IWSDOutboundAttachment* m_pAttachment;
};


//////////////////////////////////////////////////////////////////////////////
// CReadAttachmentThread Class
//       Performs worker thread reads an attachment from the client
//////////////////////////////////////////////////////////////////////////////
class CReceiveAttachmentThread : public CUnknown<CReceiveAttachmentThread>
{
public:
	typedef void ProgressFunc(LONG JobId, const wchar_t* Status, const wchar_t* Reason, size_t TotalByteReads);

	CReceiveAttachmentThread();

	HRESULT ReceiveAsyncToPrinter(HANDLE i_hPrinter, LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc);
	HRESULT ReceiveAsyncToPrinter(const wchar_t* lpPrinterName, LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc);
	HRESULT ReceiveAsyncToFile(const wchar_t* lpFileName, LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc);
	HRESULT ReceiveAsyncToAutoFileName(LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc);

private:

	~CReceiveAttachmentThread();

	HRESULT Start();
	HRESULT ThreadProc();
	HRESULT ReceiveAsyncInternal(LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, IWSDAttachment* pAttachment, std::function< ProgressFunc > InFunc);
	static DWORD WINAPI StaticThreadProc(LPVOID pParams);

private:
	
	std::function< ProgressFunc > Progress;
	IWSDAttachment* m_pAttachment;
	
	LONG JobId;
	std::wstring DocumentName;
	std::wstring DocumentFormat;

	std::wstring PrinterName;
	std::wstring FileName;
	HANDLE m_hPrinter;
};

#endif	//0
