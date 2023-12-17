#pragma once

#include <thread>
#include <string>
#include <map>
#include <mutex>
#include <memory>
#include "CommonUtils.h"

class PrinterJob
{
public:

	PrinterJob();
	~PrinterJob();

	int32_t GetJobId() const
	{
		return JobId;
	}
	
	const std::wstring& GetJobNameW() const
	{
		return JobNameW;
	}

	const std::string& GetJobName() const
	{
		return JobName;
	}

	const std::wstring& GetJobOriginatingUserNameW() const
	{
		return JobOriginatingUserNameW;
	}

	const std::string& GetJobOriginatingUserName() const
	{
		return JobOriginatingUserName;
	}

	const std::wstring& GetPrinterNameW() const
	{
		return PrinterNameW;
	}

	int32_t GetBytesProcessed() const
	{
		return BytesProcessed;
	}

	int32_t GetOctetsProcessed() const
	{
		return (BytesProcessed + 1023) / 1024;
	}

	int32_t GetMediaSheetsCompleted() const
	{
		return MediaSheetsCompleted;
	}

	int32_t GetNumberOfDocuments() const
	{
		return NumberOfDocuments;
	}

	const wchar_t* GetStatusWSD() const
	{
		return StatusWSD;
	}

	int32_t GetStatusIPP() const
	{
		return StatusIPP;
	}

	const wchar_t* GetReasonWSD() const
	{
		return ReasonWSD;
	}

	const char* GetReasonIPP() const
	{
		return ReasonIPP;
	}

	bool IsCompleted() const
	{
		return Completed;
	}

private:

	friend class CPrinterJobs;

	int32_t JobId;
	std::wstring JobNameW;
	std::wstring JobOriginatingUserNameW;
	std::string JobName;
	std::string JobOriginatingUserName;

	std::wstring PrinterNameW;

	int32_t BytesProcessed;
	int32_t MediaSheetsCompleted;
	int32_t NumberOfDocuments;

	const wchar_t* StatusWSD;
	const wchar_t* ReasonWSD;
	const char* ReasonIPP;
	int32_t StatusIPP;

	bool Completed;
};

struct IWSDAttachment;
class PrinterSourceStream;

class CPrinterJobs : public CUnknown<CPrinterJobs>
{
public:

	static CPrinterJobs* Create();

	static void Config(const bool i_bAutoDetectUnknownDocumentFormat, const bool i_bPrintToRawPrinter, const bool i_bForceSaveToFile);

	std::shared_ptr<const PrinterJob> CreatePrinterJob(const std::wstring& i_PrinterName, const char* i_JobName, const char* i_JobOriginatingUserName, std::function<void(const PrinterJob&)> SendJobStatus);
	std::shared_ptr<const PrinterJob> CreatePrinterJob(const std::wstring& i_PrinterName, const wchar_t* i_JobName, const wchar_t* i_JobOriginatingUserName, std::function<void(const PrinterJob&)> SendJobStatus);

	std::shared_ptr<const PrinterJob> FindJob(const int32_t i_JobId);
	std::shared_ptr<const PrinterJob> PrintJob(const int32_t i_JobId, PrinterSourceStream* i_DocumentData, const wchar_t* i_DocumentFormat);
	std::shared_ptr<const PrinterJob> StartJob(const int32_t i_JobId);
	std::shared_ptr<const PrinterJob> FinishJob(const int32_t i_JobId);
	std::shared_ptr<const PrinterJob> CancelJob(const int32_t i_JobId);
	std::shared_ptr<const PrinterJob> SetJobStatus(const int32_t i_JobId, const wchar_t* i_Status, const wchar_t* i_Reason, const int32_t i_TotalBytesProcessed);

	int32_t GetNumActiveJobs();
	std::vector<std::shared_ptr<const PrinterJob>> GetAllJobs();
	std::vector<std::shared_ptr<const PrinterJob>> GetActiveJobs();
	std::vector<std::shared_ptr<const PrinterJob>> GetJobsHistory();

	void SetEventRate(const int32_t i_EventRate);
	int32_t GetEventRate();

	//Helpers
	std::shared_ptr<const PrinterJob> PrintJob(const int32_t i_JobId, IWSDAttachment* i_DocumentData, const wchar_t* i_DocumentFormat);
	std::shared_ptr<const PrinterJob> PrintJob(const int32_t i_JobId, std::vector<uint8_t>&& i_DocumentData, const wchar_t* i_DocumentFormat);
	std::shared_ptr<const PrinterJob> PrintJob(const int32_t i_JobId, const std::vector<uint8_t>& i_DocumentData, bool i_bCopyData, const wchar_t* i_DocumentFormat);
	std::shared_ptr<const PrinterJob> PrintJob(const int32_t i_JobId, const uint8_t* i_DocumentData, const size_t i_DocumentSize, bool i_bCopyData, const wchar_t* i_DocumentFormat);

private:

	class PrinterJobData;
	class PrintJobAsyncThread;

	CPrinterJobs();
	~CPrinterJobs();

	bool SetJobStatus(std::shared_ptr<PrinterJobData> i_Job, const wchar_t* i_Status, const wchar_t* i_Reason, const int32_t i_TotalBytesProcessed);
	
private:

	int32_t m_NextJobId;
	int32_t m_EventRate;

	std::map<int32_t, std::shared_ptr<PrinterJobData>> m_jobs;
	std::map<int32_t, std::shared_ptr<const PrinterJob>> m_jobsHistory;
	std::recursive_mutex m_mutex;
};
