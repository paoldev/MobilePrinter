#pragma once

#include <wsdapi.h>
#include <initializer_list>
#include "src_host/MyPrinterTypes.h"
#include "CommonUtils.h"

template<typename T> inline T* WSDAlloc(void* parent)
{
	T* res = static_cast<T*>(WSDAllocateLinkedMemory(parent, sizeof(T)));
	if (res != nullptr)
	{
		memset(res, 0, sizeof(T));
	}
	return res;
}

template<typename T> inline T* WSDAlloc(void* parent, const T& value)
{
	T* res = static_cast<T*>(WSDAllocateLinkedMemory(parent, sizeof(T)));
	if (res != nullptr)
	{
		*res = value;
	}
	return res;
}

inline wchar_t* WSDAlloc(void* parent, const wchar_t* value)
{
	if (value == nullptr)
	{
		value = L"";
	}
	const size_t len = wcslen(value);
	wchar_t* res = static_cast<wchar_t*>(WSDAllocateLinkedMemory(parent, sizeof(value[0])*(len + 1)));
	if (res != nullptr)
	{
		wcscpy_s(res, len + 1, value);
	}
	return res;
}

inline wchar_t* WSDAlloc(void* parent, const std::wstring& str)
{
	return WSDAlloc(parent, str.c_str());
}

inline VALUE_TOKEN_LIST_TYPE* WSDAllocTokenList(void* parent, std::initializer_list<const wchar_t*> values)
{
	VALUE_TOKEN_LIST_TYPE* res = WSDAlloc<VALUE_TOKEN_LIST_TYPE>(parent);
	if (res)
	{
		PWCHAR_LIST_1** ppNextElem = &res->AllowedValue;

		for (auto it = values.begin(); it != values.end(); it++)
		{
			PWCHAR_LIST_1* pNextElem = WSDAlloc<PWCHAR_LIST_1>(parent);
			*ppNextElem = pNextElem;
			if (pNextElem != nullptr)
			{
				pNextElem->Element = WSDAlloc(parent, *it);
				pNextElem->Next = nullptr;
				ppNextElem = &(pNextElem->Next);
			}
		}
	}
	return res;
}

//template<int SIZE>
//VALUE_STRING_LIST_TYPE* WSDAllocStringList(void* parent, const wchar_t*(&Elems)[SIZE])
inline VALUE_STRING_LIST_TYPE* WSDAllocStringList(void* parent, std::initializer_list<const wchar_t*> values)
{
	VALUE_STRING_LIST_TYPE* res = WSDAlloc<VALUE_STRING_LIST_TYPE>(parent);
	if (res)
	{
		PWCHAR_LIST_2** ppNextElem = &res->AllowedValue;

		for (auto it = values.begin(); it != values.end(); it++)
		{
			PWCHAR_LIST_2* pNextElem = WSDAlloc<PWCHAR_LIST_2>(parent);
			*ppNextElem = pNextElem;
			if (pNextElem != nullptr)
			{
				pNextElem->Element = WSDAlloc(parent, *it);
				pNextElem->Next = nullptr;
				ppNextElem = &(pNextElem->Next);
			}
		}
	}
	return res;
}

inline VALUE_INT_LIST_TYPE* WSDAllocIntList(void* parent, std::initializer_list<LONG> values)
{
	VALUE_INT_LIST_TYPE* res = WSDAlloc<VALUE_INT_LIST_TYPE>(parent);
	if (res)
	{
		LONG_LIST** ppNextElem = &res->AllowedValue;

		for (auto it = values.begin(); it != values.end(); it++)
		{
			LONG_LIST* pNextElem = WSDAlloc<LONG_LIST>(parent);
			*ppNextElem = pNextElem;
			if (pNextElem != nullptr)
			{
				pNextElem->Element = *it;
				pNextElem->Next = nullptr;
				ppNextElem = &(pNextElem->Next);
			}
		}
	}
	return res;
}

template<typename WSDXML_ELEMENT_TYPE>
inline void AppendChild(WSDXML_ELEMENT* parentElement, WSDXML_ELEMENT_TYPE* childElement)
{
	if (parentElement && childElement)
	{
		WSDXML_NODE** pLastChild = &parentElement->FirstChild;
		while ((*pLastChild) != nullptr)
		{
			pLastChild = &((*pLastChild)->Next);
		}
		*pLastChild = &childElement->Node;
		childElement->Node.Parent = parentElement;
	}
}

