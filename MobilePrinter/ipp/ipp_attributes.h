#pragma once

#include "ipp_types.h"

namespace ipp
{
	class variant;

	class basetype
	{
	public:
		basetype(int32_t i_tag, bool i_array, bool i_novalue, bool i_unknown) : m_tag(i_tag), m_array(i_array), m_novalue(i_novalue), m_unknown(i_unknown)
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

		bool canBeNoValue() const
		{
			return m_novalue;
		}

		bool canBeUnknown() const
		{
			return m_unknown;
		}

		virtual bool validate(const variant& i_value)
		{
			return false;
		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!i_value)
			{
				return false;
			}

			return (m_tag == i_value->m_tag) && (m_array == i_value->m_array) && (m_novalue == i_value->m_novalue) && (m_unknown == i_value->m_unknown);
		}

	private:
		int32_t m_tag;
		bool m_array;
		bool m_novalue;
		bool m_unknown;
	};

	class booltype : public basetype
	{
	public:
		booltype(bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::boolean, i_array, i_novalue, i_unknown)
		{
		}
	};

	class inttype : public basetype
	{
	public:
		inttype(const int i_min, const int i_max, bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::integer, i_array, i_novalue, i_unknown), m_min(i_min), m_max(i_max)
		{
		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			return (m_min == static_cast<const inttype*>(i_value)->m_min) && (m_max == static_cast<const inttype*>(i_value)->m_max);
		}

	private:
		int32_t m_min;
		int32_t m_max;
	};

	class keywordtype : public basetype
	{
	public:
		keywordtype(const char** i_keywords, const size_t i_num_keywords, bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::keyword, i_array, i_novalue, i_unknown), m_keywords(i_keywords), m_num_keywords(i_num_keywords)
		{

		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			return (m_keywords == static_cast<const keywordtype*>(i_value)->m_keywords) && (m_num_keywords == static_cast<const keywordtype*>(i_value)->m_num_keywords);
		}

	private:
		const char** m_keywords;
		size_t m_num_keywords;
	};

	class enumtype : public basetype
	{
		const int32_t MIN = 1;
		const int32_t MAX = INT_MAX;
	public:

		enumtype(const std::pair<int, const char*>* i_enums, const size_t i_num_enums, bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::enum_tag, i_array, i_novalue, i_unknown), m_enums(i_enums), m_num_enums(i_num_enums)
		{

		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			return (m_enums == static_cast<const enumtype*>(i_value)->m_enums) && (m_num_enums == static_cast<const enumtype*>(i_value)->m_num_enums);
		}

	private:
		const std::pair<int, const char*>* m_enums;
		size_t m_num_enums;
	};

	class intrangetype : public basetype
	{
	public:
		intrangetype(const int i_min, const int i_max, bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::rangeOfInteger, i_array, i_novalue, i_unknown), m_min(i_min), m_max(i_max)
		{
		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			return (m_min == static_cast<const intrangetype*>(i_value)->m_min) && (m_max == static_cast<const intrangetype*>(i_value)->m_max);
		}

	private:
		int32_t m_min;
		int32_t m_max;
	};

	class resolutiontype : public basetype
	{
		const int32_t MIN = 1;
		const int32_t MAX = INT_MAX;

	public:
		resolutiontype(bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::resolution, i_array, i_novalue, i_unknown)
		{

		}
	};

	class datetimetype : public basetype
	{
	public:
		datetimetype(bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::dateTime, i_array, i_novalue, i_unknown)
		{

		}
	};

	class stringtype : public basetype
	{
	public:
		stringtype(const int i_tag, const int i_max, bool i_array, bool i_novalue, bool i_unknown) : basetype(i_tag, i_array, i_novalue, i_unknown), m_max(i_max)
		{

		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			return (m_max == static_cast<const stringtype*>(i_value)->m_max);
		}

	private:
		int32_t m_max;
	};

	class collectiontype : public basetype
	{
	public:
		collectiontype(bool i_array, bool i_novalue, bool i_unknown) : basetype(tags::begCollection, i_array, i_novalue, i_unknown)
		{
		}

		void add_member(const std::string& i_name, const basetype& i_type);

		std::map<std::string, basetype*>& get_members()
		{
			return m_members;
		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			if (m_members.size() != static_cast<const collectiontype*>(i_value)->m_members.size())
			{
				return false;
			}

			for (auto it = m_members.begin(); it != m_members.end(); it++)
			{
				auto it2 = static_cast<const collectiontype*>(i_value)->m_members.find(it->first);
				if (it2 == static_cast<const collectiontype*>(i_value)->m_members.end())
				{
					return false;
				}

				if (!it->second->IsEqualTo(it2->second))
				{
					return false;
				}
			}

			return true;
		}

	private:
		std::map<std::string, basetype*> m_members;
	};

	class compositetype : public basetype
	{
	public:
		static const int32_t tag = tags::operation_attributes_tag;/*temp: this is not a valid tag*/

		compositetype(bool i_array, bool i_novalue, bool i_unknown) : basetype(tag, i_array, i_novalue, i_unknown)
		{
		}

		void add_type(const basetype& i_type);

		std::vector<int32_t> getTags() const
		{
			std::vector<int32_t> tags;
			for (size_t i = 0; i < m_types.size(); i++)
			{
				tags.push_back(m_types[i]->getTag());
			}
			return tags;
		}

		virtual bool IsEqualTo(const basetype* i_value)
		{
			if (!__super::IsEqualTo(i_value))
			{
				return false;
			}

			if (m_types.size() != static_cast<const compositetype*>(i_value)->m_types.size())
			{
				return false;
			}

			for (size_t i = 0; i < m_types.size(); i++)
			{
				if (!m_types[i]->IsEqualTo(static_cast<const compositetype*>(i_value)->m_types[i]))
				{
					return false;
				}
			}

			return true;
		}

	private:
		std::vector<basetype*> m_types;
	};

	class AttributeRegistry
	{
	public:
		static void add_boolean(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_integer(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t Min, const int32_t Max, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_rangeOfInteger(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t Min, const int32_t Max, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_resolution(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_datetime(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_string(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t MaxSize, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
		static void add_keyword(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t MaxSize, const bool IsArray, const bool IsNoValue, const bool IsUnknown, const char** Keywords, const size_t NumKeywords);
		static void add_enum(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown, const std::pair<int32_t, const char*>* Enums, const size_t NumEnums);
		static void add_collection(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown);
	};
}
