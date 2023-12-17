#pragma once

namespace cmdline
{
	namespace details
	{
		template<typename T>
		struct ParserHelper
		{
			static bool has_param_prefix(const T* argv, const T* param, const T** o_pValue);
			static bool has_param_value(const T* argv, const T* param, const T** o_pValue);
			static bool is_bool_switch(const T* argv, const T* param, bool* o_pValue);
			static bool has_bool_param(const T* argv, const T* param, bool* o_pValue);
			static bool has_int32_param(const T* argv, const T* param, int32_t* o_pValue);
			static bool has_int64_param(const T* argv, const T* param, int64_t* o_pValue);
			static bool has_uint32_param(const T* argv, const T* param, uint32_t* o_pValue);
			static bool has_uint64_param(const T* argv, const T* param, uint64_t* o_pValue);
		};

		extern template struct ParserHelper<char>;
		extern template struct ParserHelper<wchar_t>;
	}

	/// <summary>
	/// Check if "argv" starts with "param" string.
	/// If o_pValue is not a null pointer, the function also sets the value of o_pValue to point to the first character after the "param" string.
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline argument.</param>
	/// <param name="param">The prefix to search for in "argv".</param>
	/// <param name="o_pValue">Reference to an object of type const T*, whose value is set by the function to the next character in "argv" after the "param" string.
	//This parameter can also be a null pointer, in which case it is not used.</param>
	/// <returns>true if "argv" starts with "param" string.</returns>
	template<typename T>
	bool has_param_prefix(const T* argv, const T* param, const T** o_pValue) { return details::ParserHelper<T>::has_param_prefix(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" starts with "param=" string.
	/// If o_pValue is not a null pointer, the function also sets the value of o_pValue to point to the first character after the "param=" string.
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline argument.</param>
	/// <param name="param">The parameter to search for in "argv". Don't specify the "=" sign here.</param>
	/// <param name="o_pValue">Reference to an object of type const T*, whose value is set by the function to the next character in "argv" after the "param=" string.
	//This parameter can also be a null pointer, in which case it is not used.</param>
	/// <returns>true if "argv" starts with "param=" string.</returns>
	template<typename T>
	bool has_param_value(const T* argv, const T* param, const T** o_pValue) { return details::ParserHelper<T>::has_param_value(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as a boolean.
	/// "param" represents the "true" value.
	/// If "param" starts with "-" sign, "-no-param" is automatically associated to the "false" value (removing the "-" sign of "param").
	/// If "param" doesn't start with "-" sign, "no-param" is automatically associated to the "false" value.
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the "true" value.</param>
	/// <param name="o_pValue">The optional boolean value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the boolean value represented by "param".</returns>
	template<typename T>
	bool is_bool_switch(const T* argv, const T* param, bool* o_pValue) { return details::ParserHelper<T>::is_bool_switch(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as a boolean.
	/// This function checks if "argv" is a bool switch (see is_bool_switch) or if "argv" is equal to "param=[true,1]" or "param=[false,0]" string.
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the boolean value.</param>
	/// <param name="o_pValue">The optional boolean value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the boolean value represented by "param", "-noparam", "param=[true,1]" or "param=[false,0]".</returns>
	template<typename T>
	bool has_bool_param(const T* argv, const T* param, bool* o_pValue) { return details::ParserHelper<T>::has_bool_param(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as an int32.
	/// This function checks if "argv" is equal to "param=[int32 number]".
	/// The number can have decimal base, hexadecimal base (if it starts with 0x or 0X) or octal base (if it starts with 0).
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the int32 value.</param>
	/// <param name="o_pValue">The optional int32 value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the int32 value represented by "param=[int32 number]" string.</returns>
	template<typename T>
	bool has_int32_param(const T* argv, const T* param, int32_t* o_pValue) { return details::ParserHelper<T>::has_int32_param(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as an int64.
	/// This function checks if "argv" is equal to "param=[int64 number]".
	/// The number can have decimal base, hexadecimal base (if it starts with 0x or 0X) or octal base (if it starts with 0).
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the int64 value.</param>
	/// <param name="o_pValue">The optional int64 value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the int64 value represented by "param=[int64 number]" string.</returns>
	template<typename T>
	bool has_int64_param(const T* argv, const T* param, int64_t* o_pValue) { return details::ParserHelper<T>::has_int64_param(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as an uint32.
	/// This function checks if "argv" is equal to "param=[uint32 number]".
	/// The number can have decimal base, hexadecimal base (if it starts with 0x or 0X) or octal base (if it starts with 0).
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the uint32 value.</param>
	/// <param name="o_pValue">The optional uint32 value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the uint32 value represented by "param=[uint32 number]" string.</returns>
	template<typename T>
	bool has_uint32_param(const T* argv, const T* param, uint32_t* o_pValue) { return details::ParserHelper<T>::has_uint32_param(argv, param, o_pValue); }

	/// <summary>
	/// Check if "argv" value can be interpreted as an uint64.
	/// This function checks if "argv" is equal to "param=[uint64 number]".
	/// The number can have decimal base, hexadecimal base (if it starts with 0x or 0X) or octal base (if it starts with 0).
	/// </summary>
	/// <typeparam name="T">Supported types: char, wchar_t</typeparam>
	/// <param name="argv">The commandline arguments.</param>
	/// <param name="param">The parameter representing the uint64 value.</param>
	/// <param name="o_pValue">The optional uint64 value if the parameter is valid.</param>
	/// <returns>true if "argv" can be safely converted to the uint64 value represented by "param=[uint64 number]" string.</returns>
	template<typename T>
	bool has_uint64_param(const T* argv, const T* param, uint64_t* o_pValue) { return details::ParserHelper<T>::has_uint64_param(argv, param, o_pValue); }

}	//cmdline
