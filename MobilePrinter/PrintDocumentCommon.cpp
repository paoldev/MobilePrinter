//// 
//// This class is derived from PrintDocumentBase implemented at
//// https://github.com/Microsoft/Windows-classic-samples/tree/master/Samples/D2DPrintingFromDesktopApps
//// 
//// My changes:
//// -	Extracted all the print-related stuff.
//// 

#include "pch.h"
#include "PrintDocumentCommon.h"

#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>
#include <xpsobjectmodel_1.h>
#include <DocumentTarget.h>
#include <Prntvpt.h>
#pragma comment(lib, "prntvpt.lib")

static const FLOAT PAGE_WIDTH_IN_DIPS    = 8.5f * 96.0f;     // 8.5 inches
static const FLOAT PAGE_HEIGHT_IN_DIPS   = 11.0f * 96.0f;    // 11 inches
static const FLOAT PAGE_MARGIN_IN_DIPS   = 96.0f;            // 1 inch
static const FLOAT FRAME_HEIGHT_IN_DIPS  = 400.0f;           // 400 DIPs
static const FLOAT HOURGLASS_SIZE        = 200.0f;           // 200 DIPs

// Initializes members.
PrintDocumentBase::PrintDocumentBase() :
    m_resourcesValid(false),
    m_d2dFactory(nullptr),
    m_wicFactory(nullptr),
    m_d2dDevice(nullptr),
	m_dxgiDevice(nullptr),
    m_jobPrintTicketStream(nullptr),
    m_printControl(nullptr),
    m_documentTarget(nullptr),
    m_printJobChecker(nullptr),
    m_pageHeight(PAGE_HEIGHT_IN_DIPS),
    m_pageWidth(PAGE_WIDTH_IN_DIPS),
	m_bCoInit(false)
{
}

// Releases resources.
PrintDocumentBase::~PrintDocumentBase()
{
    // Release device-dependent resources.
    SAFE_RELEASE(m_d2dDevice);
	SAFE_RELEASE(m_dxgiDevice);

    // Release printing-specific resources.
    SAFE_RELEASE(m_jobPrintTicketStream);
    SAFE_RELEASE(m_printControl);
    SAFE_RELEASE(m_documentTarget);
    SAFE_RELEASE(m_printJobChecker);

    // Release factories.
    SAFE_RELEASE(m_d2dFactory);
    SAFE_RELEASE(m_wicFactory);

	if (m_bCoInit)
	{
		CoUninitialize();
		m_bCoInit = false;
	}
}

// Creates the application window and initializes
// device-independent and device-dependent resources.
HRESULT PrintDocumentBase::Initialize()
{
	m_bCoInit = false;

	HRESULT hr = CoInitializeEx(nullptr, /*COINIT_APARTMENTTHREADED*/COINIT_MULTITHREADED);
	
    // Initialize device-indpendent resources, such
    // as the Direct2D factory.
	if (SUCCEEDED(hr))
	{
		m_bCoInit = true;
		hr = CreateDeviceIndependentResources();
	}

    // Create D2D device context and device-dependent resources.
    if (SUCCEEDED(hr))
    {
        hr = CreateDeviceResources();
    }

    if (FAILED(hr))
    {
    }

    return hr;
}

// Creates resources which are not bound to any device.
// Their lifetimes effectively extend for the duration
// of the app.
HRESULT PrintDocumentBase::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        // Create a Direct2D factory.
        D2D1_FACTORY_OPTIONS options;
        ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
        // If the project is in a debug build, enable Direct2D debugging via SDK Layers
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        hr = D2D1CreateFactory(
            //D2D1_FACTORY_TYPE_SINGLE_THREADED,
			D2D1_FACTORY_TYPE_MULTI_THREADED, //D2D DEBUG ERROR - A rental threaded interface was simultaneously accessed from multiple threads.
            options,
            &m_d2dFactory
            );
    }
    if (SUCCEEDED(hr))
    {
        // Create a WIC factory.
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_wicFactory)
            );
    }

    return hr;
}

