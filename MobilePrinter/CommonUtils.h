#pragma once

#include <assert.h>
#include <combaseapi.h>

#include "utils\LogUtils.h"
#include "utils\FileUtils.h"
#include "utils\StringUtils.h"
#include "utils\IUnknownUtils.h"

class global_config
{
public:
	static global_config& get()
	{
		static global_config s_instance;
		return s_instance;
	}

	void setup(const std::wstring& i_output_folder) { m_output_folder = i_output_folder; }
	const std::wstring& get_output_folder() const {	return m_output_folder;	}

private:
	std::wstring m_output_folder;
};

class myevent
{
public:
	myevent() : m_handle(INVALID_HANDLE_VALUE)
	{
		init();
	}
	~myevent()
	{
		CloseHandle(m_handle);
	}

	void set()
	{
		SetEvent(m_handle);
	}

	void reset()
	{
		ResetEvent(m_handle);
	}

	bool wait(DWORD dwMilliseconds)
	{
		return (WaitForSingleObject(m_handle, dwMilliseconds) == WAIT_OBJECT_0);
	}

private:
	void init()
	{
		m_handle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	}

private:
	HANDLE m_handle;
};



class CCoInit
{
public:
	CCoInit() : m_bCoInit(false)
	{
	}

	~CCoInit()
	{
		assert(!m_bCoInit);
	}

	HRESULT Initialize(DWORD dwCoinit)
	{
		HRESULT hr = CoInitializeEx(0, dwCoinit);
		m_bCoInit = SUCCEEDED(hr);
		return hr;
	}

	void Uninitialize()
	{
		if (m_bCoInit)
		{
			CoUninitialize();
			m_bCoInit = false;
		}
	}

private:
	bool m_bCoInit;
};