inline WSDXML_ELEMENT* WSDAllocElementWithText(void* parent, WSDXML_NAME* elementName, const wchar_t* text)
{
	WSDXML_ELEMENT* pElement = WSDAlloc<WSDXML_ELEMENT>(parent);
	if (pElement)
	{
		pElement->Node = { WSDXML_NODE::ElementType, nullptr, nullptr };
		pElement->Name = elementName;
		pElement->FirstAttribute = nullptr;
		pElement->PrefixMappings = nullptr;
		pElement->FirstChild = nullptr;

		WSDXML_TEXT* pText = WSDAlloc<WSDXML_TEXT>(parent);
		if (pText)
		{
			pText->Node = { WSDXML_NODE::TextType, pElement, nullptr };
			pText->Text = WSDAlloc(parent, text);

			AppendChild(pElement, pText);
		}
	}

	return pElement;
}

inline WSDXML_ELEMENT* WSDAllocElementWithText(void* parent, WSDXML_NAME* elementName, const std::wstring& str)
{
	return WSDAllocElementWithText(parent, elementName, str.c_str());
}

inline WSDXML_ELEMENT* WSDAllocElementWithTokenList(void* parent, WSDXML_ELEMENT* parentElement, WSDXML_NAME* elementName, std::initializer_list<const wchar_t*> values)
{
	WSDXML_ELEMENT* pTokenList = WSDAlloc<WSDXML_ELEMENT>(parent);
	if (pTokenList)
	{
		pTokenList->Node = { WSDXML_NODE::ElementType, parentElement, nullptr };
		pTokenList->Name = elementName;
		pTokenList->FirstAttribute = nullptr;
		pTokenList->PrefixMappings = nullptr;
		for (auto it = values.begin(); it != values.end(); it++)
		{
			WSDXML_ELEMENT* pToken = WSDAllocElementWithText(parent, NAME_PRINT_AllowedValue, *it);
			AppendChild(pTokenList, pToken);
		}
		AppendChild(parentElement, pTokenList);
	}

	return pTokenList;
}

inline bool AreSameName(const WSDXML_NAME* n1, const WSDXML_NAME* n2)
{
	if ((!n1) || (!n2))
	{
		return false;
	}

	if (wcscmp(n1->LocalName, n2->LocalName) != 0)
	{
		return false;
	}

	if (!n1->Space && !n2->Space)
	{
		return true;
	}

	if (n1->Space && !n2->Space)
	{
		return false;
	}

	if (!n1->Space && n2->Space)
	{
		return false;
	}

	if (!n1->Space->Uri && !n2->Space->Uri)
	{
		return true;
	}

	if (n1->Space->Uri && !n2->Space->Uri)
	{
		return false;
	}

	if (!n1->Space->Uri && n2->Space->Uri)
	{
		return false;
	}

	return (wcscmp(n1->Space->Uri, n2->Space->Uri) == 0);
}

inline void WSDAppend(WSDXML_ELEMENT*& list, WSDXML_ELEMENT* element)
{
	if (list == nullptr)
	{
		list = element;
	}
	else
	{
		WSDXML_NODE* node = &list->Node;
		while (node->Next != nullptr)
		{
			node = node->Next;
		}
		node->Next = &element->Node;
	}
}

inline void WSDAppend(void* parent, PWCHAR_LIST*& list, const wchar_t* text)
{
	PWCHAR_LIST* elem = WSDAlloc<PWCHAR_LIST>(parent);
	if (elem)
	{
		elem->Element = WSDAlloc(parent, text);
		elem->Next = nullptr;
	}

	if (list == nullptr)
	{
		list = elem;
	}
	else
	{
		PWCHAR_LIST* node = list;
		while (node->Next != nullptr)
		{
			node = node->Next;
		}
		node->Next = elem;
	}
}

inline void WSDAppend(void* parent, PWCHAR_LIST*& list, const std::wstring& str)
{
	WSDAppend(parent, list, str.c_str());
}

