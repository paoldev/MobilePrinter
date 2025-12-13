#include "pch.h"
#include "PrinterInfo.h"
#include "PrinterJobs.h"

CPrinterInfo* CPrinterInfo::Create(const wchar_t* i_pPrinterName)
{
	CPrinterInfo* pPrinter = new CPrinterInfo();
	if (pPrinter != nullptr)
	{
		if (!pPrinter->Init(i_pPrinterName))
		{
			ELOG("Error initializing printer %ls", i_pPrinterName);
			pPrinter->Release();
			pPrinter = nullptr;
		}
	}
	return pPrinter;
}

std::vector<std::wstring> CPrinterInfo::EnumeratePrinters()
{
	std::vector<std::wstring> printers;

	DWORD cnt = 0;
	DWORD sz = 0;
	DWORD Level = 1;

	void* pInfo = nullptr;
	if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_NETWORK, nullptr, Level, NULL, 0, &sz, &cnt) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		pInfo = malloc(sz);
		if (pInfo != nullptr)
		{
			if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_NETWORK, nullptr, Level, (LPBYTE)pInfo, sz, &sz, &cnt))
			{
				free(pInfo);
				pInfo = nullptr;
			}
		}
	}
	if (pInfo != nullptr)
	{
		PRINTER_INFO_1* info = static_cast<PRINTER_INFO_1*>(pInfo);
		for (DWORD i = 0; i < cnt; i++)
		{
			//As of Win10SDK 10.0.17763.0, PRINTER_INFO_1 doesn't have [string] annotations. pName is a zero-terminated string.
			//C6385	Reading invalid data from 'info':  the readable size is 'sz' bytes, but '64' bytes may be read.
#pragma warning(push)
#pragma warning(disable: 6385)
			LPWSTR pString = info[i].pName;
			printers.push_back(pString);
#pragma warning(pop)
		}
		free(pInfo);
	}
	return printers;
}

CPrinterInfo::CPrinterInfo() :
	m_hPrinter(INVALID_HANDLE_VALUE),
	m_pPrinterJobs(nullptr),
	m_uuid{},
	m_duuid{},
	m_bCollate(false),
	m_bColorDevice(false),
	m_uiNumCopies(0),
	m_uiDriver(0),
	m_bDuplex(false),
	m_uiExtraDriver(0),
	m_uiFields(0),
	m_minextent(0, 0),
	m_maxextent(0, 0),
	m_uiOrientation(0),
	m_uiPrinterMemInKB(0),
	m_uiPrintRate(0),
	m_uiPrintRatePagePerMinute(0),
	m_uiPrintRateUnit(0),
	m_uiDevModeSize(0),
	m_bStaple(false),
	m_uiTrueTypeBitmask(0),
	m_uiVersion(0),
	m_uiForm1Count(0),
	m_uiForm2Count(0)
{
}

CPrinterInfo::~CPrinterInfo()
{
	Uninit();
}

template<typename T>
void CPrinterInfo::GetProperty(const int32_t i_iProperty, T& o_property)
{
	o_property.clear();
}

template<>
void CPrinterInfo::GetProperty(const int32_t i_iProperty, std::vector<std::wstring>& o_properties)
{
	o_properties.clear();

	uint32_t uiNumChars = 0;
	switch (i_iProperty)
	{
	case DC_BINNAMES:
		uiNumChars = 24;
		break;

	case DC_FILEDEPENDENCIES:
	case DC_MEDIAREADY:
	case DC_MEDIATYPENAMES:
	case DC_PAPERNAMES:
		uiNumChars = 64;
		break;

	case DC_PERSONALITY:
		uiNumChars = 32;
		break;
	}

	if (uiNumChars == 0)
	{
		return;
	}

	uint32_t uiNumProperties = DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, nullptr, nullptr);
	if ((uiNumProperties != -1) && (uiNumProperties != 0))
	{
		wchar_t* pBuffer = static_cast<wchar_t*>(malloc(uiNumChars * sizeof(wchar_t) * uiNumProperties));
		if (pBuffer)
		{
			if (DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, pBuffer, nullptr) == uiNumProperties)
			{
				for (uint32_t p = 0; p < uiNumProperties; p++)
				{
					//Suppress C6385 intellisense warning
					wchar_t* pString = &pBuffer[p * uiNumChars];
					std::wstring prop(pString, uiNumChars);
					o_properties.push_back(prop);
				}
			}
			free(pBuffer);
		}
	}
}

