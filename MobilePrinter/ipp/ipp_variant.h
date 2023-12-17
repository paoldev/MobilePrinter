#pragma once

namespace ipp
{
	class variant;

	enum resolution_types
	{
		dot_per_inch = 3,
		dot_per_centimeter = 4
	};

	typedef std::pair<int32_t, int32_t> range_of_integers;
	typedef std::tuple<int32_t, int32_t, int32_t> resolutions;
	typedef std::map<std::string, std::vector<variant>> collection;
	struct string_with_language : public std::pair<std::string, std::string>
	{
		string_with_language() : std::pair<std::string, std::string>()
		{
		}

		string_with_language(const std::string& text, const std::string& language) : std::pair<std::string, std::string>(text, language)
		{
		}

		string_with_language(const char* text, const char* language) : std::pair<std::string, std::string>(text, language)
		{
		}

		const std::string& text() const
		{
			return first;
		}

		const std::string& language() const
		{
			return second;
		}
	};

	struct date_time
	{
		bool isPositive;
		uint32_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		uint32_t millisecond;
		bool TZIsLocal;
		bool TZIsPositive;
		uint8_t TZHour;
		uint8_t TZMinute;
	};

	inline bool operator==(const date_time& dt1, const date_time& dt2)
	{
		if (dt1.isPositive != dt2.isPositive) return false;
		if (dt1.year != dt2.year) return false;
		if (dt1.month != dt2.month) return false;
		if (dt1.day != dt2.day) return false;
		if (dt1.hour != dt2.hour) return false;
		if (dt1.minute != dt2.minute) return false;
		if (dt1.second != dt2.second) return false;
		if (dt1.millisecond != dt2.millisecond) return false;
		if (dt1.TZIsLocal != dt2.TZIsLocal) return false;
		if (dt1.TZIsPositive != dt2.TZIsPositive) return false;
		if (dt1.TZHour != dt2.TZHour) return false;
		if (dt1.TZMinute != dt2.TZMinute) return false;
		return true;
	}

	class variant
	{
	public:

		enum unsupported_t { unsupported };
		enum unknown_t { unknown };
		enum novalue_t { novalue };

		enum Type
		{
			Uninitialized, Int32, Bool, Int32Range, Resolutions, DateTime, String, StringWithLanguage, Collection, Unsupported, Unknown, NoValue
		};

		variant() : m_type(Uninitialized)
		{
		}

		explicit variant(const unsupported_t value) : m_type(Unsupported)
		{
		}

		explicit variant(const unknown_t value) : m_type(Unknown)
		{
		}

		explicit variant(const novalue_t value) : m_type(NoValue)
		{
		}

		explicit variant(const int32_t value) : m_type(Int32), m_i(value)
		{
		}

		explicit variant(const bool value) : m_type(Bool), m_b(value)
		{
		}

		explicit variant(const range_of_integers& value) : m_type(Int32Range), m_range(value)
		{
		}

		explicit variant(const resolutions& value) : m_type(Resolutions), m_resolution(value)
		{
		}

		explicit variant(const date_time& value) : m_type(DateTime), m_date(value)
		{
		}

		explicit variant(const std::string& value) : m_type(String), m_string(value, "")
		{
		}

		explicit variant(const char* value) : m_type(String), m_string(value ? value : "", "")
		{
		}

		explicit variant(const std::string& value, const std::string& language) : m_type(StringWithLanguage), m_string(value, language)
		{
		}

		explicit variant(const char* value, const char* language) : m_type(StringWithLanguage), m_string(value ? value : "", language ? language : "")
		{
		}

		explicit variant(const collection& value) : m_type(Collection), m_collection(value)
		{
		}

		void Reset()
		{
			m_type = Uninitialized;
			m_i = 0;
			m_b = false;
			m_range = {};
			m_resolution = {};
			m_date = {};
			m_string = {};
			m_collection.clear();
		}

		void Set(const unsupported_t)
		{
			Reset();
			m_type = Unsupported;
		}

		void Set(const unknown_t)
		{
			Reset();
			m_type = Unknown;
		}

		void Set(const novalue_t)
		{
			Reset();
			m_type = NoValue;
		}

