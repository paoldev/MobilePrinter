#include "pch.h"
#include "PrinterJobs.h"
#include "CommonUtils.h"
#include "wsd/src_host/MyPrinterTypes.h"
#include "ipp/ipp_types.h"
#include "PrinterHelpers.h"
#include "PrinterInterface.h"
#include "PrintDocumentPdf.h"
#include "PrinterSourceStream.h"

#include <algorithm>
#include <filesystem>

//This is not defined for WSPrint (it's mapped to "None"), but it's used by IPP when Cancel-Job is received.
#define JobStateReasonsWKVType_ProcessingToStopPoint L"ProcessingToStopPoint"

struct PrinterJobsConfig
{
	bool m_bAutoDetectUnknownDocumentFormat = true;
	bool m_bPrintToRawPrinter = false;
#ifdef _DEBUG
	bool m_bForceSaveToFile = false;
#endif
};

PrinterJobsConfig s_printerJobsConfig = {};

void CPrinterJobs::Config(const bool i_bAutoDetectUnknownDocumentFormat, const bool i_bPrintToRawPrinter, const bool i_bForceSaveToFile)
{
	s_printerJobsConfig.m_bAutoDetectUnknownDocumentFormat = i_bAutoDetectUnknownDocumentFormat;
	s_printerJobsConfig.m_bPrintToRawPrinter = i_bPrintToRawPrinter;
#ifdef _DEBUG
	s_printerJobsConfig.m_bForceSaveToFile = i_bForceSaveToFile;
#else
	(void)i_bForceSaveToFile;
#endif
}

PrinterJob::PrinterJob() :
	JobId(0),
	BytesProcessed(0),
	MediaSheetsCompleted(0),
	NumberOfDocuments(0),
	StatusWSD(nullptr),
	ReasonWSD(nullptr),
	ReasonIPP(nullptr),
	StatusIPP(0),
	Completed(false)
{
}