template<>
void CPrinterInfo::GetProperty(const int32_t i_iProperty, std::vector<uint32_t>& o_properties)
{
	o_properties.clear();

	size_t uiNumBytes = 0;
	switch (i_iProperty)
	{
	case DC_BINS:
	case DC_PAPERS:
		uiNumBytes = sizeof(uint16_t);
		break;

	case DC_ENUMRESOLUTIONS:
	case DC_MEDIATYPES:
	case DC_NUP:
		uiNumBytes = sizeof(uint32_t);
		break;
	}

	if (uiNumBytes == 0)
	{
		return;
	}

	uint32_t uiNumProperties = DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, nullptr, nullptr);
	if ((uiNumProperties != -1) && (uiNumProperties != 0))
	{
		wchar_t* pBuffer = static_cast<wchar_t*>(malloc(uiNumBytes * uiNumProperties));
		if (pBuffer)
		{
			if (DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, pBuffer, nullptr) == uiNumProperties)
			{
				if (uiNumBytes == sizeof(uint16_t))
				{
					uint16_t* pData = reinterpret_cast<uint16_t*>(pBuffer);
					for (uint32_t p = 0; p < uiNumProperties; p++)
					{
						o_properties.push_back(*pData);
						pData++;
					}
				}
				else
				{
					uint32_t* pData = reinterpret_cast<uint32_t*>(pBuffer);
					for (uint32_t p = 0; p < uiNumProperties; p++)
					{
						o_properties.push_back(*pData);
						pData++;
					}
				}
			}
			free(pBuffer);
		}
	}
}

template<>
void CPrinterInfo::GetProperty(const int32_t i_iProperty, std::vector< std::pair<int32_t, int32_t> >& o_properties)
{
	o_properties.clear();

	size_t uiNumBytes = 0;
	switch (i_iProperty)
	{
	case DC_ENUMRESOLUTIONS:
	case DC_PAPERSIZE:
		uiNumBytes = 2 * sizeof(int32_t);
		break;
	}

	if (uiNumBytes == 0)
	{
		return;
	}

	uint32_t uiNumProperties = DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, nullptr, nullptr);
	if ((uiNumProperties != -1) && (uiNumProperties != 0))
	{
		wchar_t* pBuffer = static_cast<wchar_t*>(malloc(uiNumBytes * uiNumProperties));
		if (pBuffer)
		{
			if (DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, pBuffer, nullptr) == uiNumProperties)
			{
				int32_t* pValues = reinterpret_cast<int32_t*>(pBuffer);
				for (uint32_t p = 0; p < uiNumProperties; p++)
				{
					o_properties.push_back(std::make_pair(*pValues, *(pValues + 1)));
					
					pValues += 2;
				}
			}
			free(pBuffer);
		}
	}
}

template<>
void CPrinterInfo::GetProperty(const int32_t i_iProperty, std::wstring& o_property)
{
	o_property.clear();

	switch (i_iProperty)
	{
	case DC_MANUFACTURER:
	case DC_MODEL:
		break;

	default:
		return;
	}

	uint32_t uiNumChars = DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, nullptr, nullptr);
	if ((uiNumChars != -1) && (uiNumChars != 0))
	{
		wchar_t* pBuffer = static_cast<wchar_t*>(malloc(uiNumChars * sizeof(wchar_t)));
		if (pBuffer)
		{
			if (DeviceCapabilities(m_printername.c_str(), nullptr, i_iProperty, pBuffer, nullptr) == uiNumChars)
			{
				std::wstring prop(pBuffer, uiNumChars);
				o_property = prop;
			}
			free(pBuffer);
		}
	}
}

