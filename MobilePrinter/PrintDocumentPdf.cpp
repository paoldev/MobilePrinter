#include "pch.h"
#include "PrintDocumentPdf.h"
#include "PrintDocumentCommon.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.storage.streams.h>
#include <winrt/windows.data.pdf.h>
#include <windows.data.pdf.h>
#include <windows.data.pdf.interop.h>
#include <ppltasks.h>

PrintPdf::PrintPdf() : m_pPrinter(nullptr), m_hrPrinter(S_OK)
{
}

PrintPdf::~PrintPdf()
{
	Close(nullptr);
}

HRESULT PrintPdf::Create(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName, BasePrinter ** o_ppPrinter)
{
	if (o_ppPrinter == nullptr)
	{
		return E_POINTER;
	}

	if ((lpPrinterName == nullptr) || (lpDocumentName == nullptr))
	{
		return E_INVALIDARG;
	}

	*o_ppPrinter = nullptr;

	PrintPdf* pPrinter = new PrintPdf();
	if (pPrinter == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pPrinter->Initialize(lpPrinterName, lpDocumentName);
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

HRESULT PrintPdf::Initialize(LPCWSTR lpPrinterName, LPCWSTR lpDocumentName)
{
	m_hrPrinter = m_CoInit.Initialize(/*COINIT_APARTMENTTHREADED*/COINIT_MULTITHREADED);

	if (SUCCEEDED(m_hrPrinter))
	{
		m_pPrinter = new PrintDocumentBase();
		if (m_pPrinter == nullptr)
		{
			m_hrPrinter = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(m_hrPrinter))
	{
		m_hrPrinter = m_pPrinter->Initialize();
	}

	if (SUCCEEDED(m_hrPrinter))
	{
		//hr = m_pPrinter->Print(lpPrinterName, lpDocumentName);
		m_printer_name = lpPrinterName;
		m_document_name = lpDocumentName;
	}

	return m_hrPrinter;
}

void PrintPdf::Uninitialize()
{
	if (m_pPrinter)
	{
		delete m_pPrinter;
		m_pPrinter = nullptr;
	}

	m_CoInit.Uninitialize();
}

HRESULT __stdcall PrintPdf::Write(_In_reads_bytes_(cb)  const void* pv, _In_  ULONG cb, _Out_opt_  ULONG* pcbWritten)
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

	auto* pData = static_cast<const uint8_t*>(pv);
	m_buffer.insert(m_buffer.end(), pData, pData + cb);

	if (pcbWritten)
	{
		*pcbWritten = cb;
	}

	return S_OK;
}

void __stdcall PrintPdf::Cancel()
{
	if (SUCCEEDED(m_hrPrinter))
	{
		m_buffer.clear();
		m_hrPrinter = E_ABORT;
	}
}

void __stdcall PrintPdf::Close(HRESULT * pPrintResult)
{
	if (SUCCEEDED(m_hrPrinter) && m_buffer.size())
	{
		try
		{
			winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;
			{
				winrt::Windows::Storage::Streams::DataWriter writer(stream.GetOutputStreamAt(0));
				writer.WriteBytes(m_buffer);
				writer.StoreAsync().get();
				writer.FlushAsync().get();
				(void)writer.DetachStream();
			}

			HRESULT hr = S_OK;
			stream.Seek(0);
			auto pdf = winrt::Windows::Data::Pdf::PdfDocument::LoadFromStreamAsync(stream).get();
			{
				uint32_t NumPages = pdf.PageCount();
				if (NumPages > 0)
				{
					IPdfRendererNative* pPdfRender = nullptr;

					if (SUCCEEDED(hr))
					{
						hr = m_pPrinter->Print(m_printer_name.c_str(), m_document_name.c_str(),

							[pdf, NumPages, &pPdfRender](IDXGIDevice* DXGIDevice, ID2D1DeviceContext* PageContext, uint32_t PageIndex, __out std::pair<float, float>& PageSize)->HRESULT
						{
							HRESULT hrPage = S_OK;

							if (PageIndex >= NumPages)
							{
								hrPage = E_INVALIDARG;
							}

							if ((DXGIDevice == nullptr) || (PageContext == nullptr))
							{
								hrPage = E_INVALIDARG;
							}

							if (SUCCEEDED(hrPage))
							{
								if (pPdfRender == nullptr)
								{
									hrPage = PdfCreateRenderer(DXGIDevice, &pPdfRender);
								}
							}

							if (SUCCEEDED(hrPage))
							{
								if (pPdfRender == nullptr)
								{
									hrPage = E_OUTOFMEMORY;
								}
							}

							if (SUCCEEDED(hrPage))
							{
								auto pdfPage = pdf.GetPage(PageIndex);
								auto pPage = pdfPage.as<IUnknown>();
								if (pPage)
								{
									auto pageSize = pdfPage.Size();
									PageSize.first = pageSize.Width;
									PageSize.second = pageSize.Height;
									//PDF_RENDER_PARAMS params{};

									PageContext->BeginDraw();

									//PageContext->SetDpi(600.0f, 600.0f);

									//PageContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

									//PageContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

									hrPage = pPdfRender->RenderPageToDeviceContext(pPage.get(), PageContext, nullptr/*&params*/);

									HRESULT hrDraw = PageContext->EndDraw(0, 0);
									if (SUCCEEDED(hrPage))
									{
										hrPage = hrDraw;
									}
								}
								else
								{
									hrPage = E_NOINTERFACE;
								}
							}

							if (SUCCEEDED(hrPage) && (PageIndex == (NumPages - 1)))
							{
								hrPage = S_FALSE;
							}

							return hrPage;
						}
						);

						SAFE_RELEASE(pPdfRender);
					}
				}
			}

			m_hrPrinter = hr;
		}
		catch (winrt::hresult_error const& e)
		{
			m_hrPrinter = e.to_abi();
		}
		catch (...)
		{
			m_hrPrinter = E_FAIL;
		}

		m_buffer.clear();
	}

	if (pPrintResult)
	{
		*pPrintResult = m_hrPrinter;
	}

	Uninitialize();
}
