#pragma once

#include <vector>
#include <ObjIdlBase.h>
#include "CommonUtils.h"

class PrinterSourceStream : public CUnknown<PrinterSourceStream, ISequentialStream>
{
public:

	virtual ~PrinterSourceStream();

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten) final;

	//PrinterSourceStream
	virtual HRESULT STDMETHODCALLTYPE Close() = 0;

protected:
	PrinterSourceStream();
};

class CInMemoryPrinterSourceStream : public PrinterSourceStream
{
public:

	static CInMemoryPrinterSourceStream* Create(const uint8_t* i_pBuffer, const size_t i_dwBufferSize, const bool i_bCopyData);
	static CInMemoryPrinterSourceStream* Create(const std::vector<uint8_t>& i_Buffer, const bool i_bCopyData);
	static CInMemoryPrinterSourceStream* Create(std::vector<uint8_t>&& i_Buffer);

	virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Close();

private:
	CInMemoryPrinterSourceStream(const uint8_t* i_pBuffer, const size_t i_dwBufferSize, const bool i_bCopyData);
	CInMemoryPrinterSourceStream(std::vector<uint8_t>&& i_Buffer);

	~CInMemoryPrinterSourceStream();

private:
	const uint8_t* m_pBuffer;
	size_t m_dwBufferSize;
	size_t m_dwReadPos;
	std::vector<uint8_t> m_buffer;
};

struct IWSDAttachment;
struct IWSDInboundAttachment;

class CWSDInboundAttachmentPrinterSourceStream : public PrinterSourceStream
{
public:

	static CWSDInboundAttachmentPrinterSourceStream* Create(IWSDAttachment* i_pAttachment);
	static CWSDInboundAttachmentPrinterSourceStream* Create(IWSDInboundAttachment* i_pAttachment);

	virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Close();

private:
	CWSDInboundAttachmentPrinterSourceStream(IWSDInboundAttachment* i_pAttachment);
	~CWSDInboundAttachmentPrinterSourceStream();

private:
	IWSDInboundAttachment* m_pAttachment;
};