PrinterJob::~PrinterJob()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CPrinterJobs::PrinterJobData
{
public:

	PrinterJobData() : StartEvent(nullptr), ExitEvent(nullptr), EventThread(nullptr)
	{ }

	~PrinterJobData()
	{
		shutdown();
	}

	PrinterJobData& operator=(const PrinterJobData&) = delete;

	PrinterJob& GetJobStatus() { return m_job_status; }
	const PrinterJob& GetJobStatus() const { return m_job_status; }

	bool IsCompleted() const { return m_job_status.IsCompleted(); }
	void Cancel() { m_bCanceled = true; };
	bool IsCanceled() const { return m_bCanceled; };

private:

	friend class CPrinterJobs;

	void shutdown()
	{
		if (ExitEvent) ExitEvent->set();
		if (StartEvent) StartEvent->set();
		if (EventThread) EventThread->join();

		delete ExitEvent;
		delete StartEvent;
		delete EventThread;

		ExitEvent = nullptr;
		StartEvent = nullptr;
		EventThread = nullptr;
	}

private:

	PrinterJob m_job_status;

	std::atomic_bool m_bCanceled = false;
	std::function<void(const PrinterJob&)> ProgressStatus;
	myevent* StartEvent;
	myevent* ExitEvent;
	std::thread* EventThread;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CPrinterJobs::PrintJobAsyncThread : public CUnknown<PrintJobAsyncThread>
{
public:
	typedef bool IsCanceledFunc(LONG JobId);
	typedef void ProgressFunc(LONG JobId, const wchar_t* Status, const wchar_t* Reason, size_t TotalByteReads);

	PrintJobAsyncThread();

	HRESULT ReceiveAsync(
		const wchar_t* lpPrinterName, 
		LONG JobId, 
		const wchar_t* DocumentName, 
		const wchar_t* DocumentFormat, 
		PrinterSourceStream* pAttachment,
		std::function< IsCanceledFunc > InCanceledFunc,
		std::function< ProgressFunc > InProgressFunc);

private:

	~PrintJobAsyncThread();

	HRESULT Start();
	HRESULT ThreadProc();
	HRESULT ReceiveAsyncInternal(LONG JobId, const wchar_t* DocumentName, const wchar_t* DocumentFormat, PrinterSourceStream* pAttachment, std::function<IsCanceledFunc> InCanceledFunc, std::function< ProgressFunc > InProgressFunc);
	HRESULT CreatePrinter(_In_ BasePrinter** o_ppPrinter, _In_ const void* pv, _In_ ULONG cb);	//Creates the printer according to DocumentFormat or to the autodetected file format.
	static DWORD WINAPI StaticThreadProc(LPVOID pParams);

private:

	std::function< IsCanceledFunc > IsCanceled;
	std::function< ProgressFunc > Progress;
	PrinterSourceStream* m_pAttachment;

	LONG JobId;
	std::wstring DocumentName;
	std::wstring DocumentFormat;

	std::wstring PrinterName;
	std::filesystem::path FileName;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPrinterJobs* CPrinterJobs::Create()
{
	return new CPrinterJobs();
}

CPrinterJobs::CPrinterJobs() : m_NextJobId(1), m_EventRate(1)
{
}

CPrinterJobs::~CPrinterJobs()
{
}

std::shared_ptr<const PrinterJob> CPrinterJobs::CreatePrinterJob(const std::wstring& i_PrinterName, const char* i_JobName, const char* i_JobOriginatingUserName, std::function<void(const PrinterJob&)> SendJobStatus)
{
	std::wstring JobName = utf8_to_wchar(i_JobName);
	std::wstring JobOriginatingUserName = utf8_to_wchar(i_JobOriginatingUserName);
	return CreatePrinterJob(i_PrinterName, JobName.c_str(), JobOriginatingUserName.c_str(), SendJobStatus);
}

std::shared_ptr<const PrinterJob> CPrinterJobs::CreatePrinterJob(const std::wstring& i_PrinterName, const wchar_t* i_JobName, const wchar_t* i_JobOriginatingUserName, std::function<void(const PrinterJob&)> SendJobStatus)
{
	if (i_JobName == nullptr)
	{
		i_JobName = L"";
	}

	if (i_JobOriginatingUserName == nullptr)
	{
		i_JobOriginatingUserName = L"";
	}

	std::shared_ptr<PrinterJobData> Job = std::make_shared<PrinterJobData>();

	Job->ProgressStatus = SendJobStatus ? SendJobStatus : [](const PrinterJob&) {};

	PrinterJob& JobStatus = Job->GetJobStatus();

	JobStatus.Completed = false;

	JobStatus.PrinterNameW = i_PrinterName;

	JobStatus.NumberOfDocuments = 0;
	JobStatus.BytesProcessed = 0;
	JobStatus.MediaSheetsCompleted = 0;
	JobStatus.StatusWSD = JobStateWKVType_Pending;
	JobStatus.ReasonWSD = JobStateReasonsWKVType_JobIncoming;

	using namespace ipp;
	JobStatus.StatusIPP = ipp::job_state::pending;
	JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobIncoming;

	JobStatus.JobNameW = i_JobName;
	JobStatus.JobOriginatingUserNameW = i_JobOriginatingUserName;
	JobStatus.JobName = wchar_to_utf8(i_JobName);
	JobStatus.JobOriginatingUserName = wchar_to_utf8(i_JobOriginatingUserName);

	VLOG("JobName: %ls", JobStatus.JobNameW.c_str());
	VLOG("JobUser: %ls", JobStatus.JobOriginatingUserNameW.c_str());

	m_mutex.lock();
	int32_t Timeout = m_EventRate;
	int32_t CurrentJobId = m_NextJobId++;
	JobStatus.JobId = CurrentJobId;
	m_jobs[CurrentJobId] = Job;
	m_mutex.unlock();

	auto Status = std::make_shared<const PrinterJob>(Job->GetJobStatus());

	Job->StartEvent = new myevent();
	Job->ExitEvent = new myevent();
	Job->EventThread = new std::thread([&, Timeout, Job]() {

		PrinterJob& JobStatus = Job->GetJobStatus();

		bool bRunning = true;

		//WSPrint: CreatePrintJob has 60 sec of initial timeout.
		if (!Job->StartEvent->wait(60 * 1000))
		{
			m_mutex.lock();
			SetJobStatus(Job, JobStateWKVType_Aborted, JobStateReasonsWKVType_DocumentTimeoutError, 0);
			Job->ProgressStatus(JobStatus);
			m_mutex.unlock();
			bRunning = false;
		}

		if (bRunning)
		{
			do
			{
				m_mutex.lock();
				if (!Job->IsCompleted()) //TEMP
				{
					Job->ProgressStatus(Job->GetJobStatus());
				}
				bRunning = !Job->IsCompleted();

				m_mutex.unlock();
			} while (bRunning && !Job->ExitEvent->wait(Timeout * 1000));
		}

		//FinishJob(JobStatus.GetJobId());
		});

	return Status;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::FindJob(const int32_t i_JobId)
{
	std::shared_ptr<const PrinterJob> Job;

	{
		std::lock_guard lock(m_mutex);

		auto it = m_jobs.find(i_JobId);
		if (it != m_jobs.end())
		{
			Job = std::make_shared<const PrinterJob>(it->second->GetJobStatus());
		}
		else
		{
			auto it2 = m_jobsHistory.find(i_JobId);
			if (it2 != m_jobsHistory.end())
			{
				Job = it2->second;
			}
		}
	}

	return Job;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::StartJob(const int32_t i_JobId)
{
	std::lock_guard lock(m_mutex);

	std::shared_ptr<const PrinterJob> Status;
	auto it = m_jobs.find(i_JobId);
	if (it != m_jobs.end())
	{
		std::shared_ptr<PrinterJobData> Job = it->second;
		Job->GetJobStatus().NumberOfDocuments++;
		Status = std::make_shared<const PrinterJob>(Job->GetJobStatus());
		Job->StartEvent->set();
	}

	return Status;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::FinishJob(const int32_t i_JobId)
{
	std::lock_guard lock(m_mutex);

	std::shared_ptr<const PrinterJob> Status;

	auto it = m_jobs.find(i_JobId);
	if (it == m_jobs.end())
	{
		//return ClientErrorJobIdNotFound;
		auto it2 = m_jobsHistory.find(i_JobId);
		if (it2 != m_jobsHistory.end())
		{
			Status = it2->second;
		}
	}
	else
	{
		Status = std::make_shared<const PrinterJob>(it->second->GetJobStatus());
		m_jobsHistory[it->first] = Status;
		m_jobs.erase(it->first);

		//release resources?
		//Job.shutdown();
	}
	return Status;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::CancelJob(const int32_t i_JobId)
{
	std::lock_guard lock(m_mutex);

	std::shared_ptr<const PrinterJob> Status;
	auto it = m_jobs.find(i_JobId);
	if (it != m_jobs.end())
	{
		std::shared_ptr<PrinterJobData> Job = it->second;
		Job->Cancel();
		Status = std::make_shared<const PrinterJob>(Job->GetJobStatus());
	}

	return Status;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::SetJobStatus(const int32_t i_JobId, const wchar_t* i_Status, const wchar_t* i_Reason, const int32_t i_TotalBytesProcessed)
{
	std::lock_guard lock(m_mutex);

	std::shared_ptr<const PrinterJob> Status;

	auto it = m_jobs.find(i_JobId);
	if (it != m_jobs.end())
	{
		if (SetJobStatus(it->second, i_Status, i_Reason, i_TotalBytesProcessed))
		{
			Status = std::make_shared<const PrinterJob>(it->second->GetJobStatus());
		}
	}

	return Status;
}

bool CPrinterJobs::SetJobStatus(std::shared_ptr<PrinterJobData> i_Job, const wchar_t* i_Status, const wchar_t* i_Reason, const int32_t i_TotalBytesProcessed)
{
	if (i_Job)
	{
		PrinterJob& JobStatus = i_Job->GetJobStatus();

		if (JobStatus.Completed)
		{
			//Don't change status on already completed jobs.
			return true;
		}

		JobStatus.StatusWSD = i_Status;
		JobStatus.ReasonWSD = i_Reason;
		if (i_TotalBytesProcessed >= 0)
		{
			JobStatus.BytesProcessed = i_TotalBytesProcessed;
			JobStatus.MediaSheetsCompleted = 1;	//TODO
		}

		JobStatus.Completed = (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Completed) == 0) ||
			(wcscmp(JobStatus.StatusWSD, JobStateWKVType_Canceled) == 0) || (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Aborted) == 0);

		if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Aborted) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::aborted;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Canceled) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::canceled;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Completed) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::completed;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Pending) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::pending;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Pending_Held) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::pending_held;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Processing) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::processing;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_ProcessingStopped) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::processing_stopped;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Started) == 0)
		{
			JobStatus.StatusIPP = ipp::job_state::processing;
		}
		else if (wcscmp(JobStatus.StatusWSD, JobStateWKVType_Terminating) == 0)
		{
			//leave previous state
			//JobStatus.StatusIPP = ipp::job_state::processing;
		}
		else
		{
			assert(0 && "TODO");
		}

		if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_DocumentTimeoutError) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobHoldUntilSpecified;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_JobIncoming) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobIncoming;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_None) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_None;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_DocumentTransferError) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_DocumentAccessError;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_JobCompletedWithErrors) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobCompletedWithErrors;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_JobCompletedSuccessfully) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobCompletedSuccessfully;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_JobCanceledByUser) == 0)
		{
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_JobCanceledByUser;
		}
		else if (wcscmp(JobStatus.ReasonWSD, JobStateReasonsWKVType_ProcessingToStopPoint) == 0)
		{
			JobStatus.ReasonWSD = JobStateReasonsWKVType_None;	//Reset to "None", because WSPrint doesn't support "ProcessingToStopPoint"
			JobStatus.ReasonIPP = KEYWORD_JobStateReasons_ProcessingToStopPoint;
		}
		else
		{
			assert(0 && "TODO");
		}
	}

	return (i_Job != nullptr);
}

