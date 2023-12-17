#pragma once

#include "CommonUtils.h"
#include <string>
#include <vector>

class CPrinterJobs;

class CPrinterInfo : public CUnknown<CPrinterInfo>
{
public:
	static CPrinterInfo* Create(const wchar_t* pPrinterName);
	static std::vector<std::wstring> EnumeratePrinters();

	virtual ~CPrinterInfo();

	CPrinterJobs* GetPrinterJobs() const { return m_pPrinterJobs; }

	const std::wstring& GetPrinterName() const { return m_printername; }
	const std::wstring& GetManufacturer() const { return m_manufacturer; }
	const std::wstring& GetModel() const { return m_model; }
	const std::wstring& GetModelNumber() const { return m_modelnumber; }
	const std::wstring& GetUrl() const { return m_url; }
	const std::wstring& GetPrinterInfo() const { return m_printerinfo; }
	const std::wstring& GetPrinterLocation() const { return m_printerlocation; }
	const std::wstring& GetDeviceId() const { return m_deviceid; }
	const std::wstring& GetHardwareId() const { return m_hardwareid; }
	const std::wstring& GetCommandSet() const { return m_commandset; }
	const std::wstring& GetUniquePrinterName() const { return m_uniqueprintername; }
	const std::wstring& GetURNUUIDAddress() const { return m_urnuuidaddress; }
	const GUID& GetUUID() const { return m_uuid; }
	const GUID& GetDUUID() const { return m_duuid; }

	LONG GetPrintRatePPM() const { return (m_uiPrintRatePagePerMinute != UINT_MAX) ? static_cast<LONG>(m_uiPrintRatePagePerMinute) : 1;	}

	uint32_t GetMinNumCopies() const { return 1; }
	uint32_t GetMaxNumCopies() const { return (m_uiNumCopies && (m_uiNumCopies != UINT_MAX)) ? static_cast<LONG>(m_uiNumCopies) : 1; }

	bool ColorSupported() const { return m_bColorDevice; }
	bool DuplexSupported() const { return m_bDuplex; }

private:
	CPrinterInfo();

	template<typename T>
	void GetProperty(const int32_t i_iProperty, T& o_property);

	void* GetPrinterInfo(HANDLE i_hPrinter, const int32_t i_iLevel);
	void* GetDriverInfo(HANDLE i_hPrinter, const int32_t i_iLevel);
	void* GetFormInfo(HANDLE i_hPrinter, const int32_t i_iLevel, uint32_t& o_uiFormCount);
	void* GetMonitorInfo(HANDLE i_hPrinter, const int32_t i_iLevel);

	bool Init(const wchar_t* pPrinterName);
	void Uninit();

	void GenerateUniquePrinterNameAndGuids();

private:

	HANDLE m_hPrinter;
	CPrinterJobs* m_pPrinterJobs;

	std::wstring m_printername;
	std::wstring m_manufacturer;
	std::wstring m_model;
	std::wstring m_modelnumber;
	std::wstring m_url;
	std::wstring m_printerinfo;
	std::wstring m_printerlocation;
	std::wstring m_deviceid;
	std::wstring m_hardwareid;
	std::wstring m_commandset;
	std::wstring m_uniqueprintername;
	std::wstring m_urnuuidaddress;
	GUID m_uuid;
	GUID m_duuid;

	std::vector<std::wstring> m_binnames;
	std::vector<uint32_t> m_bins;
	bool m_bCollate;
	bool m_bColorDevice;
	uint32_t m_uiNumCopies;
	uint32_t m_uiDriver;
	bool m_bDuplex;
	std::vector< std::pair<int32_t, int32_t> > m_resolutionsInDpi;
	uint32_t m_uiExtraDriver;
	uint32_t m_uiFields;
	std::vector<std::wstring> m_filedependencies;
	std::pair<int32_t, int32_t> m_minextent;
	std::pair<int32_t, int32_t> m_maxextent;
	std::vector<std::wstring> m_mediaready;
	std::vector<std::wstring> m_mediatypenames;
	std::vector<uint32_t> m_mediatypes;
	uint32_t m_uiOrientation;
	std::vector<uint32_t> m_numberofpagespersheet;
	std::vector<std::wstring> m_papernames;
	std::vector<uint32_t> m_papers;
	std::vector< std::pair<int32_t, int32_t> > m_papersize;
	std::vector<std::wstring> m_personality;
	uint32_t m_uiPrinterMemInKB;
	uint32_t m_uiPrintRate;
	uint32_t m_uiPrintRatePagePerMinute;
	uint32_t m_uiPrintRateUnit;
	uint32_t m_uiDevModeSize;
	bool m_bStaple;
	uint32_t m_uiTrueTypeBitmask;
	uint32_t m_uiVersion;

	template<typename T>
	class auto_free_ptr
	{
	public:
		auto_free_ptr() : ptr(nullptr) {}
		~auto_free_ptr() { free(); }

		auto_free_ptr<T>& operator=(T* i_ptr) { free(); ptr = i_ptr; return (*this); }
		T* operator->() const { return ptr; }
		explicit operator bool() const { return (ptr != nullptr); }

		void free() { ::free(ptr); ptr = nullptr; }

	private:
		T* ptr;
	};
		

	auto_free_ptr<PRINTER_INFO_1> m_printer1;
	auto_free_ptr<PRINTER_INFO_2> m_printer2;
	auto_free_ptr<PRINTER_INFO_3> m_printer3;
	auto_free_ptr<PRINTER_INFO_4> m_printer4;
	auto_free_ptr<PRINTER_INFO_5> m_printer5;
	auto_free_ptr<PRINTER_INFO_6> m_printer6;
	auto_free_ptr<PRINTER_INFO_7> m_printer7;
	auto_free_ptr<PRINTER_INFO_8> m_printer8;
	auto_free_ptr<PRINTER_INFO_9> m_printer9;

	auto_free_ptr<DRIVER_INFO_1> m_driver1;
	auto_free_ptr<DRIVER_INFO_2> m_driver2;
	auto_free_ptr<DRIVER_INFO_3> m_driver3;
	auto_free_ptr<DRIVER_INFO_4> m_driver4;
	auto_free_ptr<DRIVER_INFO_5> m_driver5;
	auto_free_ptr<DRIVER_INFO_6> m_driver6;
	auto_free_ptr<DRIVER_INFO_8> m_driver8;

	auto_free_ptr<FORM_INFO_1> m_form1;
	auto_free_ptr<FORM_INFO_2> m_form2;

	//auto_free_ptr<MONITOR_INFO_1> m_monitor1;
	//auto_free_ptr<MONITOR_INFO_2> m_monitor2;

	uint32_t m_uiForm1Count;
	uint32_t m_uiForm2Count;
};

