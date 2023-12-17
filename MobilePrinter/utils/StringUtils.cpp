#include "pch.h"
#include "CommonUtils.h"

#include <sstream>
#include <time.h>

std::wstring time_now()
{
	tm tm_;
	time_t t = time(nullptr);
	localtime_s(&tm_, &t);

	wchar_t wstr[100] = {};
	std::wcsftime(wstr, _countof(wstr), L"%T", &tm_);
	return wstr;
}

UUID CreateGuid(const wchar_t * text)
{
	UUID uuid = {};

	HCRYPTPROV hProv = NULL;
	if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		HCRYPTPROV hHash = NULL;
		if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		{
			if (CryptHashData(hHash, reinterpret_cast<const BYTE *>(text), static_cast<DWORD>(wcslen(text) * sizeof(wchar_t)), 0))
			{
				DWORD cbHashSize = 0, dwCount = sizeof(DWORD);

				if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&cbHashSize, &dwCount, 0))
				{
					cbHashSize = (cbHashSize <= sizeof(uuid)) ? cbHashSize : sizeof(uuid);
					if (CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(&uuid), &cbHashSize, 0))
					{

					}
				}
			}

			CryptDestroyHash(hHash);
		}

		CryptReleaseContext(hProv, 0);
	}

	return uuid;
}

std::wstring GuidToString(const UUID& uuid)
{
	wchar_t res[8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1] = {};
	wsprintf(res, L"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X", uuid.Data1, uuid.Data2, uuid.Data3, (uint32_t(uuid.Data4[0]) << 8) | uuid.Data4[1],
		uuid.Data4[2], uuid.Data4[3], uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);

	return std::wstring(res);
}

std::wstring GetExecutablePath()
{
	std::wstring path;
	path.resize(1024);
	while (!path.empty())
	{
		// See https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew
		DWORD res = GetModuleFileNameW(nullptr, &path[0], static_cast<DWORD>(path.size()));
		if (res == 0)
		{
			path.clear();
		}
		else if (res >= path.size())
		{
			switch (GetLastError())
			{
			case ERROR_SUCCESS:	//WindowsXP
			case ERROR_INSUFFICIENT_BUFFER: //other than WindowsXP
				if (path.size() < (UINT32_MAX / 2))
				{
					path.resize(2 * path.size());
				}
				else
				{
					path.clear();
				}
				break;

			default:
				path.clear();
				break;
			}
		}
		else
		{
			path.resize(res);
			break;
		}
	}

	return path;
}

std::string internal_wchar_to_utf8(const wchar_t* value, const size_t wlen)
{
	std::string res;
	if ((value != nullptr) && (wlen > 0))
	{
		int res_len = WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int32_t>(wlen), nullptr, 0, nullptr, nullptr);
		res.resize(res_len, 0);
		WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int32_t>(wlen), res.data(), res_len, nullptr, nullptr);
	}
	return res;
}

std::wstring internal_utf8_to_wchar(const char* value, const size_t len)
{
	std::wstring res;
	if ((value != nullptr) && (len > 0))
	{
		int res_len = MultiByteToWideChar(CP_UTF8, 0, value, static_cast<int32_t>(len), nullptr, 0);
		res.resize(res_len, 0);
		MultiByteToWideChar(CP_UTF8, 0, value, static_cast<int32_t>(len), res.data(), res_len);
	}
	return res;
}

std::string wchar_to_utf8(const wchar_t* value)
{
	return internal_wchar_to_utf8(value, value ? wcslen(value) : 0);
}

std::string wchar_to_utf8(const std::wstring& value)
{
	return internal_wchar_to_utf8(value.data(), value.size());
}

std::wstring utf8_to_wchar(const char* value)
{
	return internal_utf8_to_wchar(value, value ? strlen(value) : 0);
}

std::wstring utf8_to_wchar(const std::string& value)
{
	return internal_utf8_to_wchar(value.data(), value.size());
}


template<typename T> T& internal_replace_all(T& s, const T& what, const T& with)
{
	if (what.empty())
		return s;

	size_t curr_pos = 0;
	while ((curr_pos = s.find(what, curr_pos)) != T::npos)
	{
		s.replace(curr_pos, what.length(), with);
		curr_pos += with.length();
	}
	return s;
}

std::string& replace_all(std::string& s, const std::string& what, const std::string& with)
{
	return internal_replace_all(s, what, with);
}

std::wstring& replace_all(std::wstring& s, const std::wstring& what, const std::wstring& with)
{
	return internal_replace_all(s, what, with);
}

template<typename T> std::vector<T> internal_split(const T& s, typename T::value_type delimiter, bool keep_empty_tokens)
{
	typedef typename T::value_type CharT;

	std::vector<T> tokens;
	T token;
	std::basic_istringstream<CharT, std::char_traits<CharT>, std::allocator<CharT>> tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		if (token.size() || keep_empty_tokens)
		{
			tokens.push_back(token);
		}
	}
	return tokens;
}

std::vector<std::string> split(const std::string& s, char delim, bool keep_empty_tokens)
{
	return internal_split(s, delim, keep_empty_tokens);
}

std::vector<std::wstring> split(const std::wstring& s, wchar_t delim, bool keep_empty_tokens)
{
	return internal_split(s, delim, keep_empty_tokens);
}

template <typename T> T& internal_trim_left(T& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto& ch)->bool { return !isspace(ch); }));
	return s;
}

template <typename T> T& internal_trim_right(T& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](auto& ch)->bool {	return !isspace(ch); }).base(), s.end());
	return s;
}

