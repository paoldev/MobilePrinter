#pragma once

#include "ipp_variant.h"
#include "ipp_operations.h"
#include <cpprest/http_msg.h>

namespace ipp
{
	class basetype
	{
	public:
		basetype(int32_t i_tag, bool i_array) : m_tag(i_tag), m_array(i_array)
		{
		}

		virtual ~basetype()
		{
		}

		int32_t getTag() const
		{
			return m_tag;
		}

		bool isArray() const
		{
			return m_array;
		}

	private:
		int32_t m_tag;
		bool m_array;
	};

	template<const int TAG, const bool ARRAY>
	class booltype : public basetype
	{
	public:
		booltype() : basetype(TAG, ARRAY)
		{
		}

		static bool validate(bool value)
		{
			return true;
		}
	};

	template<const int TAG, const int MIN, const int MAX, const bool ARRAY>
	class inttype : public basetype
	{
	public:
		inttype() : basetype(TAG, ARRAY)
		{
		}

		static bool validate(int32_t value)
		{
			return (value >= MIN) && (value <= MAX);
		}
	};

	template<const int TAG, const int MAX, const bool ARRAY>
	class stringenum : public basetype
	{
	public:
		stringenum(const char** i_periods_enum, size_t i_num_enums) : basetype(TAG, ARRAY), m_periods_enum(i_periods_enum), num_enums(i_num_enums)
		{

		}

		bool validate(const std::string& value) const
		{
			if (static_cast<int32_t>(value.size()) <= MAX)
			{
				for (size_t i = 0; i < num_enums; i++)
				{
					if (value.compare(m_periods_enum[i]) == 0)
					{
						return true;
					}
				}
			}
			return false;
		}
	private:
		const char** m_periods_enum;
		size_t num_enums;
	};

	template<const int TAG, const bool ARRAY>
	class intenum : public basetype
	{
		const int32_t MIN = 1;
		const int32_t MAX = INT_MAX;
	public:

		intenum(const std::pair<int, const char*>* i_periods_enum, size_t i_num_enums) : basetype(TAG, ARRAY), m_enum(i_periods_enum), num_enums(i_num_enums)
		{

		}

		bool validate(const int32_t value) const
		{
			if ((value < MIN) || (value > MAX))
			{
				return false;
			}

			for (size_t i = 0; i < num_enums; i++)
			{
				if (m_enum[i].value == value)
				{
					return true;
				}
			}
			
			return false;
		}

	private:
		const std::pair<int, const char*>* m_enum;
		size_t num_enums;
	};

	template<const int TAG, const int MIN, const int MAX, const bool ARRAY>
	class intrange : public basetype
	{
	public:
		intrange() : basetype(TAG, ARRAY)
		{

		}

		bool validate(const int32_t value) const
		{
			return (value >= MIN) && (value <= MAX);
		}
	};

	template<const int TAG, const bool ARRAY>
	class resolutiontype : public basetype
	{
		const int32_t MIN = 1;
		const int32_t MAX = INT_MAX;

	public:
		resolutiontype() : basetype(TAG, ARRAY)
		{

		}
	};

	template<const int TAG, const bool ARRAY>
	class datetimetype : public basetype
	{
	public:
		datetimetype() : basetype(TAG, ARRAY)
		{

		}
	};

	template<const int TAG, const int MAX, const bool ARRAY>
	class stringtype : public basetype
	{
	public:
		stringtype() : basetype(TAG, ARRAY)
		{

		}

		bool validate(const std::string& value) const
		{
			if (value.size() <= MAX)
			{
				switch (getTag())
				{
				case tags::nameWithoutLanguage:
				case tags::nameWithLanguage:
					return true;

				case tags::textWithoutLanguage:
				case tags::textWithLanguage:
					return true;

				case tags::uri:
					return true;

				case tags::uriScheme:
					return true;

				case tags::charset:
					return true;

				case tags::naturalLanguage:
					return true;

				case tags::mimeMediaType:
					return true;

				default:
					break;
				}
			}

			return false;
		}
	};

	class collectiontype : public basetype
	{
	public:
		collectiontype() : basetype(tags::begCollection, false)
		{

		}
	};


	
	class packet
	{
	public:

		packet();
		~packet() = default;

		bool parse(const uint8_t* pData, const size_t datasize);
		bool build();

		void SetVersion(const int32_t i_major, int i_minor)
		{
			m_version_major = i_major;
			m_version_minor = i_minor;
		}

		void SetRequestId(const int32_t i_request)
		{
			m_request_id = i_request;
		}

		void SetData(const std::vector<uint8_t>& i_data)
		{
			m_ipp_data = i_data;
		}

		void SetAttribute(const int32_t i_group, const std::string& i_attribute, const variant& i_value);

