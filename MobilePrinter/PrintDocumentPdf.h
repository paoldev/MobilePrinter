#pragma once

#include "PrinterInterface.h"

class PrintDocumentBase;
struct IRandomAccessStream;

class PrintPdf : public BasePrinter
{
public:

	static HRESULT Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter** o_ppPrinter);

	//ISequentialStream
	virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv, _In_ ULONG cb, _Out_opt_ ULONG* pcbWritten) override final;

	//BasePrinter
	virtual void STDMETHODCALLTYPE Close(HRESULT* pPrintResult) override final;
	virtual void STDMETHODCALLTYPE Cancel() override final;

private:
	PrintPdf();
	~PrintPdf();

	HRESULT Initialize(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName);
	void Uninitialize();

private:

	PrintDocumentBase* m_pPrinter;
	std::wstring m_printer_name;
	std::wstring m_document_name;
	std::vector<uint8_t> m_buffer;
	HRESULT m_hrPrinter;
	CCoInit m_CoInit;
};
