//// 
//// This class is derived from DemoApp implemented at
//// https://github.com/Microsoft/Windows-classic-samples/tree/master/Samples/D2DPrintingFromDesktopApps
//// 
//// My changes:
//// -	Extracted all the print-related stuff.
//// 

#pragma once

// DirectX header files.
#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>
#include <Windows.h>
#include <WinUser.h>

#include <xpsobjectmodel_1.h>
#include <DocumentTarget.h>

#include "PrintDocumentMonitor.h"

struct ID2D1Factory1;
struct IWICImagingFactory2;
struct ID2D1Device;
struct IDXGIDevice;
struct IStream;
struct ID2D1PrintControl;
struct IPrintDocumentPackageTarget;
class XpsPrintJobChecker;

class PrintDocumentBase
{
public:
    PrintDocumentBase();

    ~PrintDocumentBase();

    HRESULT Initialize();
 
	HRESULT Print(const wchar_t* lpPrinterName, const wchar_t* lpDocumentName, std::function<HRESULT(IDXGIDevice* DXGIDevice, ID2D1DeviceContext* PageContext, uint32_t PageIndex, __out std::pair<float, float>& PageSize)> PageIterator);

private:
    HRESULT CreateDeviceIndependentResources();

    HRESULT CreateDeviceContext();

    HRESULT CreateDeviceResources();

    void DiscardDeviceResources();

    HRESULT InitializePrintJob(const wchar_t* lpPrinterName, const wchar_t* lpDocumentName);

    HRESULT GetPrintTicketFromDevmode(
        _In_ PCTSTR printerName,
        _In_reads_bytes_(devModeSize) PDEVMODE devMode,
        WORD devModeSize,
        _Out_ LPSTREAM* printTicketStream
        );

    HRESULT FinalizePrintJob(uint32_t PrintedPages);

    LRESULT OnClose();

private:
    bool m_resourcesValid;                      // Whether or not the device-dependent resources are ready to use.

    // Device-independent resources.
    ID2D1Factory1* m_d2dFactory;
    IWICImagingFactory2* m_wicFactory;

    // Device-dependent resources.
    ID2D1Device* m_d2dDevice;
	IDXGIDevice* m_dxgiDevice;

    // Printing-specific resources.
    IStream* m_jobPrintTicketStream;
    ID2D1PrintControl* m_printControl;
    IPrintDocumentPackageTarget* m_documentTarget;
    XpsPrintJobChecker* m_printJobChecker;

    // Page size (in DIPs).
    float m_pageHeight;
    float m_pageWidth;

	bool m_bCoInit;
};