		template<typename T>
		void SetAttribute(const int32_t i_group, const std::string& i_attribute, const T& i_value)
		{
			SetAttribute(i_group, i_attribute, variant(i_value));
		}

		typedef std::multimap<int32_t, collection>::iterator group_it;
		group_it AddGroup(const int32_t i_group);
		void SetAttribute(group_it i_group, const std::string& i_attribute, const variant& i_value);
		void SetAttribute(group_it i_group, const std::string& i_attribute, const std::vector<variant>& i_values);
		template<typename T>
		void SetAttribute(group_it i_group, const std::string& i_attribute, const T& i_value)
		{
			SetAttribute(i_group, i_attribute, variant(i_value));
		}
		template<typename T>
		void SetAttribute(group_it i_group, const std::string& i_attribute, const std::vector<T>& i_values)
		{
			for (auto it = i_values.cbegin(); it != i_values.end(); it++)
			{
				SetAttribute(i_group, i_attribute, variant(*it));
			}
		}
		

		int32_t GetIppMajor() const
		{
			return m_version_major;
		}

		int32_t GetIppMinor() const
		{
			return m_version_minor;
		}

		int32_t GetRequestId() const
		{
			return m_request_id;
		}

		const std::vector<uint8_t>& GetData() const
		{
			return m_ipp_data;
		}

		const std::multimap<int32_t, collection>& attributes() const
		{
			return m_ipp_attributes;
		}

	protected:

		int32_t GetOperationIdOrStatusCode() const
		{
			return m_operation_or_status_code;
		}

		void SetOperationIdOrStatusCode(const int32_t i_operation_or_status_code)
		{
			m_operation_or_status_code = i_operation_or_status_code;
		}

		bool are_groups_valid() const;

	private:

		//parser
		int32_t read1()
		{
			if ((!m_readError) && (m_position < m_datasize))
			{
				return m_data[m_position++];
			}
			m_readError = 1;
			return 0;
		}

		int32_t read2()
		{
			return (read1() << 8) | read1();
		}

		int32_t read4()
		{
			return (read2() << 16) | read2();
		}

		bool isError() const
		{
			return (m_readError != 0);
		}

		std::string read(const int32_t length, const char* encoding = nullptr);
		void read_attribute_groups();
		void read_attribute_group(group_it group);
		void read_attribute(group_it group);
		bool has_array_value();
		std::vector<variant> read_values(int32_t type, const std::string& name);
		variant read_value(const int32_t tag, const std::string& name);
		collection read_collection();
		std::vector<variant> read_collection_member(const std::string& name);
		

		//builder
		void append(size_t n);
		void write1(int32_t value);
		void write2(int32_t value);
		void write4(int32_t value);
		void write(const char* str, const int32_t length, bool writelenght, const char* encoding = nullptr);
		void write(const std::string& str, bool writelenght, const char* encoding = nullptr);
		void write_attribute_groups();
		void write_attribute_group(group_it group);
		void write_variant(int32_t tag, const variant& value);
		void write_values(const std::string& name, const std::vector<variant>& values);
		void write_first_value(int32_t tag, const std::string& name, const variant& value);
		void write_new_array_value(int32_t tag, const std::string& name, const variant& value);
		int32_t todo_tag(const std::string& key, const variant& value);

	private:

		int32_t m_version_major;
		int32_t m_version_minor;
		int32_t m_operation_or_status_code;
		int32_t m_request_id;
		std::vector<uint8_t> m_ipp_data;
		std::multimap<int32_t, collection> m_ipp_attributes;

		//parser state
		int32_t m_readError = 0;
		size_t m_position = 0;
		size_t m_datasize = 0;
		const uint8_t* m_data = nullptr;
		std::vector<int32_t> m_groups_from_request;
	};

	class response : public packet
	{
	public:
		response();

		int32_t GetStatusCode() const
		{
			return GetOperationIdOrStatusCode();
		}

		void SetStatusCode(const int32_t i_operation_id)
		{
			SetOperationIdOrStatusCode(i_operation_id);
		}
	};

	class request : public packet
	{
	public:
		request();

		int32_t GetOperationId() const
		{
			return GetOperationIdOrStatusCode();
		}

		void SetOperationId(const int32_t i_operation_id)
		{
			SetOperationIdOrStatusCode(i_operation_id);
		}

		void set_http_request(web::http::http_request i_http_request)
		{
			m_http_request = i_http_request;
		}

		template<typename REQUEST>
		int32_t to_request(REQUEST& o_request) const;

		template<typename RESPONSE>
		pplx::task<void> reply(const int32_t i_status_code, const RESPONSE& i_response) const;

	private:

		pplx::task<void> reply(const int32_t i_status_code, response& i_response) const;

		web::http::http_request m_http_request;
	};
}
