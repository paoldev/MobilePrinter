#include "pch.h"
#include "PrinterSourceStream.h"
#include <algorithm>
#include <WSDAttachment.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	PrinterSourceStream
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PrinterSourceStream::PrinterSourceStream()
{
}

PrinterSourceStream::~PrinterSourceStream()
{
}

HRESULT PrinterSourceStream::Write(_In_reads_bytes_(cb) const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CInMemoryPrinterSourceStream
//       
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CInMemoryPrinterSourceStream* CInMemoryPrinterSourceStream::Create(const uint8_t* i_pBuffer, const size_t i_dwBufferSize, const bool i_bCopyData)
{
	return new CInMemoryPrinterSourceStream(i_pBuffer, i_dwBufferSize, i_bCopyData);
}

CInMemoryPrinterSourceStream* CInMemoryPrinterSourceStream::Create(const std::vector<uint8_t>& i_Buffer, const bool i_bCopyData)
{
	return CInMemoryPrinterSourceStream::Create(i_Buffer.data(), i_Buffer.size(), i_bCopyData);
}

CInMemoryPrinterSourceStream* CInMemoryPrinterSourceStream::Create(std::vector<uint8_t>&& i_Buffer)
{
	return new CInMemoryPrinterSourceStream(std::move(i_Buffer));
}

CInMemoryPrinterSourceStream::CInMemoryPrinterSourceStream(const uint8_t* i_pBuffer, const size_t i_dwBufferSize, const bool i_bCopyData) : m_pBuffer(i_pBuffer), m_dwBufferSize(i_dwBufferSize), m_dwReadPos(0)
{
	if (i_bCopyData)
	{
		m_buffer.assign(i_pBuffer, i_pBuffer + i_dwBufferSize);
		m_pBuffer = m_buffer.data();
	}
}

CInMemoryPrinterSourceStream::CInMemoryPrinterSourceStream(std::vector<uint8_t>&& i_Buffer) : m_pBuffer(nullptr), m_dwBufferSize(i_Buffer.size()), m_dwReadPos(0)
{
	m_buffer = std::move(i_Buffer);
	m_pBuffer = m_buffer.data();
}

CInMemoryPrinterSourceStream::~CInMemoryPrinterSourceStream()
{
}

HRESULT CInMemoryPrinterSourceStream::Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead)
{
	if (pv == nullptr)
	{
		return E_INVALIDARG;
	}

	if (m_pBuffer == nullptr)
	{
		return E_POINTER;
	}

	size_t dwReadBytes = std::min(size_t(cb), m_dwBufferSize - m_dwReadPos);
	memcpy(pv, &m_pBuffer[m_dwReadPos], dwReadBytes);
	m_dwReadPos += dwReadBytes;
	if (pcbRead)
	{
		*pcbRead = static_cast<ULONG>(dwReadBytes);
	}
	return (m_dwReadPos == m_dwBufferSize) ? S_FALSE : S_OK;
}

HRESULT CInMemoryPrinterSourceStream::Close()
{
	m_buffer.clear();
	m_pBuffer = nullptr;
	m_dwBufferSize = 0;
	m_dwReadPos = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CWSDInboundAttachmentPrinterSourceStream
//       
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CWSDInboundAttachmentPrinterSourceStream* CWSDInboundAttachmentPrinterSourceStream::Create(IWSDAttachment* i_pAttachment)
{
	if (i_pAttachment == nullptr)
	{
		return nullptr;
	}

	IWSDInboundAttachment* pAttachment = nullptr;
	HRESULT hr = i_pAttachment->QueryInterface(__uuidof(IWSDInboundAttachment), (void**)&pAttachment);
	if (FAILED(hr))
	{
		return nullptr;
	}

	CWSDInboundAttachmentPrinterSourceStream* pStream = CWSDInboundAttachmentPrinterSourceStream::Create(pAttachment);
	SAFE_RELEASE(pAttachment);
	return pStream;
}

CWSDInboundAttachmentPrinterSourceStream* CWSDInboundAttachmentPrinterSourceStream::Create(IWSDInboundAttachment* i_pAttachment)
{
	if (i_pAttachment == nullptr)
	{
		return nullptr;
	}
	
	return new CWSDInboundAttachmentPrinterSourceStream(i_pAttachment);
}

CWSDInboundAttachmentPrinterSourceStream::CWSDInboundAttachmentPrinterSourceStream(IWSDInboundAttachment* i_pAttachment) : m_pAttachment(i_pAttachment)
{
	if (m_pAttachment)
	{
		m_pAttachment->AddRef();
	}
}

CWSDInboundAttachmentPrinterSourceStream::~CWSDInboundAttachmentPrinterSourceStream()
{
	SAFE_RELEASE(m_pAttachment);
}

HRESULT CWSDInboundAttachmentPrinterSourceStream::Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead)
{
	if (m_pAttachment == nullptr)
	{
		return E_POINTER;
	}

	BYTE* pBuffer = static_cast<BYTE*>(pv);
	ULONG TmpRead = 0;
	return m_pAttachment->Read(pBuffer, cb, pcbRead ? pcbRead : &TmpRead);
}

HRESULT CWSDInboundAttachmentPrinterSourceStream::Close()
{
	HRESULT hr = S_OK;
	if (m_pAttachment)
	{
		hr = m_pAttachment->Close();
		SAFE_RELEASE(m_pAttachment);
	}
	return hr;
}
