#pragma once

#include "CommonUtils.h"
#include <combaseapi.h>

class BasePrinter : public CUnknown<BasePrinter, ISequentialStream>
{
public:

	virtual ~BasePrinter();

	//Safely send a buffer to printer
	HRESULT Print(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbRead) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) = 0;
	virtual void STDMETHODCALLTYPE Cancel() = 0;

protected:
	BasePrinter();
};

class PrintToFile : public BasePrinter
{
public:

	static HRESULT Create(LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) override final;
	virtual void STDMETHODCALLTYPE Cancel() override final;

private:
	PrintToFile();
	~PrintToFile();

private:
	HRESULT m_hrPrinter;
	HANDLE  m_hFile;
};

class PrintToRawPrinter : public BasePrinter
{
public:

	static HRESULT Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);
	static HRESULT Create(HANDLE hPrinter, LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) override final;
	virtual void STDMETHODCALLTYPE Cancel() override final;

private:
	PrintToRawPrinter();
	~PrintToRawPrinter();

private:
	HRESULT m_hrPrinter;
	HANDLE  m_hPrinter;
	bool	m_bOwnedHandle;
};

struct IXpsPrintJob;
struct IXpsPrintJobStream;

class PrintToXpsPrintApiPrinter : public BasePrinter
{
public:

	static HRESULT Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) override final;
	virtual void STDMETHODCALLTYPE Cancel() override final;

private:
	PrintToXpsPrintApiPrinter();
	~PrintToXpsPrintApiPrinter();

private:
	HRESULT m_hrPrinter;

	HANDLE m_hProgressEvent;
	HANDLE m_hCompletionEvent;
	IXpsPrintJob* m_xpsPrintJob;
	IXpsPrintJobStream* m_xpsDocumentStream;
	bool m_bCoInit;
};

struct IStream;
struct IPrintDocumentPackageTarget;

class PrintToXpsPrintDocumentPrinter : public BasePrinter
{
public:

	static HRESULT Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);

	static void Config(const bool i_bIgnoreXpsSmallElements, const bool i_bIgnoreXpsSmallAbsoluteElements);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) override final;
	virtual void STDMETHODCALLTYPE Cancel() override final;

private:
	PrintToXpsPrintDocumentPrinter();
	~PrintToXpsPrintDocumentPrinter();

private:
	HRESULT m_hrPrinter;

	IPrintDocumentPackageTarget* m_xpsDocPackageTarget;
	IStream* m_xpsJobStream;
	bool m_bCoInit;
};
