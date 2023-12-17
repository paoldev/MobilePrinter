#include "pch.h"
#include "WSDPrinterInfo.h"
#include "WSDUtils.h"

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetMediaSizeName(void * root) const
{
	return WSDAllocTokenList(root,
		{ MediaSizeSelfDescribingNameWKVType_iso_a4_210x297mm/*, MediaSizeSelfDescribingNameWKVType_iso_c5_162x229mm,
			MediaSizeSelfDescribingNameWKVType_iso_dl_110x220mm , MediaSizeSelfDescribingNameWKVType_jis_b4_257x364mm ,
			MediaSizeSelfDescribingNameWKVType_na_legal_8_5x14in , MediaSizeSelfDescribingNameWKVType_na_letter_8_5x11in,
			MediaSizeSelfDescribingNameWKVType_pwg_letter_or_a4_choice*/ });
}

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetMediaType(void * root) const
{
	return WSDAllocTokenList(root, { /*MediaTypeWKVType_cardstock, MediaTypeWKVType_envelope ,
						MediaTypeWKVType_labels , MediaTypeWKVType_photographic , MediaTypeWKVType_photographic_glossy , MediaTypeWKVType_photographic_matte ,*/
						MediaTypeWKVType_stationery ,MediaTypeWKVType_stationery_inkjet/*, MediaTypeWKVType_transparency, MediaTypeWKVType_other,MediaTypeWKVType_unknown */ });
}

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetMediaColor(void * root) const
{
	return WSDAllocTokenList(root, { MediaColorWKVType_unknown/*,
						MediaColorWKVType_white, MediaColorWKVType_pink, MediaColorWKVType_yellow,MediaColorWKVType_buff, MediaColorWKVType_goldenrod,
						MediaColorWKVType_blue, MediaColorWKVType_green, MediaColorWKVType_red, MediaColorWKVType_gray, MediaColorWKVType_ivory,
						MediaColorWKVType_orange, MediaColorWKVType_other, MediaColorWKVType_no_color */ });
}

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetOrientation(void * root) const
{
	return WSDAllocTokenList(root, { OrientationWKVType_Landscape, OrientationWKVType_Portrait ,
		/*	OrientationWKVType_ReverseLandscape , OrientationWKVType_ReversePortrait*/ });
}

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetPrintQuality(void * root) const
{
	return WSDAllocTokenList(root, { PrintQualityWKVType_Draft, PrintQualityWKVType_High,
						PrintQualityWKVType_Normal, PrintQualityWKVType_Photo });
}

VALUE_TOKEN_LIST_TYPE * WSDPrinterInfo::WSDGetSides(void * root) const
{
	return WSDAllocTokenList(root, { SidesWKVType_OneSided/*, SidesWKVType_TwoSidedLongEdge ,SidesWKVType_TwoSidedShortEdge*/ });
}

NUMBER_UP_0 * WSDPrinterInfo::WSDGetNumberUp(void * root) const
{
	NUMBER_UP_0* NumberUp = WSDAlloc<NUMBER_UP_0>(root);
	NumberUp->PagesPerSheet = WSDAlloc<VALUE_INT_LIST_TYPE>(root);
	NumberUp->PagesPerSheet->AllowedValue = WSDAlloc<LONG_LIST>(root);
	NumberUp->PagesPerSheet->AllowedValue->Element = 1;
	NumberUp->PagesPerSheet->AllowedValue->Next = nullptr;
	NumberUp->Direction = WSDAllocTokenList(root, { NUpDirectionRestrictionType_RightDown/*, NUpDirectionRestrictionType_DownRight ,
		NUpDirectionRestrictionType_LeftDown , NUpDirectionRestrictionType_DownLeft */ });
	NumberUp->Any = nullptr;

	return NumberUp;
}

RESOLUTION_0 * WSDPrinterInfo::WSDGetResolution(void * root) const
{
	RESOLUTION_0* Resolution = WSDAlloc<RESOLUTION_0>(root);
	Resolution->AllowedValue = WSDAlloc<RESOLUTION_ENTRY_TYPE_LIST>(root);
	Resolution->AllowedValue->Element = WSDAlloc<RESOLUTION_ENTRY_TYPE>(root);
	Resolution->AllowedValue->Element->Width = 300;
	Resolution->AllowedValue->Element->Height = WSDAlloc<LONG>(root, 300);
	Resolution->AllowedValue->Next = nullptr;
	return Resolution;
}