int32_t CPrinterJobs::GetNumActiveJobs()
{
	std::lock_guard lock(m_mutex);
	return static_cast<int32_t>(m_jobs.size());
}

std::vector<std::shared_ptr<const PrinterJob>> CPrinterJobs::GetAllJobs()
{
	std::vector<std::shared_ptr<const PrinterJob>> jobs;
	{
		std::lock_guard lock(m_mutex);

		jobs.reserve(m_jobs.size() + m_jobsHistory.size());
		for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); it++)
		{
			jobs.push_back(std::make_shared<const PrinterJob>(it->second->GetJobStatus()));
		}
		for (auto it = m_jobsHistory.cbegin(); it != m_jobsHistory.cend(); it++)
		{
			jobs.push_back(it->second);
		}
	}

	return jobs;
}

std::vector<std::shared_ptr<const PrinterJob>> CPrinterJobs::GetActiveJobs()
{
	std::vector<std::shared_ptr<const PrinterJob>> jobs;
	{
		std::lock_guard lock(m_mutex);

		jobs.reserve(m_jobs.size());
		for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); it++)
		{
			jobs.push_back(std::make_shared<const PrinterJob>(it->second->GetJobStatus()));
		}
	}

	return jobs;
}

std::vector<std::shared_ptr<const PrinterJob>> CPrinterJobs::GetJobsHistory()
{
	std::vector<std::shared_ptr<const PrinterJob>> jobs;
	{
		std::lock_guard lock(m_mutex);

		jobs.reserve(m_jobsHistory.size());
		for (auto it = m_jobsHistory.cbegin(); it != m_jobsHistory.cend(); it++)
		{
			jobs.push_back(it->second);
		}
	}

	return jobs;
}