// Create D2D context for display (Direct3D) device
HRESULT PrintDocumentBase::CreateDeviceContext()
{
    HRESULT hr = S_OK;

    // Create a D3D device and a swap chain sized to the child window.
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
    };
    UINT countOfDriverTypes = ARRAYSIZE(driverTypes);

    ID3D11Device* d3dDevice = nullptr;
    for (UINT driverTypeIndex = 0; driverTypeIndex < countOfDriverTypes; driverTypeIndex++)
    {
        hr = D3D11CreateDevice(
            nullptr,       // use default adapter
            driverTypes[driverTypeIndex],
            nullptr,       // no external software rasterizer
            createDeviceFlags,
            nullptr,       // use default set of feature levels
            0,
            D3D11_SDK_VERSION,
            &d3dDevice,
            nullptr,       // do not care about what feature level is chosen
            nullptr        // do not retain D3D device context
            );

        if (SUCCEEDED(hr))
        {
            break;
        }
    }

    //IDXGIDevice* dxgiDevice = nullptr;
    if (SUCCEEDED(hr))
    {
        // Get a DXGI device interface from the D3D device.
        hr = d3dDevice->QueryInterface(&m_dxgiDevice);
    }
    if (SUCCEEDED(hr))
    {
        // Create a D2D device from the DXGI device.
        hr = m_d2dFactory->CreateDevice(
			m_dxgiDevice,
            &m_d2dDevice
            );
    }

    //SAFE_RELEASE(dxgiDevice);
    SAFE_RELEASE(d3dDevice);
    return hr;
}

// This method creates resources which are bound to a particular
// Direct3D device. It's all centralized here, in case the resources
// need to be recreated in case of Direct3D device loss (e.g. display
// change, remoting, removal of video card, etc). The resources created
// here can be used by multiple Direct2D device contexts (in this
// sample, one for display and another for print) which are created
// from the same Direct2D device.
HRESULT PrintDocumentBase::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_resourcesValid)
    {
        hr = CreateDeviceContext();
    }

    if (FAILED(hr))
    {
        DiscardDeviceResources();
    }
    else
    {
        m_resourcesValid = true;
    }

    return hr;
}

// Discards device-specific resources which need to be recreated
// when a Direct3D device is lost.
void PrintDocumentBase::DiscardDeviceResources()
{
    SAFE_RELEASE(m_d2dDevice);
	SAFE_RELEASE(m_dxgiDevice);

    m_resourcesValid = false;
}

// Called whenever the application begins a print job. Initializes
// the printing subsystem, draws the scene to a printing device
// context, and commits the job to the printing subsystem.
HRESULT PrintDocumentBase::Print(const wchar_t* lpPrinterName, const wchar_t* lpDocumentName, std::function<HRESULT(IDXGIDevice* DXGIDevice, ID2D1DeviceContext* PageContext, uint32_t PageIndex, std::pair<float, float>& PageSize)> PageIterator)
{
    HRESULT hr = S_OK;

    if (!m_resourcesValid)
    {
        hr = CreateDeviceResources();
    }

    if (SUCCEEDED(hr))
    {
        // Initialize printing-specific resources and prepare the
        // printing subsystem for a job.
        hr = InitializePrintJob(lpPrinterName, lpDocumentName);
    }

    ID2D1DeviceContext* d2dContextForPrint = nullptr;
    if (SUCCEEDED(hr))
    {
        // Create a D2D Device Context dedicated for the print job.
        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContextForPrint);
    }

    if (SUCCEEDED(hr))
    {
        ID2D1CommandList* commandList = nullptr;

		std::pair<float, float> pageSize;

		bool LastPage = false;
		uint32_t SuccessfulPages = 0;
        for (INT pageIndex = 0; SUCCEEDED(hr) && (!LastPage); pageIndex++)
        {
            hr = d2dContextForPrint->CreateCommandList(&commandList);

            // Create, draw, and add a Direct2D Command List for each page.
            if (SUCCEEDED(hr))
            {
                d2dContextForPrint->SetTarget(commandList);
				pageSize = {};
                hr = PageIterator(m_dxgiDevice, d2dContextForPrint, pageIndex, pageSize);
                commandList->Close();

				LastPage = (hr == S_FALSE);
            }

            if (SUCCEEDED(hr))
            {
                hr = m_printControl->AddPage(commandList, D2D1::SizeF(pageSize.first, pageSize.second), nullptr);
				if (SUCCEEDED(hr))
				{
					SuccessfulPages++;
				}
            }

            SAFE_RELEASE(commandList);
        }

        // Release the print device context.
        SAFE_RELEASE(d2dContextForPrint);

        // Send the job to the printing subsystem and discard
        // printing-specific resources.
        HRESULT hrFinal = FinalizePrintJob(SuccessfulPages);

        if (SUCCEEDED(hr))
        {
            hr = hrFinal;
        }
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
    }

    return hr;
}