template <typename T> T internal_trim_left(const T& cs)
{
	T s = cs;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto& ch)->bool { return !isspace(ch); }));
	return s;
}

template <typename T> T internal_trim_right(const T& cs)
{
	T s = cs;
	s.erase(std::find_if(s.rbegin(), s.rend(), [](auto& ch)->bool {	return !isspace(ch); }).base(), s.end());
	return s;
}

template<typename T>
bool internal_begins_with(const T& s, const T& with)
{
	return (s.find(with, 0) == 0);
}

template<typename T>
bool internal_ends_with(const T& s, const T& with)
{
	size_t p = s.rfind(with);
	if (p != T::npos)
	{
		return ((p + with.length()) == s.length());
	}

	return false;
}

template<typename T>
T internal_join(const std::vector<T>& v, typename T::value_type delim)
{
	T s;
	for (auto it = v.cbegin(); it != v.cend(); it++)
	{
		s += *it;
		if (it != (v.cend() - 1))
		{
			s += delim;
		}
	}
	return s;
}

std::string& trim_left(std::string& s)
{
	return internal_trim_left(s);
}

std::string& trim_right(std::string& s)
{
	return internal_trim_right(s);
}

std::string& trim(std::string& s)
{
	return trim_left(trim_right(s));
}

std::string trim_left(const std::string& s)
{
	return internal_trim_left(s);
}

std::string trim_right(const std::string& s)
{
	return internal_trim_right(s);
}

std::string trim(const std::string& s)
{
	std::string r = trim_right(s);
	internal_trim_left(r);
	return r;
}

bool begins_with(const std::string& s, const std::string& with)
{
	return internal_begins_with(s, with);
}

bool ends_with(const std::string& s, const std::string& with)
{
	return internal_ends_with(s, with);
}

std::string join(const std::vector<std::string>& v, char delim)
{
	return internal_join(v, delim);
}


std::wstring& trim_left(std::wstring& s)
{
	return internal_trim_left(s);
}
std::wstring& trim_right(std::wstring& s)
{
	return internal_trim_right(s);
}

std::wstring& trim(std::wstring& s)
{
	return trim_left(trim_right(s));
}

std::wstring trim_left(const std::wstring& s)
{
	return internal_trim_left(s);
}

std::wstring trim_right(const std::wstring& s)
{
	return internal_trim_right(s);
}

std::wstring trim(const std::wstring& s)
{
	std::wstring r = trim_right(s);
	internal_trim_left(r);
	return r;
}


bool begins_with(const std::wstring& s, const std::wstring& with)
{
	return internal_begins_with(s, with);
}

bool ends_with(const std::wstring& s, const std::wstring& with)
{
	return internal_ends_with(s, with);
}

std::wstring join(const std::vector<std::wstring>& v, wchar_t delim)
{
	return internal_join(v, delim);
}


template<typename T>
static int hex_to_index(const T hex)
{
	if ((hex >= T('0')) && (hex <= T('9')))
	{
		return (hex - T('0'));
	}
	else if ((hex >= T('a')) && (hex <= T('z')))
	{
		return 10 + (hex - T('a'));
	}
	else if ((hex >= T('A')) && (hex <= T('Z')))
	{
		return 10 + (hex - T('A'));
	}

	return -1;
}

template<typename T>
bool internal_decode_hex_string(const T& i_string, std::vector<uint8_t>& o_bytes)
{
	if (i_string.size() % 2)
	{
		o_bytes.clear();
		return false;
	}

	o_bytes.resize(i_string.size() / 2);
	for (size_t i = 0; i < o_bytes.size(); i++)
	{
		int b1 = hex_to_index(i_string[2 * i]);
		int b2 = hex_to_index(i_string[2 * i + 1]);

		if ((b1 < 0) || (b2 < 0))
		{
			o_bytes.clear();
			return false;
		}
		assert((b1 < 16) && (b2 < 16));
		o_bytes[i] = static_cast<uint8_t>(((b1 & 0xf) << 4) | (b2 & 0xf));
	}

	return true;
}

template<typename T> struct HexTraits {};
template<> struct HexTraits<char> { static constexpr const char* value = "0123456789ABCDEF"; };
template<> struct HexTraits<wchar_t> { static constexpr const wchar_t* value = L"0123456789ABCDEF"; };

template<typename T>
void internal_encode_hex_string(const std::vector<uint8_t>& i_bytes, T& o_string)
{
	static constexpr const typename T::value_type* ToHex = HexTraits<typename T::value_type>::value;

	o_string.resize(2 * i_bytes.size());
	for (size_t i = 0; i < i_bytes.size(); i++)
	{
		uint8_t b = i_bytes[i];
		o_string[2 * i] = ToHex[b >> 4];
		o_string[2 * i + 1] = ToHex[b & 0xf];
	}
}

bool decode_hex_string(const std::string& i_string, std::vector<uint8_t>& o_bytes)
{
	return internal_decode_hex_string(i_string, o_bytes);
}

bool decode_hex_string(const std::wstring& i_string, std::vector<uint8_t>& o_bytes)
{
	return internal_decode_hex_string(i_string, o_bytes);
}

void encode_hex_string(const std::vector<uint8_t>& i_bytes, std::string& o_string)
{
	internal_encode_hex_string(i_bytes, o_string);
}

void encode_hex_string(const std::vector<uint8_t>& i_bytes, std::wstring& o_string)
{
	internal_encode_hex_string(i_bytes, o_string);
}