void* CPrinterInfo::GetPrinterInfo(HANDLE i_hPrinter, const int32_t i_iLevel)
{
	void* pInfo = nullptr;
	DWORD cbNeeded = 0;
	GetPrinter(i_hPrinter, i_iLevel, nullptr, 0, &cbNeeded);
	if (cbNeeded)
	{
		pInfo = malloc(cbNeeded);
		if (pInfo != nullptr)
		{
			if (!GetPrinter(i_hPrinter, i_iLevel, (LPBYTE)pInfo, cbNeeded, &cbNeeded))
			{
				free(pInfo);
				pInfo = nullptr;
			}
		}
	}
	return pInfo;

/*
	DWORD cnt = 0;
	DWORD sz = 0;
	DWORD Level = 2;
	int i;
	int sl;

	void* pInfo = nullptr;
	if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, i_iLevel, NULL, 0, &sz, &cnt))
	{
		pInfo = malloc(sz);
		if (pInfo != nullptr)
		{
			if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, i_iLevel, (LPBYTE)pInfo, sz, &sz, &cnt))
			{
				free(pInfo);
				pInfo = nullptr;
			}
		}
	}
	return pInfo;
*/
}

void* CPrinterInfo::GetDriverInfo(HANDLE i_hPrinter, const int32_t i_iLevel)
{
	void* pInfo = nullptr;
	DWORD cbNeeded = 0;
	GetPrinterDriver(i_hPrinter, nullptr, i_iLevel, nullptr, 0, &cbNeeded);
	if (cbNeeded)
	{
		pInfo = malloc(cbNeeded);
		if (pInfo != nullptr)
		{
			if (!GetPrinterDriver(i_hPrinter, nullptr, i_iLevel, (LPBYTE)pInfo, cbNeeded, &cbNeeded))
			{
				free(pInfo);
				pInfo = nullptr;
			}
		}
	}
	return pInfo;
}

void* CPrinterInfo::GetFormInfo(HANDLE i_hPrinter, const int32_t i_iLevel, uint32_t& o_uiFormCount)
{
	o_uiFormCount = 0;

	void* pInfo = nullptr;
	DWORD cbCount = 0;
	DWORD cbNeeded = 0;
	EnumForms(i_hPrinter, i_iLevel, nullptr, 0, &cbNeeded, &cbCount);
	if (cbNeeded)
	{
		pInfo = malloc(cbNeeded);
		if (pInfo != nullptr)
		{
			if (!EnumForms(i_hPrinter, i_iLevel, (LPBYTE)pInfo, cbNeeded, &cbNeeded, &cbCount))
			{
				free(pInfo);
				pInfo = nullptr;
			}
		}
	}
	o_uiFormCount = pInfo ? cbCount : 0;
	return pInfo;
}

void* CPrinterInfo::GetMonitorInfo(HANDLE i_hPrinter, const int32_t i_iLevel)
{
	return nullptr;
}

