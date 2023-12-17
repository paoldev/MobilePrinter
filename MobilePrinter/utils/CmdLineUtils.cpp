#include "pch.h"
#include "CmdLineUtils.h"
#include <locale>
#include <charconv>

namespace cmdline
{
	namespace details
	{
		//Some char-type dependent traits
/*
		template<typename T> struct traits2 : std::char_traits<T>
		{
			static const std::locale& get_locale() { static std::locale s_locale; return s_locale; }	//Hack

			static auto isspace(T ch) { return std::isspace(ch, get_locale()); }

			//See stricmp.cpp
			static int stricmp(const T* lhs_ptr, const T* rhs_ptr)
			{
				const std::locale& loc = get_locale();

				int result;
				int lhs_value;
				int rhs_value;
				do
				{
					lhs_value = std::tolower(*lhs_ptr++, loc);
					rhs_value = std::tolower(*rhs_ptr++, loc);
					result = lhs_value - rhs_value;
				}
				while (result == 0 && lhs_value != 0);
				return result;
			}

			//See strnicmp.cpp
			static int strnicmp(const T* lhs_ptr, const T* rhs_ptr, const size_t count)
			{
				if (count == 0)
				{
					return 0;
				}

				const std::locale& loc = get_locale();

				int result;
				int lhs_value;
				int rhs_value;
				size_t remaining = count;
				do
				{
					lhs_value = std::tolower(*lhs_ptr++, loc);
					rhs_value = std::tolower(*rhs_ptr++, loc);
					result = lhs_value - rhs_value;
				} while (result == 0 && lhs_value != 0 && --remaining != 0);
				return result;
			}

			static long long strtoll(const T* _String, T** _EndPtr, int _Radix)
			{ 
				return from_chars<long long>(_String, _EndPtr, _Radix);
			}

			static unsigned long long strtoull(const T* _String, T** _EndPtr, int _Radix)
			{
				return from_chars<unsigned long long>(_String, _EndPtr, _Radix);
			}

			template<typename ValueType>
			static ValueType from_chars(const T* _String, T** _EndPtr, int _Radix)
			{
				while (*_String && isspace(*_String))
				{
					_String++;
				}

				if (_Radix == 0)
				{
					const T* Start = _String;
					if (*_String == '-' || *_String == '+')
					{
						_String++;
					}

					if (*_String == '0')
					{
						_Radix = ((*(_String + 1) == 'x') || (*(_String + 1) == 'X')) ? 16 : 8;
					}
					else
					{
						_Radix = 10;
					}
					_String = Start;
				}
				ValueType value = ValueType();
				//TODO: wchar_t, char16_t and char32_t "from_chars" support or equivalent implementation.
				std::from_chars_result res = std::from_chars(_String, _String + std::char_traits<T>::length(_String), value, _Radix);
				if (res.ec == std::errc::result_out_of_range)
				{
					errno = ERANGE;
				}
				else if ((int)res.ec)
				{
					value = ValueType();
				}
				if (_EndPtr)
				{
					*_EndPtr = (T*)res.ptr;
				}
				return value;
			}
		};
*/
		template<typename T> struct traits;
		template<> struct traits<char> : std::char_traits<char>
		{
			static auto stricmp(const char* s1, const char* s2) { return ::_stricmp(s1, s2); }
			static auto strnicmp(const char* s1, const char* s2, const size_t n) { return ::_strnicmp(s1, s2, n); }
			static auto strtoll(const char* _String, char** _EndPtr, int _Radix) { return ::strtoll(_String, _EndPtr, _Radix); }
			static auto strtoull(const char* _String, char** _EndPtr, int _Radix) { return ::strtoull(_String, _EndPtr, _Radix); }
			static auto isspace(int ch) { return ::isspace(ch); }
		};

