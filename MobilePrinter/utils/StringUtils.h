#pragma once

#include <string>
#include <vector>
#include <rpcdce.h>

//Get current time in HH:MM:SS string format
std::wstring time_now();

//Guid helpers
UUID CreateGuid(const wchar_t* text);
std::wstring GuidToString(const UUID& uuid);

//Executable path
std::wstring GetExecutablePath();

// String helpers to get "const CharType*" from various string types
inline const char* to_cstr(const char* s) { return s; }
inline const wchar_t* to_cstr(const wchar_t* s) { return s; }
inline const char* to_cstr(const std::string& s) { return s.c_str(); }
inline const wchar_t* to_cstr(const std::wstring& s) { return s.c_str(); }
template<int SIZE> inline const char* to_cstr(const char s[SIZE]) { return &s[0]; }
template<int SIZE> inline const wchar_t* to_cstr(const wchar_t s[SIZE]) { return &s[0]; }

//Case-insensitive wchar_t string comparison
inline int _stricmp(const wchar_t* s1, const wchar_t* s2) { return _wcsicmp(s1, s2); }

//Case-insensitive generic CharType string comparison
template<typename S1, typename S2>
inline int icompare(const S1& left, const S2& right)
{
	return _stricmp(to_cstr(left), to_cstr(right));
}

//Case-insensitive generic CharType string equality
template<typename S1, typename S2>
inline bool imatch(const S1& left, const S2& right)
{
	return (icompare(left, right) == 0);
}

std::string wchar_to_utf8(const wchar_t* value);
std::string wchar_to_utf8(const std::wstring& value);
std::wstring utf8_to_wchar(const char* value);
std::wstring utf8_to_wchar(const std::string& value);

std::string& replace_all(std::string& s, const std::string& what, const std::string& with);
std::vector<std::string> split(const std::string& s, char delim, bool keep_empty_tokens);
std::string& trim_left(std::string& s);
std::string& trim_right(std::string& s);
std::string& trim(std::string& s);
std::string trim_left(const std::string& s);
std::string trim_right(const std::string& s);
std::string trim(const std::string& s);
bool begins_with(const std::string& s, const std::string& with);
bool ends_with(const std::string& s, const std::string& with);
std::string join(const std::vector<std::string>& v, char delim);

std::wstring& replace_all(std::wstring& s, const std::wstring& what, const std::wstring& with);
std::vector<std::wstring> split(const std::wstring& s, wchar_t delim, bool keep_empty_tokens);
std::wstring& trim_left(std::wstring& s);
std::wstring& trim_right(std::wstring& s);
std::wstring& trim(std::wstring& s);
std::wstring trim_left(const std::wstring& s);
std::wstring trim_right(const std::wstring& s);
std::wstring trim(const std::wstring& s);
bool begins_with(const std::wstring& s, const std::wstring& with);
bool ends_with(const std::wstring& s, const std::wstring& with);
std::wstring join(const std::vector<std::wstring>& v, wchar_t delim);

bool decode_hex_string(const std::string& i_string, std::vector<uint8_t>& o_bytes);
bool decode_hex_string(const std::wstring& i_string, std::vector<uint8_t>& o_bytes);
void encode_hex_string(const std::vector<uint8_t>& i_bytes, std::string& o_string);
void encode_hex_string(const std::vector<uint8_t>& i_bytes, std::wstring& o_string);
