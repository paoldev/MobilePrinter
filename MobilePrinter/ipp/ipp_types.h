#pragma once

#include "ipp_gen_types_1_1.h"
#include "ipp_gen_types_2_0.h"
#include "ipp_variant.h"
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace ipp
{
	enum MAX_LIMITS
	{
		TEXT_MAX = 1023,
		NAME_MAX = 255,				//utf-8
		KEYWORD_MAX = 255,			
		CHARSET_MAX = 63,			//lowercase
		NATURAL_LANGUAGE_MAX = 63,	//lowercase
		MIME_MEDIA_TYPES_MAX = 255,	//mixedcase; case insensitive
		OCTET_STRING_MAX = 1023,
		URI_MAX = 1023,
		URI_SCHEME_MAX = 63
	};

	typedef int32_t enum_t;
	typedef std::string uri;
	typedef std::string uriScheme;
	typedef std::string keyword;
	typedef std::string mimeMediaType;
	typedef std::string naturalLanguage;

	template<const int MAX_SIZE>
	class name : public std::string
	{
	};

	template<const int MAX_SIZE>
	class text : public std::string
	{
	};

	template<const int MIN, const int MAX>
	class integer_t
	{
	public:

		integer_t(const int32_t i)
		{
			m_i = (i < MIN) ? MIN : ((i > MAX) ? MAX : i);
		}

		integer_t& operator=(const int32_t i)
		{
			m_i = (i < MIN) ? MIN : ((i > MAX) ? MAX : i);
			return *this;
		}

		operator int32_t() const
		{
			return m_i;
		}

	private:
		int32_t m_i;
	};
}
