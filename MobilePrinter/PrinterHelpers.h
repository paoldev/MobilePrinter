#pragma once

#include "CommonUtils.h"
#include <vector>

//Helpers to detect if passed parameters can be related to PDF or XPS files.
//XPS detection is just a guess, since xps file header is shared by many file types (which are not used in this program).
bool IsPdfFileName(const wchar_t* i_FileName);
bool IsXpsFileName(const wchar_t* i_FileName);
bool IsPdfDocumentFormat(const wchar_t* i_DocumentFormat);
bool IsXpsDocumentFormat(const wchar_t* i_DocumentFormat);
bool IsPdfFile(const void* i_pBuffer, const size_t i_uiBufferSize);
bool IsXpsFile(const void* i_pBuffer, const size_t i_uiBufferSize);
template<typename T> bool IsPdfFile(const std::vector<T>& i_buffer) { return IsPdfFile(i_buffer.data(), i_buffer.size() * sizeof(T)); }
template<typename T> bool IsXpsFile(const std::vector<T>& i_buffer) { return IsXpsFile(i_buffer.data(), i_buffer.size() * sizeof(T)); }

//Helper to directly print a file to the specified printer.
void PrintLocalFile(const wchar_t* lpPrinterName, const wchar_t* lpFileName);
