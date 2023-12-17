// <Include>
///////////////////////////////////////////////////////////////////////////////
//
// THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT MODIFY IT BY HAND.
//
///////////////////////////////////////////////////////////////////////////////
// </Include>

// <LiteralInclude>
#include "pch.h"
// </LiteralInclude>

// <LiteralInclude>
#include <wsdapi.h>
// </LiteralInclude>

// <LiteralInclude>
#include "MyPrinter_h.h"
// </LiteralInclude>

// <LiteralInclude>
#include "MyPrinterTypes.h"
// </LiteralInclude>

// <LiteralInclude>
#include "MyPrinterProxy.h"
// </LiteralInclude>

// <EventSourceBuilderImplementations>
HRESULT CreateCPrinterServiceTypeEventSource(IWSDDeviceHost* pHost, LPCWSTR pszServiceId, CPrinterServiceTypeEventSource** ppEventSourceOut)
{
    HRESULT hr = S_OK;
    CPrinterServiceTypeEventSource* pEventSource = NULL;

    // 
    // Validate parameters
    // 
    if( NULL == pHost )
    {
        return E_INVALIDARG;
    }

    if( NULL == pszServiceId )
    {
        return E_INVALIDARG;
    }

    if( NULL == ppEventSourceOut )
    {
        return E_POINTER;
    }

    *ppEventSourceOut = NULL;

    // 
    // Create event source object
    // 
    if( S_OK == hr )
    {
        pEventSource = new CPrinterServiceTypeEventSource();
        if( NULL == pEventSource )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // 
    // Initialize event source with host
    // 
    if( S_OK == hr )
    {
        hr = pEventSource->Init(pHost, pszServiceId);
    }

    // 
    // Cleanup
    // 
    if( S_OK == hr )
    {
        *ppEventSourceOut = pEventSource;
    }
    else
    {
        if( NULL != pEventSource )
        {
            pEventSource->Release();
        }
    }

    return hr;
}

// </EventSourceBuilderImplementations>

// <CDATA>

CPrinterServiceTypeEventSource::CPrinterServiceTypeEventSource() :
    m_cRef(1), m_host(NULL), m_serviceId(NULL)
{
}

CPrinterServiceTypeEventSource::~CPrinterServiceTypeEventSource() 
{
    if ( NULL != m_host )
    {
        m_host->Release();
        m_host = NULL;
    }
};

HRESULT STDMETHODCALLTYPE CPrinterServiceTypeEventSource::Init(
    /* [in] */ IWSDDeviceHost* pIWSDDeviceHost,
    /* [in] */ const WCHAR* serviceId )
{
    if( NULL == pIWSDDeviceHost )
    {
        return E_INVALIDARG;
    }

    m_serviceId = serviceId;

    m_host = pIWSDDeviceHost;
    m_host->AddRef();

    return S_OK;
}

// </CDATA>

// <IUnknownDefinitions>
HRESULT STDMETHODCALLTYPE CPrinterServiceTypeEventSource::QueryInterface(REFIID riid, void **ppvObject)
{
    if (NULL == ppvObject)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    *ppvObject = NULL;

    if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (__uuidof(IPrinterServiceTypeEventNotify) == riid)
    {
        *ppvObject = (IPrinterServiceTypeEventNotify *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        ((LPUNKNOWN)*ppvObject)->AddRef();
    }

    return hr;
}

ULONG STDMETHODCALLTYPE CPrinterServiceTypeEventSource::AddRef()
{
    ULONG ulNewRefCount = (ULONG)InterlockedIncrement((LONG *)&m_cRef);
    return ulNewRefCount;
}

ULONG STDMETHODCALLTYPE CPrinterServiceTypeEventSource::Release()
{
    ULONG ulNewRefCount = (ULONG)InterlockedDecrement((LONG *)&m_cRef);

    if (0 == ulNewRefCount)
    {
        delete this;
    }

    return ulNewRefCount;
}
// </IUnknownDefinitions>

// <ProxyFunctionImplementations>
HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::PrinterElementsChangeEvent
(   /* [in] */ PRINTER_ELEMENTS_CHANGE_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_PrinterElementsChangeEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[9]
        );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::PrinterStatusSummaryEvent
(   /* [in] */ PRINTER_STATUS_SUMMARY_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_PrinterStatusSummaryEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[10]
        );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::PrinterStatusConditionEvent
(   /* [in] */ PRINTER_STATUS_CONDITION_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_PrinterStatusConditionEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[11]
        );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::PrinterStatusConditionClearedEvent
(   /* [in] */ PRINTER_STATUS_CONDITION_CLEARED_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_PrinterStatusConditionClearedEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[12]
        );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::JobStatusEvent
(   /* [in] */ JOB_STATUS_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_JobStatusEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[13]
        );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CPrinterServiceTypeEventSource::JobEndStateEvent
(   /* [in] */ JOB_END_STATE_EVENT_TYPE* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceType_JobEndStateEvent request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceType[14]
        );

    return hr;
}

// </ProxyFunctionImplementations>

// <EventSourceBuilderImplementations>
HRESULT CreateCPrinterServiceV20TypeEventSource(IWSDDeviceHost* pHost, LPCWSTR pszServiceId, CPrinterServiceV20TypeEventSource** ppEventSourceOut)
{
    HRESULT hr = S_OK;
    CPrinterServiceV20TypeEventSource* pEventSource = NULL;

    // 
    // Validate parameters
    // 
    if( NULL == pHost )
    {
        return E_INVALIDARG;
    }

    if( NULL == pszServiceId )
    {
        return E_INVALIDARG;
    }

    if( NULL == ppEventSourceOut )
    {
        return E_POINTER;
    }

    *ppEventSourceOut = NULL;

    // 
    // Create event source object
    // 
    if( S_OK == hr )
    {
        pEventSource = new CPrinterServiceV20TypeEventSource();
        if( NULL == pEventSource )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // 
    // Initialize event source with host
    // 
    if( S_OK == hr )
    {
        hr = pEventSource->Init(pHost, pszServiceId);
    }

    // 
    // Cleanup
    // 
    if( S_OK == hr )
    {
        *ppEventSourceOut = pEventSource;
    }
    else
    {
        if( NULL != pEventSource )
        {
            pEventSource->Release();
        }
    }

    return hr;
}

// </EventSourceBuilderImplementations>

// <CDATA>

CPrinterServiceV20TypeEventSource::CPrinterServiceV20TypeEventSource() :
    m_cRef(1), m_host(NULL), m_serviceId(NULL)
{
}

CPrinterServiceV20TypeEventSource::~CPrinterServiceV20TypeEventSource() 
{
    if ( NULL != m_host )
    {
        m_host->Release();
        m_host = NULL;
    }
};

HRESULT STDMETHODCALLTYPE CPrinterServiceV20TypeEventSource::Init(
    /* [in] */ IWSDDeviceHost* pIWSDDeviceHost,
    /* [in] */ const WCHAR* serviceId )
{
    if( NULL == pIWSDDeviceHost )
    {
        return E_INVALIDARG;
    }

    m_serviceId = serviceId;

    m_host = pIWSDDeviceHost;
    m_host->AddRef();

    return S_OK;
}

// </CDATA>

// <IUnknownDefinitions>
HRESULT STDMETHODCALLTYPE CPrinterServiceV20TypeEventSource::QueryInterface(REFIID riid, void **ppvObject)
{
    if (NULL == ppvObject)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    *ppvObject = NULL;

    if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (__uuidof(IPrinterServiceV20TypeEventNotify) == riid)
    {
        *ppvObject = (IPrinterServiceV20TypeEventNotify *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        ((LPUNKNOWN)*ppvObject)->AddRef();
    }

    return hr;
}

ULONG STDMETHODCALLTYPE CPrinterServiceV20TypeEventSource::AddRef()
{
    ULONG ulNewRefCount = (ULONG)InterlockedIncrement((LONG *)&m_cRef);
    return ulNewRefCount;
}

ULONG STDMETHODCALLTYPE CPrinterServiceV20TypeEventSource::Release()
{
    ULONG ulNewRefCount = (ULONG)InterlockedDecrement((LONG *)&m_cRef);

    if (0 == ulNewRefCount)
    {
        delete this;
    }

    return ulNewRefCount;
}
// </IUnknownDefinitions>

// <ProxyFunctionImplementations>
HRESULT STDMETHODCALLTYPE
CPrinterServiceV20TypeEventSource::PrintDeviceCapabilitiesChangeID
(   /* [in] */ const WCHAR* body
)
{
    HRESULT hr = S_OK;

    // Validate Request Parameters
    if( NULL == body )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    RESPONSEBODY_PrinterServiceV20Type_PrintDeviceCapabilitiesChangeID request;

    request.body = body;

    hr =
        m_host->SignalEvent
        (   m_serviceId
        ,   &request
        ,   &Operations_PrinterServiceV20Type[5]
        );

    return hr;
}

// </ProxyFunctionImplementations>
