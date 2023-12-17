#pragma once

#include "PrinterInfo.h"
#include "src_host\myprintertypes.h"

class WSDPrinterInfo : public CPrinterInfo
{
public:

	VALUE_TOKEN_LIST_TYPE* WSDGetMediaSizeName(void* root) const;
	VALUE_TOKEN_LIST_TYPE* WSDGetMediaType(void* root) const;
	VALUE_TOKEN_LIST_TYPE* WSDGetMediaColor(void* root) const;
	VALUE_TOKEN_LIST_TYPE* WSDGetOrientation(void* root) const;
	VALUE_TOKEN_LIST_TYPE* WSDGetPrintQuality(void* root) const;
	VALUE_TOKEN_LIST_TYPE* WSDGetSides(void* root) const;
	NUMBER_UP_0* WSDGetNumberUp(void* root) const;
	RESOLUTION_0* WSDGetResolution(void* root) const;
	VALUE_INT_RANGE_TYPE* WSDGetCopies(void* root) const;
	VALUE_INT_RANGE_TYPE* WSDGetPriority(void* root) const;
	JOB_FINISHINGS_0* WSDGetJobFinishing(void* root) const;
};