void CPrinterJobs::SetEventRate(const int32_t i_EventRate)
{
	std::lock_guard lock(m_mutex);
	m_EventRate = std::max(i_EventRate, 1);
}

int32_t CPrinterJobs::GetEventRate()
{
	std::lock_guard lock(m_mutex);
	return m_EventRate;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::PrintJob(const int32_t i_JobId, PrinterSourceStream* i_DocumentData, const wchar_t* i_DocumentFormat)
{
	std::shared_ptr<PrinterJobData> Job;
	std::shared_ptr<const PrinterJob> JobStatus;

	{
		std::lock_guard lock(m_mutex);

		auto it = m_jobs.find(i_JobId);
		if (it != m_jobs.end())
		{
			Job = it->second;
		}
	}

	if (Job)
	{
		HRESULT hr = S_OK;

		PrintJobAsyncThread* pReadAttachmentThread = new PrintJobAsyncThread();
		if (pReadAttachmentThread != nullptr)
		{
			std::function<PrintJobAsyncThread::IsCanceledFunc> c = [Job](LONG /*JobId*/)
			{
				return Job->IsCanceled();
			};

			std::function<PrintJobAsyncThread::ProgressFunc> p = [&, Job](LONG JobId, const wchar_t* Status, const wchar_t* Reason, size_t TotalByteReads)
			{
				m_mutex.lock();
				SetJobStatus(Job, Status, Reason, static_cast<int32_t>(TotalByteReads));
				m_mutex.unlock();
				DBGLOG("Progress: %d - Bytes %llu - %ls - %ls\n", JobId, static_cast<uint64_t>(TotalByteReads), Status, Reason);
			};

			{
				std::lock_guard lock(m_mutex);
				Job->GetJobStatus().NumberOfDocuments++;
				Job->StartEvent->set();
			}

			//Read attachment and send to printer
			hr = pReadAttachmentThread->ReceiveAsync(Job->GetJobStatus().GetPrinterNameW().c_str(), Job->GetJobStatus().GetJobId(), Job->GetJobStatus().GetJobNameW().c_str(), i_DocumentFormat, i_DocumentData, c, p);

			SAFE_RELEASE(pReadAttachmentThread);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		{
			std::lock_guard lock(m_mutex);

			if (FAILED(hr))
			{
				//Set the new status
				SetJobStatus(Job, JobStateWKVType_Aborted, JobStateReasonsWKVType_DocumentTransferError, -1);
				Job->ProgressStatus(Job->GetJobStatus());
			}

			JobStatus = std::make_shared<const PrinterJob>(Job->GetJobStatus());
		}
	}

	return JobStatus;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::PrintJob(const int32_t i_JobId, IWSDAttachment* i_DocumentData, const wchar_t* i_DocumentFormat)
{
	PrinterSourceStream* pDocumentData = CWSDInboundAttachmentPrinterSourceStream::Create(i_DocumentData);
	auto JobStatus = PrintJob(i_JobId, pDocumentData, i_DocumentFormat);
	SAFE_RELEASE(pDocumentData);
	return JobStatus;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::PrintJob(const int32_t i_JobId, std::vector<uint8_t>&& i_DocumentData, const wchar_t* i_DocumentFormat)
{
	PrinterSourceStream* pDocumentData = CInMemoryPrinterSourceStream::Create(std::move(i_DocumentData));
	auto JobStatus = PrintJob(i_JobId, pDocumentData, i_DocumentFormat);
	SAFE_RELEASE(pDocumentData);
	return JobStatus;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::PrintJob(const int32_t i_JobId, const uint8_t* i_DocumentData, const size_t i_DocumentSize, bool i_bCopyData, const wchar_t* i_DocumentFormat)
{
	PrinterSourceStream* pDocumentData = CInMemoryPrinterSourceStream::Create(i_DocumentData, i_DocumentSize, i_bCopyData);
	auto JobStatus = PrintJob(i_JobId, pDocumentData, i_DocumentFormat);
	SAFE_RELEASE(pDocumentData);
	return JobStatus;
}

std::shared_ptr<const PrinterJob> CPrinterJobs::PrintJob(const int32_t i_JobId, const std::vector<uint8_t>& i_DocumentData, bool i_bCopyData, const wchar_t* i_DocumentFormat)
{
	return PrintJob(i_JobId, i_DocumentData.data(), i_DocumentData.size(), i_bCopyData, i_DocumentFormat);
}

//////////////////////////////////////////////////////////////////////////////
// PrintJobAsyncThread Class
//       Performs worker thread reads an attachment from the client
//////////////////////////////////////////////////////////////////////////////
CPrinterJobs::PrintJobAsyncThread::PrintJobAsyncThread() : m_pAttachment(nullptr), JobId(0)
{
}

CPrinterJobs::PrintJobAsyncThread::~PrintJobAsyncThread()
{
	SAFE_RELEASE(m_pAttachment);
}

HRESULT CPrinterJobs::PrintJobAsyncThread::ReceiveAsync(const wchar_t* lpPrinterName, LONG i_JobId, const wchar_t* i_DocumentName,
	const wchar_t* i_DocumentFormat, PrinterSourceStream* pAttachment, std::function<IsCanceledFunc> InCanceledFunc, std::function<ProgressFunc> InProgressFunc)
{
	HRESULT hr = S_OK;

	if (nullptr == pAttachment)
	{
		return E_INVALIDARG;
	}

	if (nullptr == lpPrinterName)
	{
		return E_INVALIDARG;
	}

	FileName.clear();
	PrinterName = lpPrinterName;

	return ReceiveAsyncInternal(i_JobId, i_DocumentName, i_DocumentFormat, pAttachment, InCanceledFunc, InProgressFunc);
}

HRESULT CPrinterJobs::PrintJobAsyncThread::ReceiveAsyncInternal(LONG i_JobId, const wchar_t* i_DocumentName, const wchar_t* i_DocumentFormat, PrinterSourceStream* pAttachment, std::function<IsCanceledFunc> InCanceledFunc, std::function<ProgressFunc> InProgressFunc)
{
	JobId = i_JobId;
	DocumentName = i_DocumentName ? i_DocumentName : L"";
	DocumentFormat = i_DocumentFormat ? i_DocumentFormat : L"";
	IsCanceled = InCanceledFunc;
	Progress = InProgressFunc;

	HRESULT hr = S_OK;
	m_pAttachment = pAttachment;

	if (m_pAttachment)
	{
		m_pAttachment->AddRef();
	}

	if (SUCCEEDED(hr))
	{
		hr = Start();
	}

	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pAttachment)
	}

	return hr;
}


HRESULT CPrinterJobs::PrintJobAsyncThread::Start()
{
	//
	// Immediately start a new thread to send the attachment
	//
	// Use QueueUserWorkItem here because the attachment threads are created
	// often and die quickly.  Also, the threadpool cap will help keep from
	// spawning too many threads.
	//
#if 1
	//If this function runs in the caller thread, before returning from SendDocument,
	//the file to be printed is correctly received.
	AddRef();
	return StaticThreadProc(this);
#else

	AddRef();
	if (0 == ::QueueUserWorkItem(StaticThreadProc, (LPVOID*)this, WT_EXECUTELONGFUNCTION))
	{
		Release();
		DWORD dwErr = ::GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}

	return S_OK;
#endif
}

DWORD WINAPI CPrinterJobs::PrintJobAsyncThread::StaticThreadProc(LPVOID pParams)
{
	if (nullptr != pParams)
	{
		// Ignore result
		(void)((PrintJobAsyncThread*)pParams)->ThreadProc();
	}
	return 0;
}

HRESULT CPrinterJobs::PrintJobAsyncThread::CreatePrinter(_In_ BasePrinter** o_ppPrinter, _In_ const void* pv, _In_ ULONG cb)
{
	HRESULT hr = S_OK;

	bool bIsPdf = false;
	bool bIsXps = false;
	if (!s_printerJobsConfig.m_bPrintToRawPrinter)
	{
		bIsPdf = IsPdfDocumentFormat(DocumentFormat.c_str());
		bIsXps = !bIsPdf && IsXpsDocumentFormat(DocumentFormat.c_str());
		if (s_printerJobsConfig.m_bAutoDetectUnknownDocumentFormat && !bIsPdf && !bIsXps)
		{
			bIsPdf = IsPdfFile(pv, cb);
			bIsXps = !bIsPdf && IsXpsFile(pv, cb);
		}
	}

	if (SUCCEEDED(hr))
	{
#ifdef _DEBUG
		if (!FileName.empty() || s_printerJobsConfig.m_bForceSaveToFile)
		{
			if (FileName.empty())
			{
				wchar_t wszFileName[MAX_PATH] = {};

				//The file extension will be appended after detecting the correct data format.
				SYSTEMTIME st = {};
				GetLocalTime(&st);
				swprintf_s(wszFileName, L"Print_%04u_%02u_%02u_%02u_%02u_%02u_%03u_J%06u",
					static_cast<uint32_t>(st.wYear),
					static_cast<uint32_t>(st.wMonth),
					static_cast<uint32_t>(st.wDay),
					static_cast<uint32_t>(st.wHour),
					static_cast<uint32_t>(st.wMinute),
					static_cast<uint32_t>(st.wSecond),
					static_cast<uint32_t>(st.wMilliseconds),
					static_cast<uint32_t>(JobId));

				FileName = global_config::get().get_output_folder();
				FileName.append(L"testfiles");
				FileName.append(wszFileName);
			}

			if (!FileName.has_extension())
			{
				const wchar_t* pszExtension = L".dat";
				if (bIsPdf)
				{
					pszExtension = L".pdf";
				}
				else if (bIsXps)
				{
					pszExtension = L".xps";
				}
				FileName.replace_extension(pszExtension);
			}

			hr = PrintToFile::Create(FileName.c_str(), o_ppPrinter);
		}
#else
		if (0) {}
#endif
		else if (PrinterName.size() > 0)
		{
			if (bIsPdf)
			{
				hr = PrintPdf::Create(PrinterName.c_str(), DocumentName.c_str(), o_ppPrinter);
			}
			else if (bIsXps)
			{
				//hr = PrintToXpsPrintApiPrinter::Create(PrinterName.c_str(), DocumentName.c_str(), o_ppPrinter);
				hr = PrintToXpsPrintDocumentPrinter::Create(PrinterName.c_str(), DocumentName.c_str(), o_ppPrinter);
			}
			else
			{
				hr = PrintToRawPrinter::Create(PrinterName.c_str(), DocumentName.c_str(), o_ppPrinter);
			}
		}
		else
		{
			hr = E_UNEXPECTED;
		}
	}

	return hr;
}

HRESULT CPrinterJobs::PrintJobAsyncThread::ThreadProc()
{
	DWORD dwTotalRead = 0;
	DWORD dwTotalWritten = 0;

	HRESULT hr = S_OK;

	if (m_pAttachment == nullptr)
	{
		hr = E_POINTER;
		Progress(JobId, JobStateWKVType_Completed, JobStateReasonsWKVType_JobCompletedWithErrors, 0);
		Release();
		return hr;
	}

	Progress(JobId, JobStateWKVType_Started, JobStateReasonsWKVType_None, dwTotalRead);
	//Progress(JobId, JobStateWKVType_Pending, JobStateReasonsWKVType_JobIncoming, dwTotalRead);

	const DWORD BUFFER_SIZE = 81920;
	std::unique_ptr<BYTE[]> buffer;
	if (SUCCEEDED(hr))
	{
		buffer = std::make_unique<BYTE[]>(BUFFER_SIZE);
		if (buffer.get() == nullptr)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	bool bCanceledByUser = false;
	bool bLastChunk = false;
	BasePrinter* pPrinter = nullptr;
	while (SUCCEEDED(hr) && (!bLastChunk))
	{
		DWORD dwBytesRead = 0;
		DWORD dwBytesWritten = 0;

		if (IsCanceled(JobId))
		{
			Progress(JobId, JobStateWKVType_Processing, JobStateReasonsWKVType_ProcessingToStopPoint, dwTotalRead);
			bCanceledByUser = true;
			break;
		}

		//Read the document chunk
		hr = m_pAttachment->Read(buffer.get(), BUFFER_SIZE, &dwBytesRead);
		bLastChunk = (hr == S_FALSE);

		DBGLOG("Read %u - 0x%08x", dwBytesRead, hr);

		//Create the printer and autodetect document format if needed.
		if (SUCCEEDED(hr) && (pPrinter == nullptr))
		{
			//The autodetection algorithm works if initial 'dwBytesRead' is enough to 
			//contain pdf and xps headers (which currently require 5 bytes at most).
			//Read at least 256 bytes: by experimental tests, 16384 bytes chunks are 
			//usually read, so this loop should never be executed.
			const ULONG MIN_REQUIRED_HEADER_SIZE = 256;
			static_assert(MIN_REQUIRED_HEADER_SIZE < BUFFER_SIZE);
			while (SUCCEEDED(hr) && (dwBytesRead < MIN_REQUIRED_HEADER_SIZE) && !bLastChunk)
			{
				ULONG dwCurrentRead = 0;
				hr = m_pAttachment->Read(buffer.get() + dwBytesRead, BUFFER_SIZE - dwBytesRead, &dwCurrentRead);
				bLastChunk = (hr == S_FALSE);

				dwBytesRead += dwCurrentRead;
			}

			if (SUCCEEDED(hr))
			{
				hr = CreatePrinter(&pPrinter, buffer.get(), dwBytesRead);
			}
		}

		//Send the document chunk to the printer
		if (SUCCEEDED(hr))
		{
			hr = pPrinter->Print(buffer.get(), dwBytesRead, &dwBytesWritten);
		}

		if (FAILED(hr))
		{
			DBGLOG("hr 0x%08x\n", hr);
		}

		dwTotalRead += dwBytesRead;
		dwTotalWritten += dwBytesWritten;
		Progress(JobId, JobStateWKVType_Processing, JobStateReasonsWKVType_None, dwTotalRead);
	}

	if (pPrinter != nullptr)
	{
		if (FAILED(hr) || bCanceledByUser)
		{
			pPrinter->Cancel();
		}

		HRESULT hrPrinter = S_OK;
		pPrinter->Close(&hrPrinter);
		if (SUCCEEDED(hr))
		{
			hr = hrPrinter;
		}
	}

	m_pAttachment->Close();

	SAFE_RELEASE(m_pAttachment);
	SAFE_RELEASE(pPrinter);

	if (bCanceledByUser)
	{
		Progress(JobId, JobStateWKVType_Canceled, JobStateReasonsWKVType_JobCanceledByUser, dwTotalRead);
	}
	else
	{
		Progress(JobId, JobStateWKVType_Completed, SUCCEEDED(hr) ? JobStateReasonsWKVType_JobCompletedSuccessfully : JobStateReasonsWKVType_JobCompletedWithErrors, dwTotalRead);
	}

	Release();

	if (FAILED(hr))
	{
		ELOG("Error 0x%08x\n", hr);
	}

	return hr;
}
