//// 
//// This class is derived from D2DPrintJobChecker implemented at
//// https://github.com/Microsoft/Windows-classic-samples/tree/master/Samples/D2DPrintingFromDesktopApps
//// 
//// My changes:
//// -	D2DPrintJobChecker renamed XpsPrintJobChecker
//// -	IUnknown interface implemented through my CUnknown implementation
//// -	Creation factory
//// 

//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "CommonUtils.h"
#include <DocumentTarget.h>

class XpsPrintJobChecker : public CUnknown<XpsPrintJobChecker, IPrintDocumentPackageStatusEvent>
{
public:

	static HRESULT Create(XpsPrintJobChecker** ppXpsPrintJobChecker);

	// Implement virtual functions from interface IDispatch.
	virtual STDMETHODIMP
		GetTypeInfoCount(
			_Out_ UINT *pctinfo
		);
	virtual STDMETHODIMP
		GetTypeInfo(
			UINT iTInfo,
			LCID lcid,
			_Outptr_result_maybenull_ ITypeInfo **ppTInfo
		);
	virtual STDMETHODIMP
		GetIDsOfNames(
			_In_                        REFIID      riid,
			_In_reads_(cNames)          LPOLESTR*   rgszNames,
			_In_range_(0, 16384)        UINT        cNames,
			LCID        lcid,
			__out_ecount_full(cNames)   DISPID*     rgDispId
		);
	virtual STDMETHODIMP
		Invoke(
			DISPID          dispIdMember,
			REFIID          riid,
			LCID            lcid,
			WORD            wFlags,
			DISPPARAMS*     pDispParams,
			VARIANT*        pVarResult,
			EXCEPINFO*      pExcepInfo,
			UINT*           puArgErr
		);

	// Implement virtual functions from interface IPrintDocumentPackageStatusEvent.
	virtual STDMETHODIMP
		PackageStatusUpdated(
			_In_ PrintDocumentPackageStatus* packageStatus
		);

	// New functions in XpsPrintJobChecker.
	HRESULT Initialize(
		_In_ IPrintDocumentPackageTarget* documentPackageTarget
	);
	PrintDocumentPackageStatus GetStatus();
	HRESULT WaitForCompletion();

private:
	XpsPrintJobChecker();
	~XpsPrintJobChecker();

	void ReleaseResources();
	static void OutputPackageStatus(
		_In_ PrintDocumentPackageStatus packageStatus
	);

private:
	PrintDocumentPackageStatus m_documentPackageStatus;
	DWORD m_eventCookie;
	HANDLE m_completionEvent;
	CRITICAL_SECTION m_criticalSection;
	IConnectionPoint* m_connectionPoint;
	bool m_isInitialized;
};