// Brings up a Print Dialog to collect user print
// settings, then creates and initializes a print
// control object properly for a new print job.
HRESULT PrintDocumentBase::InitializePrintJob(const wchar_t* lpPrinterName, const wchar_t* lpDocumentName)
{
    HRESULT hr = S_OK;

	PRINTER_INFO_8* pPRINTER_INFO_8 = nullptr;

	HANDLE hPrinter = INVALID_HANDLE_VALUE;
	if (OpenPrinter2W(lpPrinterName, &hPrinter, nullptr, nullptr))
	{
		DWORD iLevel = 8;
		void* pInfo = nullptr;
		DWORD cbNeeded = 0;
		GetPrinterW(hPrinter, iLevel, nullptr, 0, &cbNeeded);
		if (cbNeeded)
		{
			pInfo = malloc(cbNeeded);
			if (pInfo != nullptr)
			{
				if (!GetPrinterW(hPrinter, iLevel, (LPBYTE)pInfo, cbNeeded, &cbNeeded))
				{
					free(pInfo);
					pInfo = nullptr;
				}
			}
		}

		ClosePrinter(hPrinter);

		pPRINTER_INFO_8 = static_cast<PRINTER_INFO_8*>(pInfo);
	}

	if (pPRINTER_INFO_8 && (pPRINTER_INFO_8->pDevMode == nullptr))
	{
		free(pPRINTER_INFO_8);
		pPRINTER_INFO_8 = nullptr;
	}
	
	if (pPRINTER_INFO_8 == nullptr)
	{
		hr = E_FAIL;
	}

	// Retrieve user settings from print dialog.
	DEVMODE* devMode = nullptr;
    if (SUCCEEDED(hr))
    {
        if (pPRINTER_INFO_8->pDevMode != nullptr)
        {
            devMode = pPRINTER_INFO_8->pDevMode;   // retrieve DevMode

            if (devMode)
            {
                // Must check corresponding flags in devMode->dmFields
                if ((devMode->dmFields & DM_PAPERLENGTH) && (devMode->dmFields & DM_PAPERWIDTH))
                {
                    // Convert 1/10 of a millimeter DEVMODE unit to 1/96 of inch D2D unit
                    m_pageHeight = devMode->dmPaperLength / 254.0f * 96.0f;
                    m_pageWidth  = devMode->dmPaperWidth / 254.0f * 96.0f;
                }
                else
                {
                    // Use default values if the user does not specify page size.
                    m_pageHeight = PAGE_HEIGHT_IN_DIPS;
                    m_pageWidth  = PAGE_WIDTH_IN_DIPS;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = E_HANDLE;
        }
    }

    // Convert DEVMODE to a job print ticket stream.
    if (SUCCEEDED(hr))
    {
        hr = GetPrintTicketFromDevmode(
			lpPrinterName,
            devMode,
            devMode->dmSize + devMode->dmDriverExtra, // Size of DEVMODE in bytes, including private driver data.
            &m_jobPrintTicketStream
            );
    }

    // Create a factory for document print job.
    IPrintDocumentPackageTargetFactory* documentTargetFactory = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = ::CoCreateInstance(
            __uuidof(PrintDocumentPackageTargetFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&documentTargetFactory)
            );
    }

    // Initialize the print subsystem and get a package target.
    if (SUCCEEDED(hr))
    {
        hr = documentTargetFactory->CreateDocumentPackageTargetForPrintJob(
			lpPrinterName,                                // printer name
			lpDocumentName ? lpDocumentName : L"Document",    // job name
            nullptr,                                    // job output stream; when nullptr, send to printer
            m_jobPrintTicketStream,                     // job print ticket
            &m_documentTarget                           // result IPrintDocumentPackageTarget object
            );
    }

    // Create a new print control linked to the package target.
    if (SUCCEEDED(hr))
    {
        hr = m_d2dDevice->CreatePrintControl(
            m_wicFactory,
            m_documentTarget,
            nullptr,
            &m_printControl
            );
    }

    // Create and register a print job checker.
    if (SUCCEEDED(hr))
    {
        SAFE_RELEASE(m_printJobChecker);
        hr = XpsPrintJobChecker::Create(&m_printJobChecker);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_printJobChecker->Initialize(m_documentTarget);
    }

    // Release resources.

	if (pPRINTER_INFO_8)
	{
		free(pPRINTER_INFO_8);
	}

    SAFE_RELEASE(documentTargetFactory);

    return hr;
}

// Creates a print job ticket stream to define options for the next print job.
HRESULT PrintDocumentBase::GetPrintTicketFromDevmode(
    _In_ PCTSTR printerName,
    _In_reads_bytes_(devModesize) PDEVMODE devMode,
    WORD devModesize,
    _Out_ LPSTREAM* printTicketStream)
{
    HRESULT hr = S_OK;
    HPTPROVIDER provider = nullptr;

    *printTicketStream = nullptr;

    // Allocate stream for print ticket.
    hr = CreateStreamOnHGlobal(nullptr, TRUE, printTicketStream);
	if (SUCCEEDED(hr) && ((*printTicketStream) == nullptr))
	{
		//Disable intellisense C6387 '*printTicketStream' could be '0':  this does not adhere to the specification for the function 'PTConvertDevModeToPrintTicket'.
		hr = E_FAIL;
	}

    if (SUCCEEDED(hr))
    {
        hr = PTOpenProvider(printerName, 1, &provider);
    }

    // Get PrintTicket from DEVMODE.
    if (SUCCEEDED(hr))
    {
        hr = PTConvertDevModeToPrintTicket(provider, devModesize, devMode, kPTJobScope, *printTicketStream);
    }

    if (FAILED(hr) && ((*printTicketStream) != nullptr))
    {
        // Release printTicketStream if fails.
        SAFE_RELEASE((*printTicketStream));
    }

    if (provider)
    {
        PTCloseProvider(provider);
    }

    return hr;
}


// Commits the current print job to the printing subystem by
// closing the print control, and releases all printing-
// specific resources.
HRESULT PrintDocumentBase::FinalizePrintJob(uint32_t PrintedPages)
{
	HRESULT hr = S_OK;
	
	if (PrintedPages > 0)
	{
		// Send the print job to the print subsystem. (When this call
		// returns, we are safe to release printing resources.)
		// If PrinterPage==0, "D2D DEBUG ERROR - Direct2D Print: Close is called with no previous AddPage calls"
		m_printControl->Close();
	}

    SAFE_RELEASE(m_jobPrintTicketStream);
    SAFE_RELEASE(m_printControl);

    return hr;
}

// Close the sample window after checking print job status.
LRESULT PrintDocumentBase::OnClose()
{
    bool close = true;

    if (m_printJobChecker != nullptr)
    {
        PrintDocumentPackageStatus status = m_printJobChecker->GetStatus();
        if (status.Completion == PrintDocumentPackageCompletion_InProgress)
        {
			// Exit after print job is complete.
			//Cancel?
            m_printJobChecker->WaitForCompletion();
        }
    }

    return close ? 0 : 1;
}