		void Set(const int32_t value)
		{
			m_type = Int32; m_i = value;
		}
		void Set(const bool value)
		{
			m_type = Bool; m_b = value;
		}
		void Set(const range_of_integers& value)
		{
			m_type = Int32Range; m_range = value;
		}
		void Set(const resolutions& value)
		{
			m_type = Resolutions; m_resolution = value;
		}
		void Set(const date_time& value)
		{
			m_type = DateTime; m_date = value;
		}
		void Set(const std::string& value)
		{
			m_type = String; m_string.first = value; m_string.second = "";
		}
		void Set(const char* value)
		{
			m_type = String; m_string.first = value ? value : ""; m_string.second = "";
		}
		void Set(const std::string& value, const std::string& language)
		{
			m_type = StringWithLanguage; m_string.first = value; m_string.second = language;
		}
		void Set(const char* value, const char* language)
		{
			m_type = StringWithLanguage; m_string.first = value ? value : ""; m_string.second = language ? language : "";
		}
		void Set(const std::map<std::string, std::vector<variant>>& value)
		{
			m_type = Collection; m_collection = value;
		}

		Type GetType() const { return m_type; }

		int32_t GetInt32() const
		{
			assert(m_type == Int32);  return m_i;
		}
		bool GetBool() const
		{
			assert(m_type == Bool);  return m_b;
		}
		const range_of_integers& GetRange() const
		{
			assert(m_type == Int32Range);  return m_range;
		}
		const resolutions& GetResolution() const
		{
			assert(m_type == Resolutions);  return m_resolution;
		}
		const date_time& GetDate() const
		{
			assert(m_type == DateTime);  return m_date;
		}
		const std::string& GetString() const
		{
			assert(m_type == String);  return m_string.text();
		}
		const string_with_language& GetStringWithLanguage() const
		{
			assert(m_type == StringWithLanguage);  return m_string;
		}
		const std::map<std::string, std::vector<variant>>& GetCollection() const
		{
			assert(m_type == Collection);  return m_collection;
		}

		int32_t GetSize() const
		{
			switch (m_type)
			{
			case Int32: return 4;
			case Bool: return 1;
			case Int32Range: return 8;
			case Resolutions: return 9;
			case DateTime: return 11;
			case String: return static_cast<int32_t>(m_string.text().size());
			case StringWithLanguage: return static_cast<int32_t>(4 + m_string.text().size()+ m_string.language().size());
			case Collection: return 0;
			case Unsupported: return 0;
			case Unknown: return 0;
			case NoValue: return 0;
			//case Default: return 0;
			//case NotSettable: return 0;
			//case DeleteAttr: return 0;
			//case AdminDefine: return 0;
			default: assert(0); return 0;
			}
		}

		variant& operator=(const variant& i_other) = default;

		bool operator==(const variant& i_other) const
		{
			if (m_type != i_other.m_type)
			{
				return false;
			}

			switch (m_type)
			{
			case Uninitialized: return true;
			case Unsupported: return true;
			case Unknown: return true;
			case NoValue: return true;
			//case Default: return true;
			//case NotSettable: return true;
			//case DeleteAttr: return true;
			//case AdminDefine: return true;
			case Int32: return (m_i == i_other.m_i);
			case Bool: return (m_b == i_other.m_b);
			case Int32Range: return (m_range == i_other.m_range);
			case Resolutions: return (m_resolution == i_other.m_resolution);
			case DateTime: return (m_date == i_other.m_date);
			case String: return (m_string == i_other.m_string);//TODO: to_lower
			case StringWithLanguage: return (m_string == i_other.m_string);//TODO: to_lower
			case Collection: return (m_collection == i_other.m_collection);
			default: break;
			}
			assert(0 && "TODO");
			return false;
		}

	private:

		Type m_type = Uninitialized;

		int32_t m_i = 0;
		bool	m_b = false;
		range_of_integers m_range = {};
		resolutions m_resolution = {};
		date_time m_date = {};
		string_with_language m_string;
		std::map<std::string, std::vector<variant>> m_collection;
	};

	inline void collection_insert(collection& io_collection, const std::string& i_attribute, const variant& i_value)
	{
		auto it = io_collection.find(i_attribute);
		if (it == io_collection.end())
		{
			io_collection.insert(std::make_pair(i_attribute, std::vector<variant>()));
			it = io_collection.find(i_attribute);
		}

		if (it != io_collection.end())
		{
			it->second.push_back(i_value);
		}
	}

	template <typename T>
	inline void collection_insert(collection& io_collection, const std::string& i_attribute, const T& i_value)
	{
		collection_insert(io_collection, i_attribute, variant(i_value));
	}
}
