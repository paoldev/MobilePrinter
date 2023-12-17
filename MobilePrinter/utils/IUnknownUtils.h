#pragma once

#include <Unknwn.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { p->Release(); p = nullptr; } }
#endif

/**
	CUnknown: base class for IUnknown derived interfaces with auto-implemented QueryInterface method.
*/
class CUnknownHelper : public IUnknown {};

template<typename T, typename ... I>
class CUnknown : public CUnknownHelper, public I...
{
	typedef CUnknown<T, I...> TheUnknown;

public:

	CUnknown() : m_cRef(1)
	{
	}

	virtual ~CUnknown()
	{
	}

	virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (nullptr == ppvObject)
		{
			return E_POINTER;
		}

		HRESULT hr = S_OK;
		*ppvObject = nullptr;

		TheUnknown* pThis = (TheUnknown*)(T*)this;

		if (__uuidof(IUnknown) == riid)
		{
			*ppvObject = (CUnknownHelper*)pThis;
		}
		else
		{
			TQueryInterface(riid, ppvObject, pThis);
		}

		if ((*ppvObject) == nullptr)
		{
			hr = E_NOINTERFACE;
		}

		if (SUCCEEDED(hr))
		{
			pThis->AddRef();
		}

		return hr;
	}

	virtual ULONG __stdcall AddRef(void) final
	{
		ULONG ulNewRefCount = (ULONG)InterlockedIncrement((LONG*)&m_cRef);
		return ulNewRefCount;
	}

	virtual ULONG __stdcall Release(void) final
	{
		ULONG ulNewRefCount = (ULONG)InterlockedDecrement((LONG*)&m_cRef);

		if (0 == ulNewRefCount)
		{
			delete this;
		}

		return ulNewRefCount;
	}

private:

	template<typename U>
	static void QIFunc(REFIID riid, void** ppvObject, U* ptr)
	{
		if ((*ppvObject == nullptr) && (__uuidof(U) == riid))
		{
			*ppvObject = ptr;
		}
	}

	static void TQueryInterface(REFIID riid, void** ppvObject, TheUnknown* ptr)
	{
		int unused[] = { (QIFunc<I>(riid, ppvObject, ptr), 0)..., 0 };
		(void)unused;
	}

private:
	ULONG m_cRef;
};