bool CPrinterInfo::Init(const wchar_t* i_pPrinterName)
{
	if (i_pPrinterName == nullptr)
	{
		ELOG("Unknown printer");
		return false;
	}
	
	m_pPrinterJobs = CPrinterJobs::Create();
	if (m_pPrinterJobs == nullptr)
	{
		ELOG("Error initializing printer jobs queue for printer %ls", i_pPrinterName);
		return false;
	}

	m_printername = i_pPrinterName;

	GetProperty(DC_PAPERS, m_papers);
	GetProperty(DC_PAPERSIZE, m_papersize);
	GetProperty(DC_BINS, m_bins);
	GetProperty(DC_BINNAMES, m_binnames);
	GetProperty(DC_ENUMRESOLUTIONS, m_resolutionsInDpi);
	GetProperty(DC_FILEDEPENDENCIES, m_filedependencies);
	GetProperty(DC_PAPERNAMES, m_papernames);
	GetProperty(DC_PERSONALITY, m_personality);
	GetProperty(DC_MEDIAREADY, m_mediaready);
	GetProperty(DC_NUP, m_numberofpagespersheet);
	GetProperty(DC_MEDIATYPENAMES, m_mediatypenames);
	GetProperty(DC_MEDIATYPES, m_mediatypes);
	GetProperty(DC_MANUFACTURER, m_manufacturer);
	GetProperty(DC_MODEL, m_model);
	
	m_bDuplex = DeviceCapabilities(i_pPrinterName, nullptr, DC_DUPLEX, nullptr, nullptr) != 0;
	m_bCollate = DeviceCapabilities(i_pPrinterName, nullptr, DC_COLLATE, nullptr, nullptr) != 0;
	m_bStaple = DeviceCapabilities(i_pPrinterName, nullptr, DC_STAPLE, nullptr, nullptr) != 0;
	m_bColorDevice = DeviceCapabilities(i_pPrinterName, nullptr, DC_COLORDEVICE, nullptr, nullptr) != 0;

	m_uiFields = DeviceCapabilities(i_pPrinterName, nullptr, DC_FIELDS, nullptr, nullptr);
	m_uiDevModeSize = DeviceCapabilities(i_pPrinterName, nullptr, DC_SIZE, nullptr, nullptr);
	m_uiExtraDriver = DeviceCapabilities(i_pPrinterName, nullptr, DC_EXTRA, nullptr, nullptr);
	m_uiVersion = DeviceCapabilities(i_pPrinterName, nullptr, DC_VERSION, nullptr, nullptr);
	m_uiDriver = DeviceCapabilities(i_pPrinterName, nullptr, DC_DRIVER, nullptr, nullptr);
	m_uiTrueTypeBitmask = DeviceCapabilities(i_pPrinterName, nullptr, DC_TRUETYPE, nullptr, nullptr);
	m_uiOrientation = DeviceCapabilities(i_pPrinterName, nullptr, DC_ORIENTATION, nullptr, nullptr);
	m_uiNumCopies = DeviceCapabilities(i_pPrinterName, nullptr, DC_COPIES, nullptr, nullptr);
	m_uiPrintRate = DeviceCapabilities(i_pPrinterName, nullptr, DC_PRINTRATE, nullptr, nullptr);
	m_uiPrintRateUnit = DeviceCapabilities(i_pPrinterName, nullptr, DC_PRINTRATEUNIT, nullptr, nullptr);
	m_uiPrinterMemInKB = DeviceCapabilities(i_pPrinterName, nullptr, DC_PRINTERMEM, nullptr, nullptr);
	m_uiPrintRatePagePerMinute = DeviceCapabilities(i_pPrinterName, nullptr, DC_PRINTRATEPPM, nullptr, nullptr);

	//Old: Win95
	//m_uiBinAdjust = DeviceCapabilities(i_pPrinterName, nullptr, DC_BINADJUST, nullptr, nullptr);
	//DC_EMF_COMPLIANT
	//DC_DATATYPE_PRODUCED

	uint32_t uiMinExtent = DeviceCapabilities(i_pPrinterName, nullptr, DC_MINEXTENT, nullptr, nullptr);
	uint32_t uiMaxExtent = DeviceCapabilities(i_pPrinterName, nullptr, DC_MAXEXTENT, nullptr, nullptr);
	m_minextent = std::make_pair(uiMinExtent & 0xFFFF, uiMinExtent >> 16);
	m_maxextent = std::make_pair(uiMaxExtent & 0xFFFF, uiMaxExtent >> 16);

	DWORD cbNeeded = 0;
	DWORD cbReturned = 0;
	{
		cbNeeded = sizeof(m_printer1);
		cbReturned = 0;
		//EnumPrintersW(PRINTER_ENUM_NAME, (LPWSTR)i_pPrinterName, 1, (LPBYTE)&m_printer1, sizeof(m_printer1), &cbNeeded, &cbReturned);
	}

	bool res = false;
	if (OpenPrinter((LPWSTR)i_pPrinterName, &m_hPrinter, nullptr))
	{
		res = true;

		m_printer1 = static_cast<PRINTER_INFO_1*>(GetPrinterInfo(m_hPrinter, 1));
		m_printer2 = static_cast<PRINTER_INFO_2*>(GetPrinterInfo(m_hPrinter, 2));
		m_printer3 = static_cast<PRINTER_INFO_3*>(GetPrinterInfo(m_hPrinter, 3));
		m_printer4 = static_cast<PRINTER_INFO_4*>(GetPrinterInfo(m_hPrinter, 4));
		m_printer5 = static_cast<PRINTER_INFO_5*>(GetPrinterInfo(m_hPrinter, 5));
		m_printer6 = static_cast<PRINTER_INFO_6*>(GetPrinterInfo(m_hPrinter, 6));
		m_printer7 = static_cast<PRINTER_INFO_7*>(GetPrinterInfo(m_hPrinter, 7));
		m_printer8 = static_cast<PRINTER_INFO_8*>(GetPrinterInfo(m_hPrinter, 8));
		m_printer9 = static_cast<PRINTER_INFO_9*>(GetPrinterInfo(m_hPrinter, 9));

		m_driver1 = static_cast<DRIVER_INFO_1*>(GetDriverInfo(m_hPrinter, 1));
		m_driver2 = static_cast<DRIVER_INFO_2*>(GetDriverInfo(m_hPrinter, 2));
		m_driver3 = static_cast<DRIVER_INFO_3*>(GetDriverInfo(m_hPrinter, 3));
		m_driver4 = static_cast<DRIVER_INFO_4*>(GetDriverInfo(m_hPrinter, 4));
		m_driver5 = static_cast<DRIVER_INFO_5*>(GetDriverInfo(m_hPrinter, 5));
		m_driver6 = static_cast<DRIVER_INFO_6*>(GetDriverInfo(m_hPrinter, 6));
		m_driver8 = static_cast<DRIVER_INFO_8*>(GetDriverInfo(m_hPrinter, 8));

		m_form1 = static_cast<FORM_INFO_1*>(GetFormInfo(m_hPrinter, 1, m_uiForm1Count));
		m_form2 = static_cast<FORM_INFO_2*>(GetFormInfo(m_hPrinter, 2, m_uiForm2Count));
	
	//	m_monitor1 = static_cast<MONITOR_INFO_1*>(GetPrinterMonitorInfo(m_hPrinter, 1, m_uiForm1Count));
	//	m_monitor2 = static_cast<MONITOR_INFO_2*>(GetPrinterMonitorInfo(m_hPrinter, 2, m_uiForm1Count));

	/*
		DWORD res = 0;
		DWORD dwIndex = 0;

		DWORD cbValueName = 0;
		DWORD cbData = 0;

		res = EnumPrinterData(
			m_hPrinter,
			0,
			nullptr,
			0,
			&cbValueName,
			nullptr,
			nullptr,
			0,
			&cbData
		);

		wchar_t* pValueName = (wchar_t*)malloc(cbValueName);
		BYTE* pData = (BYTE*)malloc(cbData);
		DWORD cbValueNameBuf = cbValueName;
		DWORD cbDataBuf = cbData;
		do
		{
			DWORD Type = 0;
			res = EnumPrinterData(
				m_hPrinter,
				dwIndex,
				pValueName,
				cbValueNameBuf,
				&cbValueName,
				&Type,
				pData,
				cbDataBuf,
				&cbData
			);
			dwIndex++;

			if (res == ERROR_SUCCESS)
			{
				if (_strcmp(pValueName, "Model")==0)
				{
					if (m_model.empty())
					{
						m_model = reinterpret_cast<wchar_t*>(pData);
					}
				}
			}

		} while (res == ERROR_SUCCESS);

		free(pData);
		free(pValueName);
*/

		ClosePrinter(m_hPrinter);
	}

#define UPDATE_PRINTER_STRING(field, structname, var)	\
if (field.empty() && structname && structname->var && structname->var[0])	\
{	\
	field = structname->var;	\
}

#define UPDATE_PRINTER_STRING2(field, structname, var, structname2, var2)	\
	UPDATE_PRINTER_STRING(field, structname, var) else UPDATE_PRINTER_STRING(field, structname2, var2)

#define UPDATE_PRINTER_STRING3(field, structname, var, structname2, var2, structname3, var3)	\
	UPDATE_PRINTER_STRING2(field, structname, var, structname2, var2) else UPDATE_PRINTER_STRING(field, structname3, var3)

	UPDATE_PRINTER_STRING(m_model, m_driver2, pName);
	UPDATE_PRINTER_STRING3(m_printerinfo, m_printer1, pComment, m_printer1, pDescription, m_driver2, pName);
	UPDATE_PRINTER_STRING3(m_printerlocation, m_printer2, pLocation, m_printer2, pPrinterName, m_printer2, pServerName);
	UPDATE_PRINTER_STRING(m_manufacturer, m_driver6, pszMfgName);
	UPDATE_PRINTER_STRING(m_url, m_driver6, pszOEMUrl);
	UPDATE_PRINTER_STRING(m_hardwareid, m_driver6, pszHardwareID);

#undef UPDATE_PRINTER_STRING3
#undef UPDATE_PRINTER_STRING2
#undef UPDATE_PRINTER_STRING

	GenerateUniquePrinterNameAndGuids();

	m_commandset = L"application/pdf,application/vnd.ms-xpsdocument"; //TODO

	// This string shouldn't be longer than 255 chars.
	m_deviceid = L"MFG:" + m_manufacturer + L";";
	m_deviceid += L"MDL:" + m_model + L";";
	m_deviceid += L"DES:" + m_printerinfo + L";";
	m_deviceid += L"CLS:Printer;";
	// TODO: Use CID values from https://learn.microsoft.com/en-us/windows-hardware/drivers/print/v4-driver-setup-concepts?
	m_deviceid += L"CID:MyPrinter;";	//TODO
	m_deviceid += L"CMD:" + m_commandset + L";";

	return res;
}