VALUE_INT_RANGE_TYPE * WSDPrinterInfo::WSDGetCopies(void * root) const
{
	VALUE_INT_RANGE_TYPE* Copies = WSDAlloc<VALUE_INT_RANGE_TYPE>(root);
	Copies->MinValue = WSDAlloc<LONG>(root, GetMinNumCopies());
	Copies->MaxValue = GetMaxNumCopies();

	return Copies;
}

VALUE_INT_RANGE_TYPE * WSDPrinterInfo::WSDGetPriority(void * root) const
{
	VALUE_INT_RANGE_TYPE* Priority = WSDAlloc<VALUE_INT_RANGE_TYPE>(root);
	Priority->MinValue = WSDAlloc<LONG>(root, 1);
	Priority->MaxValue = 100;

	return Priority;
}

JOB_FINISHINGS_0 * WSDPrinterInfo::WSDGetJobFinishing(void * root) const
{
	JOB_FINISHINGS_0* JobFinishings = WSDAlloc<JOB_FINISHINGS_0>(root);
	JobFinishings->Staple = WSDAlloc<STAPLE_0>(root);
	JobFinishings->Staple->Location = WSDAllocTokenList(root, { StapleLocationWKVType_None /*,
		StapleLocationWKVType_StapleBottomLeft , StapleLocationWKVType_StapleBottomRight ,StapleLocationWKVType_StapleTopLeft,
		StapleLocationWKVType_StapleTopRight , StapleLocationWKVType_StapleDualBottom , StapleLocationWKVType_StapleDualLeft ,
		StapleLocationWKVType_StapleDualRight , StapleLocationWKVType_StapleDualTop , StapleLocationWKVType_SaddleStitch ,
		StapleLocationWKVType_other ,StapleLocationWKVType_unknown*/ });
	JobFinishings->Staple->Angle = WSDAllocTokenList(root, { /*StapleAngleWKVType_unknown,*/ StapleAngleWKVType_NotApplicable/*,
		StapleAngleWKVType_Any , StapleAngleWKVType_Horizontal , StapleAngleWKVType_Slanted , StapleAngleWKVType_Vertical*/ });
	JobFinishings->Staple->Any = nullptr;
	JobFinishings->HolePunch = WSDAlloc<HOLE_PUNCH_0>(root);
	JobFinishings->HolePunch->Edge = WSDAllocTokenList(root, { HolePunchEdgeWKVType_None/*, HolePunchEdgeWKVType_Top ,
		HolePunchEdgeWKVType_Bottom , HolePunchEdgeWKVType_Left , HolePunchEdgeWKVType_Right*/ });
	JobFinishings->HolePunch->Pattern = WSDAllocTokenList(root, { /*HolePunchPatternWKVType_unknown, */
		HolePunchPatternWKVType_NotApplicable/*, HolePunchPatternWKVType_TwoHoleUSTop , HolePunchPatternWKVType_ThreeHoleUS ,
		HolePunchPatternWKVType_TwoHoleDIN , HolePunchPatternWKVType_FourHoleDIN , HolePunchPatternWKVType_TwentyTwoHoleUS ,
		HolePunchPatternWKVType_NineteenHoleUS , HolePunchPatternWKVType_TwoHoleMetric , HolePunchPatternWKVType_Swedish4Hole ,
		HolePunchPatternWKVType_TwoHoleUSSide , HolePunchPatternWKVType_FiveHoleUS , HolePunchPatternWKVType_SevenHoleUS ,
		HolePunchPatternWKVType_Mixed7H4S , HolePunchPatternWKVType_Norweg6Hole , HolePunchPatternWKVType_Metric26Hole , HolePunchPatternWKVType_Metric30Hole */ });
	JobFinishings->HolePunch->Any = nullptr;
	JobFinishings->Any = nullptr;
	return JobFinishings;
}
