#include "pch.h"
#include "ipp_attributes.h"
#include "CommonUtils.h"

namespace ipp
{
	extern void init_attributes();
	bool setup_attributes()
	{
		init_attributes();
		return true;
	}

	static bool s_init_attributes = setup_attributes();

	std::map<std::string, basetype*>& get_attributes()
	{
		static std::map<std::string, basetype*> s_attributes;
		return s_attributes;
	}

	std::map<std::string, basetype*>& get_global_attributes()
	{
		static std::map<std::string, basetype*> s_global_attributes;
		return s_global_attributes;
	}

	std::map<std::string, basetype*>& get_attributes(const char* RootName, const char* AttributeName)
	{
		std::string fullpath = std::string(RootName) + "." + std::string(AttributeName);
		std::vector<std::string> path = split(fullpath, '.', true);
		std::map<std::string, basetype*>& db = get_attributes();

		if (path.size())
		{
			//Remove the attribute name.
			path.pop_back();
		}

		for (size_t i = 0; i < path.size(); i++)
		{
			auto it = db.find(path.at(i));
			if (it == db.end())
			{
				//This should happen on RootName only; in all other cases, AttributeRegistry::add_collection should be called instead.
				it = db.insert(std::make_pair(path.at(i), new collectiontype(false, false, false))).first;
			}
			assert(it->second->getTag() == tags::begCollection);
			db = static_cast<collectiontype*>(it->second)->get_members();
		}
		return db;
	}

	std::pair<std::string, basetype*> make_attribute_pair(const char* AttributeName, basetype* Type)
	{
		std::string ShortAttributeName;
		std::vector<std::string> v = split(AttributeName, '.', true);
		if (v.size())
		{
			ShortAttributeName = v.back();
		}

		return std::make_pair(ShortAttributeName, Type);
	}

	void insert_attribute(const char* RootName, const char* AttributeName, basetype* Type)
	{
		auto AttributeDesc = make_attribute_pair(AttributeName, Type);
		get_attributes(RootName, AttributeName).insert(AttributeDesc);

		//AttributeDesc.first = std::string(RootName) + "." + std::string(AttributeName);
		auto& globals = get_global_attributes();
		auto it = globals.find(AttributeDesc.first);
		if (it != globals.end())
		{
			if (!it->second->IsEqualTo(Type))
			{
				DBGLOG("Conflicting types for %s.%s (A: %d, NV: %d, UNK: %d)\n", RootName, AttributeName, int(it->second->isArray() != Type->isArray()), int(it->second->canBeNoValue() != Type->canBeNoValue()), int(it->second->canBeUnknown() != Type->canBeUnknown()));
			}
		}
		else
		{
			globals.insert(AttributeDesc);
		}
	}

	std::vector<int32_t> find_attribute_tag(const char* AttributeName)
	{
		std::vector<int32_t> tags;
		auto& globals = get_global_attributes();
		auto it = globals.find(AttributeName);
		if (it != globals.end())
		{
			if (it->second->getTag() == ipp::compositetype::tag)
			{
				tags = static_cast<const ipp::compositetype*>(it->second)->getTags();
			}
			else
			{
				tags.push_back(it->second->getTag());
			}
		}
		return tags;
	}

	void AttributeRegistry::add_boolean(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new booltype(IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_integer(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t Min, const int32_t Max, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new inttype(Min, Max, IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_rangeOfInteger(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t Min, const int32_t Max, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new intrangetype(Min, Max, IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_resolution(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new resolutiontype(IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_datetime(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new datetimetype(IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_string(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t MaxSize, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new stringtype(Tag, MaxSize, IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_keyword(const char* RootName, const char* AttributeName, const int32_t Tag, const int32_t MaxSize, const bool IsArray, const bool IsNoValue, const bool IsUnknown, const char** Keywords, const size_t NumKeywords)
	{
		insert_attribute(RootName, AttributeName, new keywordtype(Keywords, NumKeywords/*, MaxSize*/, IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_enum(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown, const std::pair<int32_t, const char*>* Enums, const size_t NumEnums)
	{
		insert_attribute(RootName, AttributeName, new enumtype(Enums, NumEnums/*, MaxSize*/, IsArray, IsNoValue, IsUnknown));
	}

	void AttributeRegistry::add_collection(const char* RootName, const char* AttributeName, const int32_t Tag, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		insert_attribute(RootName, AttributeName, new collectiontype(IsArray, IsNoValue, IsUnknown));
	}

}