		template<> struct traits<wchar_t> : std::char_traits<wchar_t>
		{
			static auto stricmp(const wchar_t* s1, const wchar_t* s2) { return ::_wcsicmp(s1, s2); }
			static auto strnicmp(const wchar_t* s1, const wchar_t* s2, const size_t n) { return ::_wcsnicmp(s1, s2, n); }
			static auto strtoll(const wchar_t* _String, wchar_t** _EndPtr, int _Radix) { return ::wcstoll(_String, _EndPtr, _Radix); }
			static auto strtoull(const wchar_t* _String, wchar_t** _EndPtr, int _Radix) { return ::wcstoull(_String, _EndPtr, _Radix); }
			static auto isspace(wint_t ch) { return ::iswspace(ch); }
		};
		/*
		template<> struct traits<char16_t>
		{
			static auto strtoll(const char16_t* _String, char16_t** _EndPtr, int _Radix) { return ::strtoll(_String, _EndPtr, _Radix); }
			static auto strtoull(const char16_t* _String, char16_t** _EndPtr, int _Radix) { return ::strtoull(_String, _EndPtr, _Radix); }
		};*/

#define STRING_TRAIT(name, val)	\
		template<typename T> struct name {};	\
		template<> struct name<char> { static constexpr const char* value = val; static constexpr size_t size = sizeof(val)/sizeof(val[0])-1; };	\
		template<> struct name<wchar_t> { static constexpr const wchar_t* value = L ## val; static constexpr size_t size = sizeof(val)/sizeof(val[0])-1; };	\
		template<> struct name<char16_t> { static constexpr const char16_t* value = u ## val; static constexpr size_t size = sizeof(val)/sizeof(val[0])-1; };	\
		template<> struct name<char32_t> { static constexpr const char32_t* value = U ## val; static constexpr size_t size = sizeof(val)/sizeof(val[0])-1; }

#define CHAR_TRAIT(name, val)	\
		template<typename T> struct name {};	\
		template<> struct name<char> { static constexpr char value = val; };	\
		template<> struct name<wchar_t> { static constexpr wchar_t value = L ## val; }; \
		template<> struct name<char16_t> { static constexpr char16_t value = u ## val; }; \
		template<> struct name<char32_t> { static constexpr char32_t value = U ## val; }

		STRING_TRAIT(TrueString, "true");
		STRING_TRAIT(FalseString, "false");
		STRING_TRAIT(ZeroString, "0");
		STRING_TRAIT(OneString, "1");
		STRING_TRAIT(NoString, "no-");
		STRING_TRAIT(HyphenNoString, "-no-");
		CHAR_TRAIT(HyphenChar, '-');
		CHAR_TRAIT(EqualChar, '=');

#undef CHAR_TRAIT
#undef STRING_TRAIT


		template<typename T>
		bool are_args_valid(const T* argv, const T* param)
		{
			//nullptr params are invalid
			if (!argv || !param)
			{
				return false;
			}

			//empty strings are invalid
			if (!argv[0] || !param[0])
			{
				return false;
			}

			return true;
		}

		template<typename T>
		bool ParserHelper<T>::has_param_prefix(const T* argv, const T* param, const T** o_pValue)
		{
			if (!are_args_valid(argv, param))
			{
				return false;
			}

			//check if "argv" starts with "param" string (case-insensitive)
			const size_t len = traits<T>::length(param);
			if ((len > 0) && (traits<T>::strnicmp(argv, param, len) == 0))
			{
				if (o_pValue)
				{
					//return the pointer to the first char after "param" string
					*o_pValue = argv + len;
				}
				return true;
			}

			return false;
		}

		template<typename T>
		bool ParserHelper<T>::has_param_value(const T* argv, const T* param, const T** o_pValue)
		{
			//check if "argv" starts with "param" string (case-insensitive)
			const T* pPrefix = nullptr;
			if (!has_param_prefix(argv, param, &pPrefix))
			{
				return false;
			}

			//check if pPrefix[0]=='='
			if (pPrefix[0] != EqualChar<T>::value)
			{
				//if ((pPrefix > argv) && (pPrefix[-1] == traits<T>::Equal))
				//{
				//	pPrefix--;
				//}
				//else
				{
					return false;
				}
			}

			if (o_pValue)
			{
				//return the pointer to the first char after "param=" string
				*o_pValue = pPrefix + 1;
			}
			return true;
		}

		template<typename T>
		bool ParserHelper<T>::is_bool_switch(const T* argv, const T* param, bool* o_pValue)
		{
			if (!are_args_valid(argv, param))
			{
				return false;
			}

			//check if "argv" is equal to "param" string (case-insensitive)
			if (traits<T>::stricmp(argv, param) == 0)
			{
				if (o_pValue)
				{
					//"param" is set to true
					*o_pValue = true;
				}
				return true;
			}
			//check if "argv" is equal to "-no-param" string, if "param" starts with '-' (case-insensitive)
			else if ((param[0] == HyphenChar<T>::value) && (traits<T>::strnicmp(argv, HyphenNoString<T>::value, HyphenNoString<T>::size) == 0))
			{
				if (traits<T>::stricmp(argv + HyphenNoString<T>::size, param + 1) == 0)
				{
					if (o_pValue)
					{
						//"param" is set to false
						*o_pValue = false;
					}
					return true;
				}
			}
			//check if "argv" is equal to "no-param" string, if "param" doesn't start with '-' (case-insensitive)
			else if ((param[0] != HyphenChar<T>::value) && (traits<T>::strnicmp(argv, NoString<T>::value, NoString<T>::size) == 0))
			{
				if (traits<T>::stricmp(argv + NoString<T>::size, param) == 0)
				{
					if (o_pValue)
					{
						*o_pValue = false;
					}
					return true;
				}
			}
			//else if (traits<T>::strnicmp(param, HyphenNoString<T>::value, HyphenNoString<T>::size) == 0)
			//{
			//	//3 instead of 4: include "-" from "argv".
			//	if (traits<T>::stricmp(argv, param + HyphenNoString<T>::size - 1) == 0)
			//	{
			//		if (o_pValue)
			//		{
			//			*o_pValue = false;
			//		}
			//		return true;
			//	}
			//}

			return false;
		}

		template<typename T>
		bool ParserHelper<T>::has_bool_param(const T* argv, const T* param, bool* o_pValue)
		{
			if (!are_args_valid(argv, param))
			{
				return false;
			}

			if (is_bool_switch(argv, param, o_pValue))
			{
				return true;
			}

			const T* pValue = nullptr;
			if (!has_param_value(argv, param, &pValue))
			{
				return false;
			}

			//check if pValue is equal to "true" or "1" (case-insensitive)
			if ((traits<T>::stricmp(pValue, TrueString<T>::value) == 0) || (traits<T>::stricmp(pValue, OneString<T>::value) == 0))
			{
				if (o_pValue)
				{
					*o_pValue = true;
				}
				return true;
			}
			//check if pValue is equal to "false" or "0" (case-insensitive)
			else if ((traits<T>::stricmp(pValue, FalseString<T>::value) == 0) || (traits<T>::stricmp(pValue, ZeroString<T>::value) == 0))
			{
				if (o_pValue)
				{
					*o_pValue = false;
				}
				return true;
			}

			return false;
		}

		template<typename T>
		bool ParserHelper<T>::has_int64_param(const T* argv, const T* param, int64_t* o_pValue)
		{
			const T* pValue = nullptr;
			if (!has_param_value(argv, param, &pValue))
			{
				return false;
			}

			T* pEndValue = nullptr;
			int64_t value = traits<T>::strtoll(pValue, &pEndValue, 0);
			if (errno == ERANGE)
			{
				//if out-of-range, reset "errno" and return error
				errno = 0;
				return false;
			}
			if (value == 0)
			{
				//value non converted.
				if (pValue == pEndValue)
				{
					return false;
				}

				//trim white space from beginning of the string; if another char is found, return error.
				while (pValue != pEndValue)
				{
					if (!traits<T>::isspace(*pValue))
					{
						return false;
					}

					pValue++;
				}
			}

			//trim white space at the end of the string; if another char is found, return error.
			while (*pEndValue)
			{
				if (!traits<T>::isspace(*pEndValue))
				{
					return false;
				}
				pEndValue++;
			}

			if (o_pValue)
			{
				//return value
				*o_pValue = value;
			}

			return true;
		}

		template<typename T>
		bool ParserHelper<T>::has_uint64_param(const T* argv, const T* param, uint64_t* o_pValue)
		{
			const T* pValue = nullptr;
			if (!has_param_value(argv, param, &pValue))
			{
				return false;
			}

			//trim white space from beginning of the string
			while ((*pValue) && traits<T>::isspace(*pValue))
			{
				pValue++;
			}

			//if '-' is the first char, this isn't an unsigned type.
			if ((*pValue) == HyphenChar<T>::value)
			{
				return false;
			}

			T* pEndValue = nullptr;
			uint64_t value = traits<T>::strtoull(pValue, &pEndValue, 0);
			if (errno == ERANGE)
			{
				//if out-of-range, reset "errno" and return error
				errno = 0;
				return false;
			}
			if (value == 0)
			{
				//value non converted.
				if (pValue == pEndValue)
				{
					return false;
				}

				//trim white space from beginning of the string; if another char is found, return error.
				while (pValue != pEndValue)
				{
					if (!traits<T>::isspace(*pValue))
					{
						return false;
					}

					pValue++;
				}
			}

			//trim white space at the end of the string; if another char is found, return error.
			while (*pEndValue)
			{
				if (!traits<T>::isspace(*pEndValue))
				{
					return false;
				}
				pEndValue++;
			}

			if (o_pValue)
			{
				//return value
				*o_pValue = value;
			}

			return true;
		}

		template<typename T>
		bool ParserHelper<T>::has_int32_param(const T* argv, const T* param, int32_t* o_pValue)
		{
			int64_t value = 0;
			if (!has_int64_param(argv, param, &value))
			{
				return false;
			}

			//check if valid int64 value is out of range as int32.
			if ((value < LONG_MIN) || (value > LONG_MAX))
			{
				return false;
			}

			if (o_pValue)
			{
				*o_pValue = static_cast<int32_t>(value);
			}

			return true;
		}

		template<typename T>
		bool ParserHelper<T>::has_uint32_param(const T* argv, const T* param, uint32_t* o_pValue)
		{
			uint64_t value = 0;
			if (!has_uint64_param(argv, param, &value))
			{
				return false;
			}

			//check if valid uint64 value is out of range as uint32.
			if (value > ULONG_MAX)
			{
				return false;
			}

			if (o_pValue)
			{
				*o_pValue = static_cast<uint32_t>(value);
			}

			return true;
		}

		template struct ParserHelper<char>;
		template struct ParserHelper<wchar_t>;

	}	//namespace details


#ifdef _DEBUG

	bool TestCmdLineParam()
	{
#define COND_FUNC(funcName, str1, str2, param3)	\
	assert(funcName(str1, str2, param3));	\
	assert(funcName(L"" str1, L"" str2, param3))
	
#define COND_FUNC_AND(funcName, str1, str2, param3, additionalcondition)	\
	assert(funcName(str1, str2, param3) && additionalcondition);	\
	assert(funcName(L"" str1, L"" str2, param3) && additionalcondition)

		char16_t ttt = u'A';
		char16_t cc = std::tolower(ttt, std::locale());
/*
		std::string_view sview(" -10 ");
		const char* as = " -10 ";
		char* end_;
		long long value = 0;
		value = details::traits2<char>::strtoll(as, &end_, 0);
		assert(value == details::traits<char>::strtoll(as, &end_, 0));
*/
		const char16_t* s1 = u"TEST57";
		const char16_t* s2 = u"TEST";
		bool rr = std::equal(s1, s1 + std::char_traits<char16_t>::length(s1), s2, [loc = std::locale()](auto ch1, auto ch2) { return std::tolower(ch1, loc) == std::tolower(ch2, loc); });
		int64_t aaa = wcstoll(L"10", nullptr, 10);
		uint64_t aaas = wcstoll(L"10", nullptr, 10);

		bool bb = false;
		int32_t tt = 0;
		int64_t tt2 = 0;
		uint32_t tt3 = 0;
		uint64_t tt4 = 0;
		COND_FUNC(!cmdline::is_bool_switch, "-test", "-test2", &bb);
		COND_FUNC_AND(cmdline::is_bool_switch, "-no-test", "-test", &bb, (!bb));
		COND_FUNC(!cmdline::is_bool_switch, "-test", "-test2", &bb);
		COND_FUNC_AND(cmdline::is_bool_switch, "-test", "-test", &bb, bb);
		COND_FUNC_AND(cmdline::is_bool_switch, "-no-test", "-test", &bb, (!bb));
		COND_FUNC_AND(cmdline::is_bool_switch, "test", "test", &bb, bb);
		COND_FUNC_AND(cmdline::is_bool_switch, "no-test", "test", &bb, (!bb));
		COND_FUNC_AND(cmdline::has_bool_param, "-test", "-test", &bb, bb);
		COND_FUNC_AND(cmdline::has_bool_param, "-no-test", "-test", &bb, (!bb));
		COND_FUNC_AND(cmdline::has_bool_param, "test", "test", &bb, bb);
		COND_FUNC_AND(cmdline::has_bool_param, "no-test", "test", &bb, (!bb));
		COND_FUNC_AND(cmdline::has_bool_param, "-test=true", "-test", &bb, bb);
		COND_FUNC_AND(cmdline::has_bool_param, "-test=false", "-test", &bb, (!bb));
		COND_FUNC_AND(cmdline::has_bool_param, "test=1", "test", &bb, bb);
		COND_FUNC_AND(cmdline::has_bool_param, "test=0", "test", &bb, (!bb));
		COND_FUNC(!cmdline::has_bool_param, "-test=value2", "-test", &bb);
		COND_FUNC_AND(cmdline::has_int32_param, "-test= -10 ", "-test", &tt, (tt == -10));
		COND_FUNC(!cmdline::has_int32_param, "-test= -10asd ", "-test", &tt);
		COND_FUNC(!cmdline::has_int64_param, "-test=asd -10 ", "-test", &tt2);
		COND_FUNC_AND(cmdline::has_int64_param, "-test= -10 ", "-test", &tt2, (tt2 == -10));
		COND_FUNC(!cmdline::has_uint32_param, "-test= -10 ", "-test", &tt3);
		COND_FUNC(!cmdline::has_uint64_param, "-test= -10 ", "-test", &tt4);
		COND_FUNC(!cmdline::has_int32_param, "-test= asd -10 ", "-test", &tt);
		COND_FUNC(!cmdline::has_int64_param, "-test= -10000000000000000000000000000000000000000000000000 ", "-test", &tt2);
		COND_FUNC(!cmdline::has_uint32_param, "-test= 10000000000000000000000000000000000000000000000000000000 ", "-test", &tt3);
		COND_FUNC_AND(cmdline::has_uint64_param, "-test= 10 ", "-test", &tt4, (tt4 == 10));
		COND_FUNC_AND(cmdline::has_int32_param, "-test= 0x12345678 ", "-test", &tt, (tt == 305419896));
		
#undef COND_FUNC_AND
#undef COND_FUNC

		return true;
	}

	static bool bTest = TestCmdLineParam();

#endif	//_DEBUG

}	//cmdline
