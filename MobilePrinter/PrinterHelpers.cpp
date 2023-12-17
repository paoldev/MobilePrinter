#include "pch.h"
#include "PrinterHelpers.h"
#include "PrinterInterface.h"
#include "PrintDocumentPdf.h"

#include <filesystem>

namespace details
{
	std::wstring get_extension_lowercase(const wchar_t* i_FileName)
	{
		std::wstring ext;
		if (i_FileName)
		{
			std::filesystem::path filename(i_FileName);
			ext = filename.extension().native();
		}
		return ext;
	}

	bool buffer_begins_with(const void* i_pBuffer, const size_t i_uiBufferSize, const void* i_pComparisonTerm, const size_t i_uiComparisonTermSize)
	{
		if (i_pBuffer && i_pComparisonTerm && (i_uiBufferSize >= sizeof(i_uiComparisonTermSize)))
		{
			return (memcmp(i_pBuffer, i_pComparisonTerm, i_uiComparisonTermSize) == 0);
		}
		return false;
	}
}

bool IsPdfFileName(const wchar_t* i_FileName)
{
	std::wstring ext = details::get_extension_lowercase(i_FileName);
	return imatch(ext, L"pdf");
}

bool IsXpsFileName(const wchar_t* i_FileName)
{
	std::wstring ext = details::get_extension_lowercase(i_FileName);
	return imatch(ext, L"xps");
}

bool IsPdfDocumentFormat(const wchar_t* i_DocumentFormat)
{
	return (i_DocumentFormat != nullptr) ? (_wcsicmp(i_DocumentFormat, L"application/pdf") == 0) : false;
}

bool IsXpsDocumentFormat(const wchar_t* i_DocumentFormat)
{
	if (i_DocumentFormat)
	{
		if ((_wcsicmp(i_DocumentFormat, L"application/vnd.ms-xpsdocument") == 0) ||
			(_wcsicmp(i_DocumentFormat, L"application/oxps") == 0))
		{
			return true;
		}
	}
	return false;
}

bool IsPdfFile(const void* i_pBuffer, const size_t i_uiBufferSize)
{
	constexpr uint8_t PDFHeader[5] = { 0x25, 0x50, 0x44, 0x46, 0x2D };
	return details::buffer_begins_with(i_pBuffer, i_uiBufferSize, PDFHeader, sizeof(PDFHeader));
}

bool IsXpsFile(const void* i_pBuffer, const size_t i_uiBufferSize)
{
	constexpr uint8_t XPSHeader[4] = { 0x50, 0x4B, 0x03, 0x04 };	//This is common to any pkzipped files.
	return details::buffer_begins_with(i_pBuffer, i_uiBufferSize, XPSHeader, sizeof(XPSHeader));
}

//Helper to directly print a file to the specified printer.
void PrintLocalFile(const wchar_t* lpPrinterName, const wchar_t* lpFileName)
{
	if ((lpPrinterName == nullptr) || (lpFileName == nullptr))
	{
		return;
	}

	std::vector<uint8_t> buff;
	LoadFile(lpFileName, buff);

	if (buff.size())
	{
		HRESULT hr = E_FAIL;
		BasePrinter* pPrinter = nullptr;
		if (IsXpsFile(buff))	//IsXpsFileName(lpFileName))
		{
			hr = PrintToXpsPrintDocumentPrinter::Create(lpPrinterName, lpFileName, &pPrinter);
			//PrintToXpsPrintApiPrinter::Create(lpPrinterName, lpFileName, &pPrinter);
		}
		else if (IsPdfFile(buff))	//IsPdfFileName(lpFileName))
		{
			hr = PrintPdf::Create(lpPrinterName, lpFileName, &pPrinter);
		}
		else
		{
			hr = PrintToRawPrinter::Create(lpPrinterName, lpFileName, &pPrinter);
		}

		if (SUCCEEDED(hr))
		{
			ULONG dwWritten = 0;
			HRESULT hrf = pPrinter->Print(buff.data(), static_cast<ULONG>(buff.size()), &dwWritten);
			pPrinter->Close(&hrf);
			pPrinter->Release();
		}
	}
}