void CPrinterInfo::Uninit()
{
	if (m_hPrinter != INVALID_HANDLE_VALUE)
	{
		ClosePrinter(m_hPrinter);
		m_hPrinter = INVALID_HANDLE_VALUE;
	}

	if (m_pPrinterJobs != nullptr)
	{
		m_pPrinterJobs->Release();
		m_pPrinterJobs = nullptr;
	}

	m_printername.clear();
	m_manufacturer.clear();
	m_model.clear();
	m_modelnumber.clear();
	m_url.clear();
	m_printerinfo.clear();
	m_printerlocation.clear();
	m_deviceid.clear();
	m_hardwareid.clear();
	m_commandset.clear();
	m_uniqueprintername.clear();
	m_uuid = {};
	m_duuid = {};

	m_binnames.clear();
	m_bins.clear();
	m_bCollate = false;
	m_bColorDevice = false;
	m_uiNumCopies = 0;
	m_uiDriver = 0;
	m_bDuplex = false;
	m_resolutionsInDpi.clear();
	m_uiExtraDriver = 0;
	m_uiFields = 0;
	m_filedependencies.clear();
	m_minextent = std::make_pair<int32_t, int32_t>(0, 0);
	m_maxextent = std::make_pair<int32_t, int32_t>(0, 0);
	m_mediaready.clear();
	m_mediatypenames.clear();
	m_mediatypes.clear();
	m_uiOrientation = 0;
	m_numberofpagespersheet.clear();
	m_papernames.clear();
	m_papers.clear();
	m_papersize.clear();
	m_personality.clear();
	m_uiPrinterMemInKB = 0;
	m_uiPrintRate = 0;
	m_uiPrintRatePagePerMinute = 0;
	m_uiPrintRateUnit = 0;
	m_uiDevModeSize = 0;
	m_bStaple = false;
	m_uiTrueTypeBitmask = 0;
	m_uiVersion = 0;
	m_uiForm1Count = 0;
	m_uiForm2Count = 0;

	m_printer1.free();
	m_printer2.free();
	m_printer3.free();
	m_printer4.free();
	m_printer5.free();
	m_printer6.free();
	m_printer7.free();
	m_printer8.free();
	m_printer9.free();

	m_driver1.free();
	m_driver2.free();
	m_driver3.free();
	m_driver4.free();
	m_driver5.free();
	m_driver6.free();
	m_driver8.free();

	m_form1.free();
	m_form2.free();

	//m_monitor1.free();
	//m_monitor2.free();
}

void CPrinterInfo::GenerateUniquePrinterNameAndGuids()
{
	m_uniqueprintername.clear();

	wchar_t ComputerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerName(ComputerName, &dwSize))
	{
		m_uniqueprintername += ComputerName;
		m_uniqueprintername += L"\\";
	}
	m_uniqueprintername += L"MobilePrinter";
	if (m_printername.size())
	{
		m_uniqueprintername += L"\\";

		// Clean-up printer name; however this code preserves printer names such as "\\\\\\\\".
		std::wstring_view printername(m_printername);
		if (const size_t slashPos = printername.find_first_not_of(L'\\'); slashPos != std::wstring_view::npos)
		{
			printername.remove_prefix(slashPos);
		}
		m_uniqueprintername += printername;
	}

	m_uuid = CreateGuid(m_uniqueprintername.c_str());
	m_duuid = CreateGuid(m_printername.c_str());

	m_urnuuidaddress = L"urn:uuid:" + GuidToString(m_uuid);
}
