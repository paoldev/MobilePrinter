#include "pch.h"
#include "CommonUtils.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <set>
#include <iomanip>

struct ExportParams
{
	bool export_as_enum = false;
	bool export_enum_reference_as_standalone = false;
	bool ignore_attribute_and_keywords_bindings = true;
};

typedef std::vector<std::string> LineTokens;

std::vector<LineTokens> Tokenize(const std::string& i_filename, const bool i_first_line_is_header, const size_t i_num_tokens)
{
	std::vector<LineTokens> res;

	size_t lines = 0;
	std::ifstream file;
	file.open(i_filename);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if ((lines > 0) || (!i_first_line_is_header))
			{
				std::vector<std::string> tokens = split(line, ',', true);
				if (tokens.size() != i_num_tokens)
				{
					if ((i_num_tokens <= 1) || (tokens.size() != (i_num_tokens - 1)))
					{
						break;
					}
				}

				for (size_t i = 0; i < tokens.size(); i++)
				{
					trim(tokens.at(i));
				}
				for (size_t i = tokens.size(); i < i_num_tokens; i++)
				{
					tokens.push_back("");
				}

				res.push_back(tokens);
			}

			lines++;
		}
	}

	return res;
}

std::vector<LineTokens> Tokenize(const std::vector<std::string>& i_infilenames, const bool i_first_line_is_header, const size_t i_num_tokens)
{
	std::vector<LineTokens> lines;

	for (size_t i = 0; i < i_infilenames.size(); i++)
	{
		std::vector<LineTokens> tmplines = Tokenize(i_infilenames.at(i), i_first_line_is_header, i_num_tokens);
		lines.insert(lines.end(), tmplines.cbegin(), tmplines.cend());
	}

	return lines;
}

bool is_ipp_1_1_reference(const std::string& i_ref)
{
	return ((i_ref.compare("[RFC8010]") == 0) || (i_ref.compare("[RFC8011]") == 0) || (i_ref.compare("[RFC2911]") == 0));
}

bool is_ipp_fax_or_scan_reference(const std::string& i_ref)
{
	return ((i_ref.compare("[PWG5100.15]") == 0) || (i_ref.compare("[PWG5100.17]") == 0));
}

bool is_ipp_3d_reference(const std::string& i_ref)
{
	return (i_ref.compare("[PWG5100.21]") == 0);
}

bool is_valid_reference(const std::string& i_ref, const bool i_ipp_1_1_only)
{
	if (is_ipp_fax_or_scan_reference(i_ref))
	{
		return false;
	}

	if (is_ipp_3d_reference(i_ref))
	{
		return false;
	}

	if (i_ipp_1_1_only)
	{
		if (!is_ipp_1_1_reference(i_ref))
		{
			return false;
		}
	}

	return true;
}

struct KeywordType
{
	std::string Name;
	std::string Type;
	std::string ParentAttribute;
	std::vector<std::string> Values;
};

void FixSyntax(std::string& io_syntax)
{
	replace_all(io_syntax, "type1", "");
	replace_all(io_syntax, "type2", "");
	trim(io_syntax);

	if (io_syntax.size() >= 2)
	{
		if ((io_syntax.front() == '(') && (io_syntax.back() == ')'))
		{
			io_syntax = io_syntax.substr(1, io_syntax.size() - 2);
			trim(io_syntax);
		}
	}
}

std::string FixAttributeUnderscore(const std::string& i_attribute)
{
	std::string s(i_attribute);
	replace_all(s, "-", "_");
	replace_all(s, ".", "_");
	return s;
}

std::string FixAttributeCamelCase(const std::string& i_attribute)
{
	std::string s(i_attribute);
	if (s.size())
	{
		s.at(0) = std::toupper(s.at(0));
	}
	size_t start = s.find('-');
	while (start != std::string::npos)
	{
		if ((start + 1) < s.size())
		{
			s.at(start + 1) = std::toupper(s.at(start + 1));
		}

		start = s.find('-', start + 1);
	}
	replace_all(s, "-", "");
	replace_all(s, ".", "_");
	return s;
}

std::string StripAttributeSuffix(const std::string& i_attribute)
{
	const char* suffixes[] = { "_supported", "_default", "_actual", "_supplied", "_accepted", "-supported", "-default", "-actual", "-supplied", "-accepted" };

	std::string s(i_attribute);
	for (size_t i = 0; i < _countof(suffixes); i++)
	{
		if (ends_with(s, suffixes[i]))
		{
			s = s.substr(0, s.size() - strlen(suffixes[i]));
			break;
		}
	}
	return s;
}

std::string GenerateKeywordName(const std::string& i_attribute)
{
	std::string s = StripAttributeSuffix(i_attribute);
	return FixAttributeUnderscore(s);
}

std::string GenerateKeywordNameCamelCase(const std::string& i_attribute)
{
	std::string s = StripAttributeSuffix(i_attribute);
	return FixAttributeCamelCase(s);
}

bool IsArrayType(const std::string& i_syntax, std::string& o_array_element)
{
	o_array_element.clear();

	std::string s(i_syntax);
	trim(s);
	size_t start = s.find("1setOf");
	if (start == 0)
	{
		o_array_element = s.substr(sizeof("1setOf") - 1);
		trim(o_array_element);
		if ((o_array_element.size() >= 2) && (o_array_element.front() == '(') && (o_array_element.back() == ')'))
		{
			o_array_element = o_array_element.substr(1, o_array_element.size() - 2);
			trim(o_array_element);
		}
		return true;
	}
	return false;
}

bool PromoteType(std::string& io_syntax, const std::string& new_type)
{
	if ((io_syntax == "keyword") && (new_type == "keyword | name(MAX)"))
	{
		io_syntax = new_type;
		trim(io_syntax);
		return true;
	}

	std::string elem1, elem2;

	if (IsArrayType(io_syntax, elem1) && IsArrayType(new_type, elem2))
	{
		if (PromoteType(elem1, elem2))
		{
			if (elem1.find('|') != std::string::npos)
			{
				io_syntax = "1setOf (" + elem1 + ")";
			}
			else
			{
				io_syntax = "1setOf " + elem1;
			}
			trim(io_syntax);
			return true;
		}
	}

	return false;
}

bool AreTypesCompatible(const std::string& i_syntax, const std::string& new_type)
{
	if (i_syntax == new_type)
	{
		return true;
	}
	if ((i_syntax == "keyword | name(MAX)") && (new_type == "keyword"))
	{
		return true;
	}
	return false;
}

//Find "<any" or "<all" value dependencies (and other variations)
bool IsAnyOrAllDependency(const std::string& i_name, const std::string& i_value, std::string& o_parent_reference)
{
	o_parent_reference.clear();
	std::string value = trim(i_value);
	if (value.size() >= 2)
	{
		if (value.front() == '\"')
		{
			//get first keyword after "any" or "all", between quotes.

			//Fix invalid "trimming-type" entry (missing end quotes)
			value = replace_all(value, "\"\"trimming-type value", "\"\"trimming-type\"\" value");

			//Fix special media dependencies
			value = replace_all(value, "\"\"media\"\" size name", "\"\"media-size-name\"\"");
			value = replace_all(value, "\"\"media\"\" color", "\"\"media-color\"\"");
			value = replace_all(value, "\"\"media\"\" input tray", "\"\"media-input-tray\"\"");
			value = replace_all(value, "\"\"media\"\" media or size", "\"\"media\"\"");	//Map to "media" instead of 'media-media and media-size'

			size_t start_quotes = value.find("\"\"");
			size_t end_quotes = std::string::npos;
			if (start_quotes != std::string::npos)
			{
				end_quotes = value.find("\"\"", start_quotes + 2);
			}
			if ((start_quotes != std::string::npos) && (end_quotes != std::string::npos))
			{
				std::string attribute = value.substr(start_quotes + 2, end_quotes - start_quotes - 2);

				//Fix invalid "job-default-output-until" attribute name
				if (attribute == "job-default-output-until")
				{
					attribute = "job-delay-output-until";
				}

				o_parent_reference = attribute;
				return true;
			}
			else
			{
				printf("Error parsing dependency: %s -> %s\n", i_name.c_str(), i_value.c_str());
			}
		}
		else if (value.front() == '<')
		{
			static std::vector<std::pair<std::string, std::string>> s_embedded = {
				{ "accuracy-units values", "accuracy-units" },
				{ "Printer attribute keyword name", "Printer object"},
				{ "Document object attribute", "Document object"},
				{ "Job Template attribute keyword name", "Job Template"},
				{ "Job object attribute", "Job object"},
				{ "materials-col member attribute name", "materials-col"},
				{ "multiple-object-handling ", "multiple-object-handling"},
				{ "print-base ", "print-base"},
				{ "print-objects ", "print-objects"},
				{ "print-supports ", "print-supports"},
				{ "Subscription object attribute", "Subscription object"}
			};

			for (size_t i = 0; i < s_embedded.size(); i++)
			{
				if (strstr(value.c_str(), s_embedded[i].first.c_str()))
				{
					o_parent_reference = s_embedded[i].second;
					return true;
				}
			}

			printf("Error parsing dependency: %s -> %s\n", i_name.c_str(), i_value.c_str());
		}
	}

	return false;
}

bool ParseKeywordsFiles(const std::vector<std::string>& i_infilenames, const bool i_ipp_1_1_only, std::vector<KeywordType>& keywords)
{
	enum Tokens
	{
		Name = 0,
		Value,
		Syntax,
		Type,
		Reference,
		NumTokens
	};

	std::vector<LineTokens> lines = Tokenize(i_infilenames, true, NumTokens);
	if (lines.size())
	{
		std::vector<KeywordType> mediaTypesKeywords;	//special list for media subtypes
		std::multimap<std::string, std::string> obsolete_keywords;

		std::string tmpName;
		for (auto it = lines.begin(); it != lines.end(); it++)
		{
			FixSyntax(it->at(Syntax));

			const std::string& name = it->at(Name);
			const std::string& value = it->at(Value);
			const std::string& syntax = it->at(Syntax);
			const std::string& type = it->at(Type);
			const std::string& ref = it->at(Reference);

			//if (ref.empty())
			//{
			//	printf("Missing reference: %s -> %s\n", name.c_str(), value.c_str());
			//	continue;
			//}

			if (!is_valid_reference(ref, i_ipp_1_1_only))
			{
				printf("Skipping reference: %s -> %s - ref %s\n", name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			if (keywords.empty() || (keywords.back().Name != name))
			{
				KeywordType newType;
				newType.Name = name;
				newType.Type = syntax;
				keywords.push_back(newType);
			}

			if (!type.empty())
			{
				if (name.compare("media") == 0)
				{
					std::string mediaTypeName = "media-" + type;
					replace_all(mediaTypeName, " ", "-");
					const std::string& mediaTypeValue = value;
					auto itMedia = std::find_if(mediaTypesKeywords.begin(), mediaTypesKeywords.end(), [keyName = mediaTypeName](auto& elem)->bool { return elem.Name == keyName; });
					if (itMedia != mediaTypesKeywords.end())
					{
						itMedia->Values.push_back(value);
					}
					else
					{
						KeywordType newType;
						newType.Name = mediaTypeName;
						newType.Type = syntax;
						newType.Values.push_back(value);
						mediaTypesKeywords.push_back(newType);
					}
				}
				else
				{
					printf("Unexpected keyword type: %s -> %s -> %s\n", name.c_str(), value.c_str(), type.c_str());
				}
			}

			if (!value.empty())
			{
				std::string parent_dependency;
				if (IsAnyOrAllDependency(keywords.back().Name, value, parent_dependency))
				{
					if (keywords.back().ParentAttribute.empty())
					{
						keywords.back().ParentAttribute = parent_dependency;
						//keywords.back().Values.clear();
					}
					else if (keywords.back().ParentAttribute != parent_dependency)
					{
						printf("Unexpected parent dependency: %s -> previous %s -> now %s\n", name.c_str(), keywords.back().ParentAttribute.c_str(), parent_dependency.c_str());
					}
				}
				else
				{
					//Can this enum value be ignored (not used, obsolete, deprecated, reserved, etc)?
					if (value.find('(') != std::string::npos)
					{
						//If this value is obsolete or deprecated, remove it later from the list.
						if ((value.find("(obsolete") != std::string::npos) || (value.find("(deprecated") != std::string::npos))
						{
							auto pos = value.find('(');
							auto to_be_removed = std::pair<std::string, std::string>(keywords.back().Name, trim(value.substr(0, pos)));
							obsolete_keywords.insert(to_be_removed);
						}

						printf("Skipping invalid keyword: %s -> %s\n", name.c_str(), value.c_str());
						continue;
					}
					else
					{
						keywords.back().Values.push_back(value);
					}
				}

				//If possible, promote keyword syntax to a more generic one, according to value syntax.
				if (syntax != keywords.back().Type)
				{
					std::string old_type = keywords.back().Type;
					if (PromoteType(keywords.back().Type, syntax))
					{
						printf("Type promoted: %s -> %s: old '%s' - new '%s'\n",
							keywords.back().Name.c_str(), value.c_str(), old_type.c_str(), keywords.back().Type.c_str());
					}
					else if (!AreTypesCompatible(keywords.back().Type, syntax))
					{
						printf("Type mismatch: %s -> %s: found '%s' - expected '%s'\n",
							keywords.back().Name.c_str(), value.c_str(), syntax.c_str(), keywords.back().Type.c_str());
					}
				}
			}
		}

		//Remove obsolete keywords
		for (auto it = obsolete_keywords.cbegin(); it != obsolete_keywords.cend(); it++)
		{
			auto it_enum = std::find_if(keywords.begin(), keywords.end(), [name = it->first](auto& elem)->bool { return elem.Name == name; });
			if (it_enum != keywords.end())
			{
				for (auto it_value = it_enum->Values.begin(); it_value != it_enum->Values.end(); )
				{
					if ((*it_value) == it->second)
					{
						it_value = it_enum->Values.erase(it_value);
					}
					else
					{
						it_value++;
					}
				}
			}
		}

		//Add media-media, media-size, etc.
		keywords.insert(keywords.end(), mediaTypesKeywords.cbegin(), mediaTypesKeywords.cend());
	}

	return !keywords.empty();
}

struct AttributeType
{
	std::string Name;
	std::string Type;
	std::vector<AttributeType> Members;	//Only if Type is collection.
};

struct CollectionType
{
	std::string Name;
	std::vector<AttributeType> Members;
};

void InsertTab(std::ofstream& file_cpp, int32_t i_level)
{
	while (i_level > 0)
	{
		file_cpp << "\t";
		i_level--;
	}
}

void FixCollectionReferences(const std::vector<CollectionType>& Collections, AttributeType& Attribute)
{
	auto it = &Attribute;

	const char Prefix1[] = "Member attributes are the same as the \"\"";
	const char Prefix2[] = "Any ";
	for (size_t m = 0; m < it->Members.size(); m++)
	{
		AttributeType& attr = it->Members[m];

		if (attr.Type.empty() && attr.Members.empty())
		{
			size_t prefix1 = attr.Name.find(Prefix1);
			size_t prefix2 = attr.Name.find(Prefix2);
			if (prefix1 != std::string::npos)
			{
				std::string referencename;
				std::string collectionname;
				size_t endprefix1 = prefix1 + sizeof(Prefix1) - 1;
				size_t case1 = attr.Name.find("\"\">", endprefix1);
				size_t case2 = attr.Name.find("\"\"", endprefix1);
				if (case1 != std::string::npos)
				{
					referencename = attr.Name.substr(endprefix1, case1 - endprefix1);
				}
				else if (case2 != std::string::npos)
				{
					referencename = attr.Name.substr(endprefix1, case2 - endprefix1);
					size_t case3 = attr.Name.find(" attribute", case2);
					if (case3 != std::string::npos)
					{
						collectionname = attr.Name.substr(case2 + 2, case3 - case2 - 2);
					}
				}
				trim(referencename);
				trim(collectionname);
				if (referencename.size())
				{
					if (collectionname.empty())
					{
						collectionname = "Job Template";	//TODO: check if this is valid for all attributes.
					}

					if ((it->Name == "job-finishings-col-actual") && (referencename=="media-col"))
					{
						referencename = "job-finishings-col";
					}

					for (int loop = 0; loop < 2; loop++)
					{
						auto itcoll = Collections.cbegin();
						for (; itcoll != Collections.cend(); itcoll++)
						{
							if (itcoll->Name.compare(collectionname) == 0)
							{
								break;
							}
						}

						if (itcoll != Collections.cend())
						{
							auto itref = itcoll->Members.cbegin();
							for (; itref != itcoll->Members.cend(); itref++)
							{
								if (itref->Name.compare(referencename) == 0)
								{
									break;
								}
							}
							if (itref != itcoll->Members.cend())
							{
								it->Members.erase(it->Members.begin() + m, it->Members.begin() + m + 1);
								it->Members.insert(it->Members.begin() + m, itref->Members.begin(), itref->Members.end());
								//m += itref->Members.size();
								m--;//restart from 'm' position. JobStatus "overrides" references another reference.
								break;
							}
							else
							{
								if ((loop == 0) && (collectionname == "Document Template"))
								{
									collectionname = "Document Description";
									continue;
								}
								else if ((loop == 0) && (collectionname == "Job Template"))
								{
									collectionname = "Job Status";
									continue;
								}

								printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
								break;
							}
						}
						else
						{
							printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
							break;
						}
					}
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (prefix2 != std::string::npos)
			{
				size_t endprefix2 = prefix2 + sizeof(Prefix2) - 1;
				size_t case3 = attr.Name.find(" attribute", endprefix2);
				if (case3 != std::string::npos)
				{
					std::string collectionname = attr.Name.substr(endprefix2, case3 - endprefix2);
					trim(collectionname);
					if (collectionname.size())
					{
						auto itcoll = Collections.cbegin();
						for (; itcoll != Collections.cend(); itcoll++)
						{
							if (itcoll->Name.compare(collectionname) == 0)
							{
								break;
							}
						}

						//TODO: this causes a recursion, due to Job Template 'overrides'
						if (itcoll != Collections.cend())
						{
							it->Members.erase(it->Members.begin() + m, it->Members.begin() + m + 1);
							//it->Members.insert(it->Members.begin() + m, itcoll->Members.begin(), itcoll->Members.end());
							size_t cont = m;
							for (auto k = itcoll->Members.cbegin(); k != itcoll->Members.end(); k++)
							{
								if (k->Name != it->Name)
								{
									it->Members.insert(it->Members.begin() + cont, *k);
									cont++;
								}
							}
							//m += itcoll->Members.size();
							m--;//restart from beginning. JobStatus "overrides" references another reference.
						}
						else

						{
							printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
						}
					}
					else
					{
						printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
					}
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else
			{
			}
		}
	}
}

void FixCollectionReferences(std::vector<CollectionType>& Collections, std::vector<AttributeType>& Members)
{
	for (auto it = Members.begin(); it != Members.end(); it++)
	{
		if (it->Name.empty())
		{
			//Deprecated or Extension attribute. Ignore it.
			continue;
		}

		bool bCollection = (it->Type.find("collection") != std::string::npos);
		if (bCollection)
		{
			FixCollectionReferences(Collections, *it);

			FixCollectionReferences(Collections, it->Members);
		}
	}
}

void FixCollectionReferences(std::vector<CollectionType>& Collections)
{
	for (auto itC = Collections.begin(); itC != Collections.end(); itC++)
	{
		CollectionType& mainCollection = *itC;

		FixCollectionReferences(Collections, mainCollection.Members);
	}
}

class AttributeExporter
{
public:

	static bool parse_rangeOfInteger(const std::string& InType, std::string& OutIntMin, std::string& OutIntMax)
	{
		return parse_integer_range(InType, OutIntMin, OutIntMax);
	}

	static bool parse_integer(const std::string& InType, std::string& OutIntMin, std::string& OutIntMax)
	{
		return parse_integer_range(InType, OutIntMin, OutIntMax);
	}

	static bool parse_text(const std::string& InType, std::string& OutIntMax)
	{
		return parse_string(InType, "TEXT_MAX", OutIntMax);
	}

	static bool parse_name(const std::string& InType, std::string& OutIntMax)
	{
		return parse_string(InType, "NAME_MAX", OutIntMax);
	}

	static bool parse_octetString(const std::string& InType, std::string& OutIntMax)
	{
		return parse_string(InType, "OCTET_STRING_MAX", OutIntMax);
	}

	static bool parse_uri(const std::string& InType, std::string& OutIntMax)
	{
		return parse_string(InType, "URI_MAX", OutIntMax);
	}

	static std::string get_rangeOfInteger(const std::string& RootName, const std::string& AttributeName, const std::string& IntMin, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_RANGE(\"{RootName}\", \"{AttributeName}\", tags::rangeOfInteger, {IntMin}, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, IntMin, IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_integer(const std::string& RootName, const std::string& AttributeName, const std::string& IntMin, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_INTEGER(\"{RootName}\", \"{AttributeName}\", tags::integer, {IntMin}, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, IntMin, IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_boolean(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_BOOLEAN(\"{RootName}\", \"{AttributeName}\", tags::boolean, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_resolution(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_RESOLUTION(\"{RootName}\", \"{AttributeName}\", tags::resolution, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_dateTime(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_DATETIME(\"{RootName}\", \"{AttributeName}\", tags::dateTime, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_uri(const std::string& RootName, const std::string& AttributeName, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::uri, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_uriScheme(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::uriScheme, URI_SCHEME_MAX, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_charset(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::charset, CHARSET_MAX, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_naturalLanguage(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::naturalLanguage, NATURAL_LANGUAGE_MAX, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_mimeMediaType(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::mimeMediaType, MIME_MEDIA_TYPES_MAX, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_text(const std::string& RootName, const std::string& AttributeName, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::textWithoutLanguage, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_enum(const std::string& RootName, const std::string& AttributeName, const std::string& EnumName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_ENUM(\"{RootName}\", \"{AttributeName}\", tags::enum_tag, {IsArray}, {IsNoValue}, {IsUnknown}, ipp::{KeywordOrEnum}::description);";
		return get_attribute(Format, RootName, AttributeName, "", "", EnumName, IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_keyword(const std::string& RootName, const std::string& AttributeName, const std::string& IntMax, const std::string& KeywordName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_KEYWORD(\"{RootName}\", \"{AttributeName}\", tags::keyword, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown}, ipp::s_{KeywordOrEnum}_keywords);";
		return get_attribute(Format, RootName, AttributeName, "", IntMax, KeywordName, IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_name(const std::string& RootName, const std::string& AttributeName, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::nameWithoutLanguage, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_octetString(const std::string& RootName, const std::string& AttributeName, const std::string& IntMax, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "ADD_STRING(\"{RootName}\", \"{AttributeName}\", tags::octetString, {IntMax}, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", IntMax, "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_beginCollection(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		const char* Format = "BEGIN_COLLECTION(\"{RootName}\", \"{AttributeName}\", tags::begCollection, {IsArray}, {IsNoValue}, {IsUnknown});";
		return get_attribute(Format, RootName, AttributeName, "", "", "", IsArray, IsNoValue, IsUnknown);
	}

	static std::string get_endCollection(const std::string& RootName, const std::string& AttributeName, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		return "END_COLLECTION();";
	}

private:

	static std::string to_string(const bool value)
	{
		return value ? "true" : "false";
	}

	static std::string get_attribute(const char* Format, const std::string& InRootName, const std::string& InAttributeName, const std::string& IntMin, const std::string& IntMax, const std::string& KeywordOrEnum, const bool IsArray, const bool IsNoValue, const bool IsUnknown)
	{
		std::vector<std::string> path = split(InRootName + "." + InAttributeName, '.', true);
		std::string RootName, AttributeName;
		if (path.size())
		{
			RootName = path.at(0);
			path.erase(path.begin());

			AttributeName = join(path, '.');
		}

		std::string res = Format;
		replace_all(res, "{RootName}", RootName);
		replace_all(res, "{AttributeName}", AttributeName);
		replace_all(res, "{IntMin}", IntMin);
		replace_all(res, "{IntMax}", IntMax);
		replace_all(res, "{KeywordOrEnum}", KeywordOrEnum);
		replace_all(res, "{IsArray}", to_string(IsArray));
		replace_all(res, "{IsNoValue}", to_string(IsNoValue));
		replace_all(res, "{IsUnknown}", to_string(IsUnknown));
		return res;
	}

	static bool parse_integer_range(const std::string& InType, std::string& OutIntMin, std::string& OutIntMax)
	{
		std::string Type = InType;
		replace_all(Type, " ", "");
		size_t openP = Type.find("(");
		size_t closeP = Type.find(")", openP + 1);
		if ((openP != std::string::npos) && (closeP != std::string::npos))
		{
			size_t colon = Type.find(":", openP + 1);
			if (colon != std::string::npos)
			{
				std::string int_min = Type.substr(openP + 1, colon - openP - 1);
				std::string int_max = Type.substr(colon + 1, closeP - colon - 1);
				if (int_min.find("MIN") != std::string::npos)
				{
					int_min = "INT_MIN";
				}
				if (int_max.find("MAX") != std::string::npos)
				{
					int_max = "INT_MAX";
				}

				OutIntMin = int_min;
				OutIntMax = int_max;
				return true;
			}
			else
			{
				std::string int_max = Type.substr(openP + 1, closeP - openP - 1);
				if (int_max.size())
				{
					//std::string int_min = "1";//TODO: check if this is valid for all attributes.
					std::string int_min = "INT_MIN";//TODO: check if this is valid for all attributes.
					if (int_max.find("MAX") != std::string::npos)
					{
						int_max = "INT_MAX";
					}

					OutIntMin = int_min;
					OutIntMax = int_max;
					return true;
				}
			}
		}
		else if ((openP == std::string::npos) && (closeP == std::string::npos))
		{
			OutIntMin = "INT_MIN";
			OutIntMax = "INT_MAX";
			return true;
		}

		return false;
	}

	static bool parse_string(const std::string& InType, const std::string& MaxDefault, std::string& OutIntMax)
	{
		std::string Type = InType;
		replace_all(Type, " ", "");
		size_t openP = Type.find("(");
		size_t closeP = Type.find(")", openP + 1);

		//Some ill-formed attributes don't have a closing parenthesis
		if ((openP != std::string::npos) && (closeP == std::string::npos))
		{
			closeP = Type.size();
		}

		if ((openP != std::string::npos) && (closeP != std::string::npos))
		{
			std::string max_len = Type.substr(openP + 1, closeP - openP - 1);

			if (max_len.find("MAX") != std::string::npos)
			{
				max_len = MaxDefault;
			}

			OutIntMax = max_len;
			return true;
		}
		else if ((openP == std::string::npos) && (closeP == std::string::npos))
		{
			OutIntMax = MaxDefault;
			return true;
		}

		return false;
	}
};

void FixAttributesNames(std::vector<AttributeType>& Members)
{
	{
		//Fix conflicting enum-keywords definitions

		for (auto it = Members.begin(); it != Members.end(); it++)
		{
			if (it->Name == "job-collation-type-actual")
			{
				//declared as 1setOf keyword
				it->Type = "1setOf enum";
			}
			else if ((it->Name == "media-key") || (it->Name == "preset-name"))
			{
				//Remove 'keyword', to simplify attribute declaration.
				it->Type = "name(MAX)";
			}
			else if (it->Name == "media-key-supported")
			{
				//Remove 'keyword', to simplify attribute declaration.
				it->Type = "1setOf name(MAX)";
			}
			else if (it->Name == "input-orientation-requested")
			{
				//Fix invalid 'keyword' type.
				it->Type = "type2 enum";
			}
		}

		//Fix deprecated and extension attributes
		for (auto it = Members.begin(); it != Members.end(); it++)
		{
			bool bExtension = (it->Name.find("(extension)") != std::string::npos);
			if (bExtension)
			{
				std::string new_name = it->Name;
				replace_all(new_name, "(extension)", "");
				trim(new_name);

				auto it2 = Members.begin();
				for (; it2 != Members.end(); it2++)
				{
					if (it2->Name == new_name)
					{
						break;
					}
				}

				if (it2 != Members.end())
				{
					//replace the new type over the old element
					bool bOldNoValue = (it2->Type.find("no-value") != std::string::npos);
					it2->Type = it->Type;
					if (bOldNoValue && (it2->Type.find("no-value") == std::string::npos))
					{
						it2->Type = "no-value | " + it2->Type;
					}
					//Reset the attribute
					*it = AttributeType();
				}
				else
				{
					it->Name = new_name;
				}
			}

			bool bDeprecated = (it->Name.find("(deprecated)") != std::string::npos);
			if (bDeprecated)
			{
				std::string new_name = it->Name;
				replace_all(new_name, "(deprecated)", "");
				trim(new_name);

				auto it2 = Members.begin();
				for (; it2 != Members.end(); it2++)
				{
					if (it2->Name == new_name)
					{
						break;
					}
				}

				if (it2 != Members.end())
				{
					//New attribute found: reset the deprecated one and ignore it.
					*it = AttributeType();
				}
				else
				{
					it->Name = new_name;
				}
			}

			bool bObsolete = (it->Name.find("(obsolete)") != std::string::npos);
			if (bObsolete)
			{
				std::string new_name = it->Name;
				replace_all(new_name, "(obsolete)", "");
				trim(new_name);

				auto it2 = Members.begin();
				for (; it2 != Members.end(); it2++)
				{
					if (it2->Name == new_name)
					{
						break;
					}
				}

				if (it2 != Members.end())
				{
					//New attribute found: reset the deprecated one and ignore it.
					*it = AttributeType();
				}
				else
				{
					it->Name = new_name;
				}
			}

			//See [PWG5100.6]
			if (it->Type.find("rangeOfInteger(MAX)") != std::string::npos)
			{
				if ((it->Name == "document-copies") || (it->Name == "document-numbers") || (it->Name == "pages"))
				{
					replace_all(it->Type, "rangeOfInteger(MAX)", "rangeOfInteger(1:MAX)");
				}
			}

			if (it->Members.size())
			{
				FixAttributesNames(it->Members);
			}
		}
	}
}

void FixAttributesNames(std::vector<CollectionType>& Collections)
{
	for (auto itC = Collections.begin(); itC != Collections.end(); itC++)
	{
		auto& Members = itC->Members;

		FixAttributesNames(Members);
	}
}

void GetAttributeMemberList(const std::vector<AttributeType>& Attributes, const std::string& ParentName, bool bRecurseCollections, std::vector<std::string>& o_attributes)
{
	for (auto itm = Attributes.begin(); itm != Attributes.end(); itm++)
	{
		if (itm->Name.empty())
		{
			//Deprecated or Extension attribute. Ignore it.
			continue;
		}

		std::string thisName = ParentName.empty() ? itm->Name : (ParentName + "." + itm->Name);

		if (std::find(o_attributes.cbegin(), o_attributes.cend(), thisName) == o_attributes.cend())
		{
			o_attributes.push_back(thisName);
		}
		if (bRecurseCollections && itm->Members.size())
		{
			GetAttributeMemberList(itm->Members, thisName, bRecurseCollections, o_attributes);
		}
	}
}

void GetAttributeList(const std::vector<CollectionType>& Collections, const std::string& RootCollectionName, bool bRecurseCollections, std::vector<std::string>& o_attributes)
{
	for (auto it = Collections.begin(); it != Collections.end(); it++)
	{
		if (it->Name == RootCollectionName)
		{
			GetAttributeMemberList(it->Members, "", bRecurseCollections, o_attributes);
		}
	}
}

void AddAttributeKeywords(const std::vector<CollectionType>& Collections, std::vector<KeywordType>& keywords,
	const std::string& i_new_keyword, const std::string& i_type, const std::vector<std::string>& i_initial_values, 
	const std::vector<std::string>& i_collections, bool i_recurse)
{
	auto itk = std::find_if(keywords.begin(), keywords.end(), [attr = i_new_keyword](auto& elem)->bool { return elem.Name == attr; });
	if (itk == keywords.end())
	{
		KeywordType key;
		key.Name = i_new_keyword;
		key.Type = i_type;
		itk = keywords.insert(keywords.end(), key);
	}
	itk->ParentAttribute.clear();
	itk->Values = i_initial_values;
	for (size_t i = 0; i < i_collections.size(); i++)
	{
		GetAttributeList(Collections, i_collections.at(i), i_recurse, itk->Values);
	}
}

void GenerateAttributesKeywords(std::vector<CollectionType>& Collections, std::vector<KeywordType>& keywords, const bool i_ipp_1_1_only)
{
	//Generate keywords from collection members
	//if (i_level == 0)
	for (auto itC = Collections.begin(); itC != Collections.end(); itC++)
	{
		auto& Members = itC->Members;

		for (auto it = Members.begin(); it != Members.end(); it++)
		{
			if (it->Name.empty())
			{
				//Deprecated or Extension attribute. Ignore it.
				continue;
			}

			bool bCollection = (it->Type.find("collection") != std::string::npos);
			if (bCollection && (it->Name.find("\"") == std::string::npos))
			{
				KeywordType key;
				key.Name = it->Name;
				bool bIgnore = false;
				for (auto it2 = it->Members.cbegin(); (!bIgnore) && (it2 != it->Members.cend()); it2++)
				{
					if ((it2->Name.find("\"") != std::string::npos))
					{
						bIgnore = true;
						break;
					}
					if (it2->Name.size())
					{
						key.Values.push_back(it2->Name);
					}
				}
				if (!bIgnore)
				{
					std::string expected_keyword_name = GenerateKeywordName(it->Name);
					auto itk = std::find_if(keywords.begin(), keywords.end(), [attr = expected_keyword_name](auto& elem)->bool { return GenerateKeywordName(elem.Name) == attr; });
					if (itk == keywords.end())
					{
						keywords.push_back(key);
					}
					else
					{
						for (auto itv = key.Values.begin(); itv != key.Values.end(); itv++)
						{
							if (std::find(itk->Values.begin(), itk->Values.end(), *itv) == itk->Values.end())
							{
								itk->Values.push_back(*itv);
							}
						}
					}
				}
			}
		}
	}

	//Fix attribute-dependent keywords
	if (!i_ipp_1_1_only)
	{
		std::vector<std::string> empty_list;
		{
			//job-mandatory-attributes: all Job Template attributes
			AddAttributeKeywords(Collections, keywords, "job-mandatory-attributes", "1setOf keyword", empty_list, { "Job Template" }, true);
		}
		{
			//PWG 5100.5
			//document-creation-attributes-supported: all Document Template and operation attributes
			AddAttributeKeywords(Collections, keywords, "document-creation-attributes-supported", "1setOf keyword", empty_list, { "Document Template", "Operation" }, true);
		}
		{
			//IPPPRIVACY10
			//document-privacy-attributes: all Document attributes
			AddAttributeKeywords(Collections, keywords, "document-privacy-attributes", "1setOf keyword",
				{ "all", "default", "document-description", "document-template", "none" }, { "Document Description", "Document Template" }, true);
		}
		{
			//IPPPRIVACY10
			//job-privacy-attributes: all Document attributes
			AddAttributeKeywords(Collections, keywords, "job-privacy-attributes", "1setOf keyword",
				{ "all", "default", "job-description", "job-template", "none" }, { "Job Description", "Job Template" }, true);
		}
		{
			//PWG5100.18
			//fetch-document-attributes-supported: all Printer attributes
			AddAttributeKeywords(Collections, keywords, "fetch-document-attributes-supported", "1setOf keyword",
				empty_list, { "Printer Description" }, true);
		}
		{
			//PWG5100.11
			//job-creation-attributes-supported: all Job Template attributes and Job Operation attributes
			std::vector<std::string> job_operations;
			GetAttributeList(Collections, "Operation", false, job_operations);
			job_operations.erase(std::remove_if(job_operations.begin(), job_operations.end(), [](auto& elem)->bool { return !begins_with(elem, "job-"); }), job_operations.end());

			AddAttributeKeywords(Collections, keywords, "job-creation-attributes-supported", "1setOf keyword",
				job_operations, { "Job Template" }, false);

			//PWG5100.13
			AddAttributeKeywords(Collections, keywords, "printer-mandatory-job-attributes", "1setOf keyword",
				job_operations, { "Job Template" }, false);

			//PWG5100.13
			if (std::find(job_operations.cbegin(), job_operations.cend(), "document-format") == job_operations.cend())
			{
				job_operations.push_back("document-format");
			}
			AddAttributeKeywords(Collections, keywords, "printer-get-attributes-supported", "1setOf keyword",
				job_operations, { "Job Template" }, false);
		}
		{
			//PWG5100.7
			//job-history-attributes-configured: all Job Status attributes
			AddAttributeKeywords(Collections, keywords, "job-history-attributes-configured", "1setOf keyword",
				empty_list, { "Job Status" }, false);
		}
		{
			//PWG5100.7
			//job-history-attributes-supported: all Job Status attributes
			AddAttributeKeywords(Collections, keywords, "job-history-attributes-supported", "1setOf keyword",
				empty_list, { "Job Status" }, false);
		}
		{
			//RFC3995
			//notify-attributes: all Job Status attributes
			AddAttributeKeywords(Collections, keywords, "notify-attributes", "1setOf keyword",
				empty_list, { "Printer Status", "Job Status", "Subscription Status" }, false);
		}
		{
			//IPPWG20151019
			//"pdl-override-guaranteed-supported": all Job Template attributes
			AddAttributeKeywords(Collections, keywords, "pdl-override-guaranteed-supported", "1setOf keyword",
				empty_list, { "Job Template" }, false);
		}

		//PWG5100.22
		AddAttributeKeywords(Collections, keywords, "system-settable-attributes-supported", "1setOf keyword",
			{ "none" }, empty_list, false);

		//PWG5100.22
		AddAttributeKeywords(Collections, keywords, "system-mandatory-printer-attributes", "1setOf keyword",
			{ "printer-name" }, { "Printer Description" }, false);

		//PWG5100.22
		AddAttributeKeywords(Collections, keywords, "printer-creation-attributes-supported", "1setOf keyword",
			{ "document-format-default", "document-format-supported", "multiple-document-jobs-supported", "natural-language-configured", "printer-geo-location", "printer-info", "printer-location", "printer-make-and-model", "printer-name" },
			{ "Printer Description" }, false);

		//RFC3380
		{
			std::vector<std::string> job_template;
			GetAttributeList(Collections, "Job Template", false, job_template);
			job_template.erase(std::remove_if(job_template.begin(), job_template.end(), [](auto& elem)->bool { return !ends_with(elem, "-default") && !ends_with(elem, "-supported") && !ends_with(elem, "-ready"); }), job_template.end());
			if (std::find(job_template.cbegin(), job_template.cend(), "none") == job_template.cend())
			{
				job_template.insert(job_template.begin(), "none");
			}
			AddAttributeKeywords(Collections, keywords, "printer-settable-attributes-supported", "1setOf keyword",
				job_template, {}, false);
		}

		//RFC3380
		{
			std::vector<std::string> job_template;
			GetAttributeList(Collections, "Job Template", false, job_template);
			job_template.erase(std::remove_if(job_template.begin(), job_template.end(), [](auto& elem)->bool { return !begins_with(elem, "job-"); }), job_template.end());
			if (std::find(job_template.cbegin(), job_template.cend(), "none") == job_template.cend())
			{
				job_template.insert(job_template.begin(), "none");
			}
			AddAttributeKeywords(Collections, keywords, "job-settable-attributes-supported", "1setOf keyword",
				job_template, {}, false);
		}

		//RFC3380
		AddAttributeKeywords(Collections, keywords, "document-format-varying-attributes", "1setOf keyword", { "none" }, { "Printer Description"}, false);

		//[IPPPRIVACY10]
		AddAttributeKeywords(Collections, keywords, "subscription-privacy-attributes", "1setOf keyword",
			{ "all", "default", "none", "subscription-description", "subscription-template" }, {"Subscription Status", "Subscription Template"}, false);

		//[PWG5100.3]
		AddAttributeKeywords(Collections, keywords, "user-defined-values-supported", "1setOf keyword", empty_list, { "Job Template"}, true);
		
		//PWG5100.22
		AddAttributeKeywords(Collections, keywords, "resource-settable-attributes-supported", "1setOf keyword", empty_list, { "Resource Description" }, true);
	}
}

void ExportAttributeList(const std::vector<CollectionType>& Collections, std::vector<AttributeType>& Members, const std::string& RootName, const std::string& ParentName, 
	const int32_t i_level, const bool i_ipp_1_1_only, const ExportParams& params, std::ofstream& file_h, std::ofstream& file_cpp)
{
	if (Members.empty())
	{
		printf("Ignoring empty collection '%s' (root '%s')\n", ParentName.c_str(), RootName.c_str());
		return;
	}
	
	//Export attributes
	for (auto it = Members.begin(); it != Members.end(); it++)
	{
		if (it->Name.empty())
		{
			//Deprecated or Extension attribute. Ignore it.
			continue;
		}

		bool bNoValue = (it->Type.find("no-value") != std::string::npos);
		if (bNoValue)
		{
			replace_all(it->Type, "no-value", "");
			trim(it->Type);
			if (it->Type.size() > 0)
			{
				if (it->Type.front() == '|')
				{
					it->Type.erase(it->Type.begin(), it->Type.begin() + 1);
				}
				if (it->Type.back() == '|')
				{
					it->Type.pop_back();
				}
				trim(it->Type);
			}
		}

		bool bUnknown = (it->Type.find("unknown") != std::string::npos);
		if (bUnknown)
		{
			replace_all(it->Type, "unknown", "");
			trim(it->Type);
			if (it->Type.size() > 0)
			{
				if (it->Type.front() == '|')
				{
					it->Type.erase(it->Type.begin(), it->Type.begin() + 1);
				}
				if (it->Type.back() == '|')
				{
					it->Type.pop_back();
				}
				trim(it->Type);
			}
		}

		std::string compactType = it->Type;
		replace_all(compactType, " ", "");

		if ((compactType.find("keyword|name") != std::string::npos))
		{
			replace_all(compactType, "keyword|name", "keyword_or_name");
			it->Type = compactType;
		}

		{
			size_t intpos = compactType.find("integer");
			if (intpos != std::string::npos)
			{
				size_t pipepos = compactType.find("|", intpos + 1);
				if (pipepos != std::string::npos)
				{
					size_t rangepos = compactType.find("rangeOfInteger", pipepos + 1);
					if (rangepos != std::string::npos)
					{
						//'integer(min, max) | rangeOfInteger(min, max)' have the same min, max values.
						//Convert it to rangeOfInteger.
						compactType.erase(compactType.begin() + intpos, compactType.begin() + rangepos);
						replace_all(compactType, "rangeOfInteger", "integer_or_rangeOfInteger");
						it->Type = compactType;
					}
				}
			}
		}

		if (it->Type.find("|") != std::string::npos)
		{
			printf("TODO composed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
		}
		else
		{
			bool bIsArray = (it->Type.find("1setOf") != std::string::npos);
			if (bIsArray)
			{
				replace_all(it->Type, "1setOf", "");
				trim(it->Type);
			}

			//Remove parenthesis around types
			if (it->Type.size() >= 2)
			{
				if ((it->Type.front() == '(') && (it->Type.back() == ')'))
				{
					it->Type.erase(it->Type.begin(), it->Type.begin() + 1);
					it->Type.pop_back();
				}
				trim(it->Type);
			}

			if (it->Type.find("integer_or_rangeOfInteger") != std::string::npos)
			{
				//TODO: add integer_or_rangeOfInteger type
				std::string int_min;
				std::string int_max;
				if (AttributeExporter::parse_rangeOfInteger(it->Type, int_min, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_rangeOfInteger(ParentName, it->Name, int_min, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("rangeOfInteger") != std::string::npos)
			{
				std::string int_min;
				std::string int_max;
				if (AttributeExporter::parse_rangeOfInteger(it->Type, int_min, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_rangeOfInteger(ParentName, it->Name, int_min, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("integer") != std::string::npos)
			{
				std::string int_min;
				std::string int_max;
				if (AttributeExporter::parse_integer(it->Type, int_min, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_integer(ParentName, it->Name, int_min, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("boolean") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_boolean(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("resolution") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_resolution(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("dateTime") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_dateTime(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("uriScheme") != std::string::npos)	//uriScheme before uri
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_uriScheme(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("uri") != std::string::npos)
			{
				std::string int_max;
				if (AttributeExporter::parse_uri(it->Type, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_uri(ParentName, it->Name, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("charset") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_charset(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("naturalLanguage") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_naturalLanguage(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("mimeMediaType") != std::string::npos)
			{
				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_mimeMediaType(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("text") != std::string::npos)
			{
				std::string int_max;
				if (AttributeExporter::parse_text(it->Type, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_text(ParentName, it->Name, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if ((it->Type.find("enum") != std::string::npos) || (it->Type.compare("num") == 0))
			{
				//print-quality type is called "num" instead of "enum"
				std::string enum_name = FixAttributeUnderscore(it->Name);

				enum_name = StripAttributeSuffix(enum_name);

				if (it->Name == "input-orientation-requested")
				{
					enum_name = "orientation_requested";
				}
				else if (it->Name == "resource-states")
				{
					enum_name = "resource_state";
				}
				else if (it->Name == "finishings-ready")
				{
					enum_name = "finishings";
				}

				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_enum(ParentName, it->Name, enum_name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("keyword_or_name") != std::string::npos)
			{
				std::string keyword_name = FixAttributeUnderscore(it->Name);

				keyword_name = StripAttributeSuffix(keyword_name);

				std::string int_max;
				if (AttributeExporter::parse_name(it->Type, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_keyword(ParentName, it->Name, int_max, keyword_name, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("keyword") != std::string::npos)
			{
				std::string keyword_name = FixAttributeUnderscore(it->Name);

				keyword_name = StripAttributeSuffix(keyword_name);

				if (it->Name == "resource-types")
				{
					keyword_name = "resource_type";
				}

				InsertTab(file_cpp, i_level + 2);
				file_cpp << AttributeExporter::get_keyword(ParentName, it->Name, "KEYWORD_MAX", keyword_name, bIsArray, bNoValue, bUnknown) << "\n";
			}
			else if (it->Type.find("name") != std::string::npos)
			{
				std::string int_max;
				if (AttributeExporter::parse_name(it->Type, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_name(ParentName, it->Name, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.find("octetString") != std::string::npos)
			{
				std::string int_max;
				if (AttributeExporter::parse_octetString(it->Type, int_max))
				{
					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_octetString(ParentName, it->Name, int_max, bIsArray, bNoValue, bUnknown) << "\n";
				}
				else
				{
					printf("TODO illformed type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
				}
			}
			else if (it->Type.compare("collection") == 0)
			{
				if (it->Members.empty())
				{
					printf("Ignoring empty collection '%s'\n", it->Name.c_str());
				}
				else
				{
					//FixCollectionReferences(Collections, *it);

					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_beginCollection(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
					
					ExportAttributeList(Collections, it->Members, RootName, ParentName + "." + it->Name, i_level + 1, i_ipp_1_1_only, params, file_h, file_cpp);

					InsertTab(file_cpp, i_level + 2);
					file_cpp << AttributeExporter::get_endCollection(ParentName, it->Name, bIsArray, bNoValue, bUnknown) << "\n";
				}
			}
			else
			{
				printf("TODO simple type '%s' for '%s'\n", it->Type.c_str(), it->Name.c_str());
			}
		}
	}
}

bool IsObsoleteOrDeprecated(const std::string& i_value, std::string& o_parent_value)
{
	o_parent_value.clear();
	size_t pos1 = i_value.find("(obsolete");
	size_t pos2 = i_value.find("(deprecated");
	if ((pos1 != std::string::npos) || (pos2 != std::string::npos))
	{
		auto pos = std::min(pos1, pos2);
		o_parent_value = trim(i_value.substr(0, pos));
		return true;
	}
	return false;
}

void RemoveAttribute(std::vector<AttributeType>& i_attributes, const std::string& i_name)
{
	std::vector<std::string> attribs = split(i_name, '.', false);
	if (attribs.size() > 0)
	{
		std::string root = attribs[0];
		attribs.erase(attribs.begin());
		std::string subname = join(attribs, '.');
		for (auto it = i_attributes.begin(); it != i_attributes.end(); )
		{
			if ((*it).Name == root)
			{
				if (attribs.empty())
				{
					it = i_attributes.erase(it);
				}
				else
				{
					RemoveAttribute(it->Members, subname);
					it++;
				}
			}
			else
			{
				it++;
			}
		}
	}
	else if (i_attributes.size())
	{
		printf("Unexpected attribute %s\n", i_name.c_str());
	}
}

void RemoveAttribute(std::vector<CollectionType>& i_collections, const std::string& i_name)
{
	std::vector<std::string> attribs = split(i_name, '.', false);
	if (attribs.size() > 0)
	{
		std::string root = attribs[0];
		attribs.erase(attribs.begin());
		std::string subname = join(attribs, '.');
		for (auto it = i_collections.begin(); it != i_collections.end(); )
		{
			if ((*it).Name == root)
			{
				if (attribs.empty())
				{
					it = i_collections.erase(it);
				}
				else
				{
					RemoveAttribute(it->Members, subname);
					it++;
				}
			}
			else
			{
				it++;
			}
		}
	}
	else if (i_collections.size())
	{
		printf("Unexpected attribute %s\n", i_name.c_str());
	}
}

bool ParseAttributesFiles(const std::vector<std::string>& i_infilenames, const bool i_ipp_1_1_only, std::vector<CollectionType>& collections, std::vector<KeywordType>& keywords)
{
	enum Tokens
	{
		Collection = 0,
		Name,
		MemberAttribute,
		SubmemberAttribute,
		Syntax,
		Reference,
		NumTokens
	};

	collections.clear();
	std::vector<LineTokens> lines = Tokenize(i_infilenames, true, NumTokens);
	if (lines.size())
	{
		std::string obsolete_value;
		std::vector<std::string> obsolete_attributes;

		std::string tmpName;
		for (auto it = lines.begin(); it != lines.end(); it++)
		{
			FixSyntax(it->at(Syntax));

			const std::string& collection = it->at(Collection);
			const std::string& name = it->at(Name);
			const std::string& member = it->at(MemberAttribute);
			const std::string& submember = it->at(SubmemberAttribute);
			const std::string& syntax = it->at(Syntax);

			if (syntax.find("collection") == std::string::npos)
			{
				const std::string& ref = it->at(Reference);
				if (!is_valid_reference(ref, i_ipp_1_1_only))
				{
					printf("Skipping reference %s - ref %s\n", name.c_str(), ref.c_str());
					continue;
				}
			}

			if (collections.empty() || (collections.back().Name != collection))
			{
				CollectionType newType;
				newType.Name = collection;
				collections.push_back(newType);
			}

			auto& thisCollection = collections.back();

			if (!name.empty())
			{
				AttributeType value;
				value.Type = syntax;

				if (!submember.empty())
				{
					if (IsObsoleteOrDeprecated(submember, obsolete_value))
					{
						std::string attribName = collection + "." + thisCollection.Members.back().Name + "." + thisCollection.Members.back().Members.back().Name + "." + obsolete_value;
						obsolete_attributes.push_back(attribName);
					}
					else
					{
						value.Name = submember;
						thisCollection.Members.back().Members.back().Members.push_back(value);
					}
				}
				else if (!member.empty())
				{
					if (IsObsoleteOrDeprecated(member, obsolete_value))
					{
						std::string attribName = collection + "." + thisCollection.Members.back().Name + "." + obsolete_value;
						obsolete_attributes.push_back(attribName);
					}
					else
					{
						value.Name = member;
						thisCollection.Members.back().Members.push_back(value);
					}
				}
				else
				{
					if (IsObsoleteOrDeprecated(name, obsolete_value))
					{
						std::string attribName = collection + "." + obsolete_value;
						obsolete_attributes.push_back(attribName);
					}
					else
					{
						value.Name = name;
						thisCollection.Members.push_back(value);
					}
				}
			}
		}

		//Remove obsolete attributes
		for (auto it = obsolete_attributes.cbegin(); it != obsolete_attributes.cend(); it++)
		{
			RemoveAttribute(collections, *it);
		}

		//Remove empty collections
		for (auto it = collections.begin(); it != collections.end(); it++)
		{
			auto& thisMembers = it->Members;
			for (auto it2 = thisMembers.begin(); it2 != thisMembers.end(); )
			{
				bool bInc2 = true;
				auto& thisMember = *it2;
				if (thisMember.Type.find("collection") != std::string::npos)
				{
					if (!thisMember.Members.empty())
					{
						auto& thisMembers3 = thisMember.Members;
						for (auto it3 = thisMembers3.begin(); it3 != thisMembers3.end();)
						{
							bool bInc3 = true;
							auto& thisMember3 = *it3;
							if (thisMember3.Type.find("collection") != std::string::npos)
							{
								if (!thisMember3.Members.empty())
								{
									auto& thisMembers4 = thisMember3.Members;
									for (auto it4 = thisMembers4.begin(); it4 != thisMembers4.end(); )
									{
										bool bInc4 = true;
										auto& thisMember4 = *it4;
										if (thisMember4.Type.find("collection") != std::string::npos)
										{
											if (!thisMember4.Members.empty())
											{

											}
											if (thisMember4.Members.empty())
											{
												it4 = thisMembers4.erase(it4);
												bInc4 = false;
											}
										}
										if (bInc4)
										{
											it4++;
										}
									}
								}
								if (thisMember3.Members.empty())
								{
									it3 = thisMembers3.erase(it3);
									bInc3 = false;
								}
							}

							if (bInc3)
							{
								it3++;
							}
						}
					}
					if (thisMember.Members.empty())
					{
						it2 = thisMembers.erase(it2);
						bInc2 = false;
					}
				}
				if (bInc2)
				{
					it2++;
				}
			}
		}
	}

	if (collections.size())
	{
		FixAttributesNames(collections);

		FixCollectionReferences(collections);

		GenerateAttributesKeywords(collections, keywords, i_ipp_1_1_only);
	}

	return !collections.empty();
}

struct EnumValue
{
	std::string name;
	std::string value;
};

struct Enum
{
	std::string name;
	std::string reference;
	std::vector<EnumValue> values;
};

void ExportEnumValues(const Enum& i_enum, const ExportParams& params, std::ofstream& file_h, std::ofstream& file_cpp)
{
	const bool i_export_as_enum = params.export_as_enum;
	const bool i_export_enum_reference_as_standalone = params.export_enum_reference_as_standalone;

	std::string enum_type = FixAttributeUnderscore(i_enum.name);
	if (i_export_as_enum)
	{
		file_h << "\tenum " << enum_type << " : int\n\t{\n";

		file_cpp << "\tconst std::pair<int, const char*> s_" << enum_type << "_description[" << i_enum.values.size() << "] =\n\t{\n";
	}
	else
	{
		if (i_export_enum_reference_as_standalone || i_enum.reference.empty())
		{
			file_h << "\tclass " << enum_type << "\n\t{\n\tpublic:\n";

			file_cpp << "\tconst std::pair<int, const char*> " << enum_type << "::description[" << i_enum.values.size() << "] =\n\t{\n";
		}
		else
		{
			std::string base_enum_type = FixAttributeUnderscore(i_enum.reference);
			file_h << "\tclass " << enum_type << " : public " << base_enum_type << " {};\n";
			return;
		}
	}

	std::string line_h;
	for (auto it = i_enum.values.cbegin(); it != i_enum.values.cend(); it++)
	{
		std::string enum_name = FixAttributeUnderscore(it->name);
		const std::string& enum_desc = it->name;
		if (i_export_as_enum)
		{
			line_h = enum_name + " = " + it->value;
		}
		else
		{
			line_h = "constexpr static const int " + enum_name + " = " + it->value;
		}

		file_h << "\t\t" << line_h;
		file_cpp << "\t\t" << "{ " << it->value << ", \"" << enum_desc << "\" }";
		if (i_export_as_enum)
		{
			if (it != (i_enum.values.cend() - 1))
			{
				file_h << ",";
			}
		}
		else
		{
			file_h << ";";
		}
		file_h << "\n";

		if (it != (i_enum.values.cend() - 1))
		{
			file_cpp << ",";
		}
		file_cpp << "\n";
	}
	if (!i_export_as_enum)
	{
		file_h << "\n\t\tstatic const std::pair<int, const char*> description[" << i_enum.values.size() << "];\n";
		file_h << "\n\t\tstatic const char* get_description(const int i_value);\n";
	}
	file_h << "\t};\n";
	if (i_export_as_enum)
	{
		file_h << "\n\t\textern const std::pair<int, const char*> s_" << enum_type << "_description[" << i_enum.values.size() << "];\n";
		file_h << "\n\t\textern const char* get_description(const " << enum_type << " i_value);\n";
	}
	file_cpp << "\t};\n";
	if (i_export_as_enum)
	{
		file_cpp << "\n\tconst char* get_description(const " << enum_type << " i_value)\n";
		file_cpp << "\t{\n";
		file_cpp << "\t\treturn get_enum_description(&s_" << enum_type << "_description[0], " << i_enum.values.size() << ", i_value);\n";
		file_cpp << "\t}\n";
	}
	else
	{
		file_cpp << "\n\tconst char* " << enum_type << "::get_description(const int i_value)\n";
		file_cpp << "\t{\n";
		file_cpp << "\t\treturn get_enum_description(&description[0], " << i_enum.values.size() << ", i_value);\n";
		file_cpp << "\t}\n";
	}
}

bool ParseStatusCodesFile(const std::string& i_infilename, const bool i_ipp_1_1_only, Enum& statuscodes)
{
	enum Tokens
	{
		Value = 0,
		Name,
		Reference,
		NumTokens
	};

	statuscodes = Enum();
	statuscodes.name = "status-codes";
	statuscodes.reference.clear();

	std::vector<LineTokens> lines = Tokenize(i_infilename, true, NumTokens);
	if (lines.size())
	{
		std::string tmpName;
		for (auto it = lines.begin(); it != lines.end(); it++)
		{
			const std::string& value = it->at(Value);
			const std::string& name = it->at(Name);
			const std::string& ref = it->at(Reference);

			/*if (ref.empty())
			{
				printf("Missing reference: %s -> %s\n", name.c_str(), value.c_str());
				continue;
			}
*/
			if (!is_valid_reference(ref, i_ipp_1_1_only))
			{
				printf("Skipping reference %s -> %s - ref %s\n", name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			//Is range status code (value=0x0008-0x007F)?
			if (value.find('-') != std::string::npos)
			{
				printf("Skipping range status code: %s -> %s -> %s\n", name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			//Is name reserved or does it contain any parenthesis?
			if ((name.find('(') != std::string::npos) || (name.find("Reserved") != std::string::npos))
			{
				printf("Skipping status code: %s -> %s -> %s\n", name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			EnumValue code;
			code.name = name;
			code.value = value;
			statuscodes.values.push_back(code);
		}
	}

	return !statuscodes.values.empty();
}

bool ParseEnumsFiles(const std::vector<std::string>& i_infilenames, const bool i_ipp_1_1_only, const Enum& statuscodes, std::vector<Enum>& enums)
{
	enum Tokens
	{
		Attribute = 0,
		Value,
		Name,
		Syntax,
		Reference,
		NumTokens
	};

	enums.clear();
	std::map<std::string, std::string> ref_enums;
	std::multimap<std::string, std::string> obsolete_enums;

	std::vector<LineTokens> lines = Tokenize(i_infilenames, true, NumTokens);
	if (lines.size())
	{
		std::string tmpName;
		for (auto it = lines.begin(); it != lines.end(); it++)
		{
			//Remove type1, type2, parenthesis and other stuff
			FixSyntax(it->at(Syntax));

			const std::string& attribute = it->at(Attribute);
			const std::string& value = it->at(Value);
			const std::string& name = it->at(Name);
			const std::string& syntax = it->at(Syntax);
			const std::string& ref = it->at(Reference);

			/*if (ref.empty())
			{
				printf("Missing reference: %s -> %s -> %s\n", attribute.c_str(), name.c_str(), value.c_str());
				continue;
			}
*/
			if (!is_valid_reference(ref, i_ipp_1_1_only))
			{
				printf("Skipping reference %s -> %s -> %s -> %s\n", attribute.c_str(), name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			//New enum type found (value.empty)
			if (enums.empty() || value.empty())
			{
				Enum val;
				val.name = attribute;
				enums.push_back(val);
				continue;
			}

			//Does value reference another enum type?
			if (value.find('<') != std::string::npos)
			{
				size_t start = value.find("\"\"");
				size_t end = std::string::npos;
				if (start != std::string::npos)
				{
					end = value.find("\"\"", start + 2);
				}
				if ((start != std::string::npos) && (end != std::string::npos))
				{
					std::string ref_attribute = value.substr(start + 2, end - start - 2);
					trim(ref_attribute);

					ref_enums[attribute] = ref_attribute;
				}
				else
				{
					printf("Unexpected value entry: %s -> %s -> %s. Fix the parser.\n", attribute.c_str(), name.c_str(), value.c_str());
				}
				continue;
			}

			//Has this enum value an empty name?
			if (name.empty())
			{
				printf("Skipping empty name: %s -> %s -> %s\n", attribute.c_str(), name.c_str(), value.c_str());
				continue;
			}

			//Can this enum value be ignored (not used, obsolete, deprecated, reserved, etc)?
			if (name.find('(') != std::string::npos)
			{
				//If this value is obsolete or deprecated, remove it later from the list.
				if ((name.find("(obsolete") != std::string::npos) || (name.find("(deprecated") != std::string::npos))
				{
					auto pos = name.find('(');
					auto to_be_removed = std::pair<std::string, std::string>(enums.back().name, trim(name.substr(0, pos)));
					obsolete_enums.insert(to_be_removed);
				}

				printf("Skipping value code: %s -> %s -> %s\n", attribute.c_str(), name.c_str(), value.c_str());
				continue;
			}

			auto& last = enums.back();

			EnumValue val;
			val.name = name;
			val.value = value;
			last.values.push_back(val);
		}
	}

	//Remove obsolete values
	for (auto it = obsolete_enums.cbegin(); it != obsolete_enums.cend(); it++)
	{
		auto it_enum = std::find_if(enums.begin(), enums.end(), [name = it->first](auto& elem)->bool { return elem.name == name; });
		if (it_enum != enums.end())
		{
			for (auto it_value = it_enum->values.begin(); it_value != it_enum->values.end(); )
			{
				if (it_value->name == it->second)
				{
					it_value = it_enum->values.erase(it_value);
				}
				else
				{
					it_value++;
				}
			}
		}
	}

	//Resolve referenced dependency.
	for (auto it = ref_enums.cbegin(); it != ref_enums.cend(); it++)
	{
		auto it_a = std::find_if(enums.begin(), enums.end(), [attr = it->first](auto& elem)->bool { return elem.name == attr; });
		auto it_ref = std::find_if(enums.begin(), enums.end(), [attr = it->second](auto& elem)->bool { return elem.name == attr; });
		if ((it_a != enums.end()) && (it_ref != enums.end()))
		{
			it_a->reference = it_ref->name;
			it_a->values = it_ref->values;
		}
		else
		{
			if (it_a != enums.end())
			{
				if ((it->second == "status-code") && statuscodes.name.size())
				{
					//it_a->reference = statuscodes.name;
					it_a->values = statuscodes.values;	
					//remove 'successful-ok'
					it_a->values.erase(std::remove_if(it_a->values.begin(), it_a->values.end(), [](auto& elem)->bool { return elem.name == "successful-ok"; }));
				}
				else
				{
					enums.erase(it_a);
					printf("Skipping enum %s -> missing reference %s\n", it->first.c_str(), it->second.c_str());
				}
			}
			else
			{
				printf("Error looking for attribute %s -> referencing %s\n", it->first.c_str(), it->second.c_str());
			}
		}
	}

	//Fix and optionally remove default, ready and supported enum names.
	for (auto it = enums.begin(); it != enums.end(); )
	{
		if ((it->name.find("-default") != std::string::npos) || (it->name.find("-ready") != std::string::npos) || ((it->name.find("-supported") != std::string::npos)))
		{
			std::string attr = it->name;
			if (attr.find("-default") != std::string::npos)
			{
				replace_all(attr, "-default", "");
			}
			else if (attr.find("-ready") != std::string::npos)
			{
				replace_all(attr, "-ready", "");
			}
			else
			{
				replace_all(attr, "-supported", "");
			}

			auto fn_find_enum_by_name = [attr](auto& elem)->bool { return elem.name == attr; };
			auto fn_replace_reference = [old_ref = it->name, new_ref = attr](auto& elem)-> void { if (elem.reference == old_ref) { elem.reference = new_ref; }};

			auto it_found = std::find_if(enums.cbegin(), enums.cend(), fn_find_enum_by_name);
			if (it_found != enums.cend())
			{
				printf("Removing enum %s -> use %s\n", it->name.c_str(), attr.c_str());

				std::for_each(enums.begin(), enums.end(), fn_replace_reference);

				it = enums.erase(it);
			}
			else
			{
				if (it->name == "operations-supported")
				{
					std::for_each(enums.begin(), enums.end(), fn_replace_reference);

					it->name = "operations";
				}

				it++;
			}
		}
		else
		{
			it++;
		}
	}

	//Sort enums by reference: move enums with references just after their base enum.
	bool bSwapped = true;
	while (bSwapped)
	{
		bSwapped = false;
		for (auto it = enums.begin(); it != enums.end(); it++)
		{
			if (it->reference.size())
			{
				auto fn_find_enum_by_name = [attr = it->reference](auto& elem)->bool { return elem.name == attr; };

				auto it_ref = std::find_if(it + 1, enums.end(), fn_find_enum_by_name);
				if (it_ref != enums.cend())
				{
					if (it_ref == (it + 1))
					{
						std::swap(*it, *it_ref);
					}
					else
					{
						Enum tmp = std::move(*it);
						size_t pos = it - enums.begin();
						size_t last = it_ref - enums.begin();
						for (size_t p = pos; p < last; p++)
						{
							enums.at(p) = enums.at(p + 1);
						}
						enums.at(last) = std::move(tmp);
						it--;
					}

					bSwapped = true;
				}
			}
		}
	}

	return !enums.empty();
}

std::vector<KeywordType> EnumsToKeywords(const std::vector<Enum>& i_enums)
{
	std::vector<KeywordType> keywords;
	keywords.reserve(i_enums.size());
	for (auto it = i_enums.cbegin(); it != i_enums.end(); it++)
	{
		KeywordType key;
		key.Name = it->name;
		key.ParentAttribute = it->reference;
		key.Type.clear();
		
		key.Values.reserve(it->values.size());
		for (auto itv = it->values.cbegin(); itv != it->values.cend(); itv++)
		{
			key.Values.push_back(itv->name);
		}
		keywords.push_back(std::move(key));
	}
	return keywords;
}

bool is_valid_tag_name(const std::string& i_name)
{
	if (i_name.empty())
	{
		return false;
	}

	//Name MUST start with lowercase letter
	if ((i_name[0] < 'a') || (i_name[0] > 'z'))
	{
		return false;
	}

	//Name MUST contain letters or hyphen
	auto is_not_alpha_or_hyphen = [](auto& c)->bool { return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '-')); };
	if (std::find_if(i_name.cbegin(), i_name.cend(), is_not_alpha_or_hyphen) != i_name.cend())
	{
		return false;
	}

	return true;
}

bool is_valid_tag_value(const std::string& i_value)
{
	if (i_value.empty())
	{
		return false;
	}

	//Range of values (0x60-0x7E)
	if (i_value.find('-') != std::string::npos)
	{
		return false;
	}

	//Invalid values (Note above)
	if (!begins_with(i_value, "0x"))
	{
		return false;
	}

	return true;
}

bool ParseTagsFiles(const std::vector<std::string>& i_infilenames, const bool i_ipp_1_1_only, Enum& tags)
{
	enum Tokens
	{
		Value = 0,
		Name,
		Reference,
		NumTokens
	};

	tags = Enum();
	tags.name = "tags";
	tags.reference.clear();

	std::vector<LineTokens> lines = Tokenize(i_infilenames, true, NumTokens);

	if (lines.size())
	{
		std::string tmpName;
		for (auto it = lines.begin(); it != lines.end(); it++)
		{
			//Fix some names
			if ((it->at(Name) == "Reserved") && (it->at(Value) == "0x00"))
			{
				it->at(Name) = "begin-attribute-group-tag";
			}
			else if (it->at(Name) == "enum")
			{
				it->at(Name) = "enum-tag";
			}
			else if (it->at(Name).find("octetString") == 0)
			{
				it->at(Name) = "octetString";
			}
			else if (it->at(Name).find("extension") != std::string::npos)
			{
				it->at(Name) = "extension";
				it->at(Reference) = "[RFC8010]";
			}
			replace_all(it->at(Name), "\"", "");
			trim(it->at(Name));

			const std::string& value = it->at(Value);
			const std::string& name = it->at(Name);
			const std::string& ref = it->at(Reference);

			//if (ref.empty())
			//{
			//	printf("Missing reference: %s -> %s\n", name.c_str(), value.c_str());
			//	continue;
			//}

			if (!is_valid_reference(ref, i_ipp_1_1_only))
			{
				printf("Skipping reference %s -> %s -> %s\n", name.c_str(), value.c_str(), ref.c_str());
				continue;
			}

			//Range of values (0x60-0x7E)
			if (!is_valid_tag_value(value))
			{
				printf("Skipping invalid tag value: %s -> %s -> %s\n", value.c_str(), name.c_str(), ref.c_str());
				continue;
			}

			//Name contains parenthesis or spaces
			if (!is_valid_tag_name(name))
			{
				printf("Skipping invalid tag name: %s -> %s -> %s\n", value.c_str(), name.c_str(), ref.c_str());
				continue;
			}

			EnumValue code;
			code.name = name;
			code.value = value;
			tags.values.push_back(code);
		}
	}

	return !tags.values.empty();
}

void SaveKeywords(std::vector<KeywordType>& i_keywords, const ExportParams& params, std::ofstream& file_h, std::ofstream& file_cpp)
{
	std::string tmpMacro;
	std::set<std::string> alreadyDone;
	for (auto it = i_keywords.begin(); it != i_keywords.end(); it++)
	{
		std::string keyword_name = GenerateKeywordName(it->Name);

		if (alreadyDone.find(keyword_name) != alreadyDone.end())
		{
			//Keyword redefinition?
			printf("Keyword already processed: %s -> %s\n", it->Name.c_str(), keyword_name.c_str());
			continue;
		}

		if ((it->ParentAttribute.empty()) && (it->Values.empty()))
		{
			printf("TODO '%s': values %d\n", it->Name.c_str(), static_cast<int32_t>(it->Values.size()));
			it->Values.push_back("TODO");
		}

		if ((it->ParentAttribute.empty()) && (!it->Values.empty()))
		{
			size_t max_value_length = 0;
			for (auto it2 = it->Values.cbegin(); it2 != it->Values.cend(); it2++)
			{
				max_value_length = std::max(max_value_length, it2->size());
			}

			std::string macro_name = GenerateKeywordNameCamelCase(it->Name);

			std::string variable = "s_" + keyword_name + "_keywords";
			std::string macro_prefix = "#define KEYWORD_" + macro_name + "_";
			size_t expected_macro_prefix_length = macro_prefix.size() + max_value_length;

			file_h << "\textern const char* " << variable << "[" << it->Values.size() << "];\n\n";
			file_cpp << "\tconst char* " << variable << "[" << it->Values.size() << "] = {\n";

			for (auto it2 = it->Values.cbegin(); it2 != it->Values.cend(); it2++)
			{
				if (it2 != (it->Values.cend() - 1))
				{
					file_cpp << "\t\t\"" << *it2 << "\",\n";
				}
				else
				{
					file_cpp << "\t\t\"" << *it2 << "\"\n";
				}

				tmpMacro = macro_prefix + FixAttributeCamelCase(*it2);
				while (tmpMacro.size() < expected_macro_prefix_length)
				{
					tmpMacro.push_back(' ');
				}

				file_h << tmpMacro << "\t" << "(ipp::" << variable << "[" << (it2 - it->Values.cbegin()) << "])\n";
			}

			file_h << "\n";
			file_cpp << "\t};\n\n";

			alreadyDone.insert(keyword_name);
		}
		else if ((!it->ParentAttribute.empty()) /*&& (it->Values.empty())*/ && !params.ignore_attribute_and_keywords_bindings)
		{
			std::string parent_name = GenerateKeywordName(it->ParentAttribute);

			if (keyword_name != parent_name)
			{
				auto fn_find_parent_keyword = [parent_name](auto& elem)->bool { return (GenerateKeywordName(elem.Name) == parent_name) && !elem.Values.empty(); };
				auto it_parent = std::find_if(i_keywords.begin(), i_keywords.end(), fn_find_parent_keyword);
				if (it_parent != i_keywords.end())
				{
					std::string variable = "s_" + keyword_name + "_keywords";
					std::string parent = "s_" + parent_name + "_keywords";
					std::string macro = "#define " + variable + " " + parent;

					file_h << macro << "\n\n";

					size_t max_value_length = 0;
					for (auto it2 = it_parent->Values.cbegin(); it2 != it_parent->Values.cend(); it2++)
					{
						max_value_length = std::max(max_value_length, it2->size());
					}

					std::string macro_name_parent = GenerateKeywordNameCamelCase(it->ParentAttribute);
					std::string macro_prefix_parent = "KEYWORD_" + macro_name_parent + "_";

					std::string macro_name = GenerateKeywordNameCamelCase(it->Name);
					std::string macro_prefix = "#define KEYWORD_" + macro_name + "_";
					size_t expected_macro_prefix_length = macro_prefix.size() + max_value_length;

					std::string tmpValueName;
					std::string tmpMacroParent;
					for (auto it2 = it_parent->Values.cbegin(); it2 != it_parent->Values.cend(); it2++)
					{
						tmpValueName = FixAttributeCamelCase(*it2);
						tmpMacro = macro_prefix + tmpValueName;
						while (tmpMacro.size() < expected_macro_prefix_length)
						{
							tmpMacro.push_back(' ');
						}
						tmpMacroParent = macro_prefix_parent + tmpValueName;

						file_h << tmpMacro << "\t" << tmpMacroParent << "\n";
					}

					file_h << "\n";
				}
				else
				{
					printf("Unexpected missing parent keyword: %s -> %s\n", it->Name.c_str(), it->ParentAttribute.c_str());
				}
			}

			alreadyDone.insert(keyword_name);
		}
		else
		{
			printf("Ignoring '%s': values %d - parent '%s'\n", it->Name.c_str(), static_cast<int32_t>(it->Values.size()), it->ParentAttribute.c_str());
		}
	}
}

void SaveEnums(const std::vector<Enum>& enums, const bool i_ipp_1_1_only, const ExportParams& params,  std::ofstream& file_h, std::ofstream& file_cpp)
{
	if (enums.size())
	{
		for (auto it_e = enums.cbegin(); it_e != enums.cend(); it_e++)
		{
			if (it_e != enums.cbegin())
			{
				file_h << "\n";
				file_cpp << "\n";
			}

			ExportEnumValues(*it_e, params, file_h, file_cpp);
		}
	}
}

void SaveAttributes(std::vector<CollectionType>& collections, const bool i_ipp_1_1_only, const ExportParams& params, std::ofstream& file_h, std::ofstream& file_cpp)
{
	if (collections.size())
	{
		file_cpp <<
			"#define ADD_BOOLEAN(root_, name_, tag_, Array_, NoValue_, Unknown_)				AttributeRegistry::add_boolean(root_, name_, tag_, Array_, NoValue_, Unknown_)\n"
			"#define ADD_INTEGER(root_, name_, tag_, Min_, Max_, Array_, NoValue_, Unknown_)	AttributeRegistry::add_integer(root_, name_, tag_, Min_, Max_, Array_, NoValue_, Unknown_)\n"
			"#define ADD_KEYWORD(root_, name_, tag_, Max_, Array_, NoValue_, Unknown_, Enum_)	AttributeRegistry::add_keyword(root_, name_, tag_, Max_, Array_, NoValue_, Unknown_, Enum_, _countof(Enum_))\n"
			"#define ADD_ENUM(root_, name_, tag_, Array_, NoValue_, Unknown_, Enum_)			AttributeRegistry::add_enum(root_, name_, tag_, Array_, NoValue_, Unknown_, Enum_, _countof(Enum_))\n"
			"#define ADD_RANGE(root_, name_, tag_, Min_, Max_, Array_, NoValue_, Unknown_)		AttributeRegistry::add_rangeOfInteger(root_, name_, tag_, Min_, Max_, Array_, NoValue_, Unknown_)\n"
			"#define ADD_RESOLUTION(root_, name_, tag_, Array_, NoValue_, Unknown_)				AttributeRegistry::add_resolution(root_, name_, tag_, Array_, NoValue_, Unknown_)\n"
			"#define ADD_STRING(root_, name_, tag_, Max_, Array_, NoValue_, Unknown_)			AttributeRegistry::add_string(root_, name_, tag_, Max_, Array_, NoValue_, Unknown_)\n"
			"#define ADD_DATETIME(root_, name_, tag_, Array_, NoValue_, Unknown_)				AttributeRegistry::add_datetime(root_, name_, tag_, Array_, NoValue_, Unknown_)\n"
			"#define BEGIN_COLLECTION(root_, name_, tag_, Array_, NoValue_, Unknown_)			AttributeRegistry::add_collection(root_, name_, tag_, Array_, NoValue_, Unknown_)\n"
			"#define END_COLLECTION(...)	(void)0\n\n";

		file_cpp << "\tvoid init_attributes()\n\t{\n";

		//ExportAttributesKeywords(collections, file_h, file_cpp);

		for (auto itC = collections.begin(); itC != collections.end(); itC++)
		{
			CollectionType& mainCollection = *itC;

			ExportAttributeList(collections, mainCollection.Members, mainCollection.Name, mainCollection.Name, 0, i_ipp_1_1_only, params, file_h, file_cpp);
		}
		file_cpp << "\t}\n";
	}
}

void exportConstants(const std::string& outfiledir, const std::string& outfilename, const std::string& fileprefix, const std::string& namespacebegin, const std::string& namespaceend,
	const bool ipp_1_1_only, const ExportParams& params)
{
	std::ofstream file_cpp, file_h;
	file_cpp.open(outfiledir + outfilename + ".cpp");
	file_h.open(outfiledir + outfilename + ".h");
	if (file_cpp.is_open() && file_h.is_open())
	{
		file_h << fileprefix;
		file_cpp << fileprefix;

		std::string namespace_ipp = "ipp";

		file_h << "#pragma once\n\n" << namespacebegin << "\nnamespace " << namespace_ipp << "\n{\n";
		file_cpp << "#include \"pch.h\"\n#include \"" << outfilename << ".h\"\n#include \"ipp_attributes.h\"\n\n" << namespacebegin << "\nnamespace " << namespace_ipp << "\n{\n";

		file_cpp << "\textern const char* get_enum_description(const std::pair<int, const char*>* i_values, const size_t i_size, const int i_value);\n\n";

		Enum status_codes, tags;
		std::vector<Enum> enums;
		if (!ParseStatusCodesFile("ipp\\csv\\ipp-registrations-11.csv", ipp_1_1_only, status_codes))
		{
			status_codes.name.clear();
		}
		if (!ParseTagsFiles({ "ipp\\csv\\ipp-registrations-7.csv" , "ipp\\csv\\ipp-registrations-8.csv" , "ipp\\csv\\ipp-registrations-9.csv" }, ipp_1_1_only, tags))
		{
			tags.name.clear();
		}
		ParseEnumsFiles({ "ipp\\csv\\ipp-registrations-6.csv", "ipp\\csv\\ipp-my-additional-enums.csv" }, ipp_1_1_only, status_codes, enums);
		if (tags.name.size())
		{
			enums.insert(enums.begin(), tags);
		}
		if (status_codes.name.size())
		{
			enums.insert(enums.begin(), status_codes);
		}
		
		std::vector<KeywordType> keywords;
		ParseKeywordsFiles({ "ipp\\csv\\ipp-registrations-4.csv", "ipp\\csv\\ipp-my-additional-keywords.csv" }, ipp_1_1_only, keywords);

		std::vector<KeywordType> enumskeys = EnumsToKeywords(enums);
		keywords.insert(keywords.end(), std::make_move_iterator(enumskeys.begin()), std::make_move_iterator(enumskeys.end()));
		
		std::vector<CollectionType> attributes;
		ParseAttributesFiles({ "ipp\\csv\\ipp-registrations-2.csv", "ipp\\csv\\ipp-my-additional-attributes.csv" }, ipp_1_1_only, attributes, keywords);

		SaveEnums(enums, ipp_1_1_only, params, file_h, file_cpp);
		
		file_h << "\n";

		SaveKeywords(keywords, params, file_h, file_cpp);
		SaveAttributes(attributes, ipp_1_1_only, params, file_h, file_cpp);

		file_h << "}\t//namespace " << namespace_ipp << "\n" << namespaceend;
		file_cpp << "}\t//namespace " << namespace_ipp << "\n" << namespaceend;

		file_h.close();
		file_cpp.close();
	}
}

void exportConstants()
{
	tm tm_;
	time_t t = time(nullptr);
	localtime_s(&tm_, &t);
	std::ostringstream oss;
	oss << std::put_time(&tm_, "%d/%m/%Y %H:%M:%S");

	std::string fileprefix =
		"///////////////////////////////////////////////////////////////////////////////\n"
		"//\n"
		"// THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT MODIFY IT BY HAND.\n"
		"//\n"
		"// " + oss.str() + "\n"
		"//\n"
		"///////////////////////////////////////////////////////////////////////////////\n\n";

	std::string outfiledir = "ipp\\";
	std::string outfilename = "ipp_gen_types";

	const bool ipp_1_1_only = true;

	ExportParams params{};
	params.export_as_enum = false;	//due to conflicting names
	params.export_enum_reference_as_standalone = false;
	params.ignore_attribute_and_keywords_bindings = false;

	exportConstants(outfiledir, outfilename + "_1_1", fileprefix, "#if !IPP_SUPPORTS_2_0_ATTRIBUTES\n", "\n#endif\t//!IPP_SUPPORTS_2_0_ATTRIBUTES\n", ipp_1_1_only, params);
	exportConstants(outfiledir, outfilename + "_2_0", fileprefix, "#if IPP_SUPPORTS_2_0_ATTRIBUTES\n", "\n#endif\t//IPP_SUPPORTS_2_0_ATTRIBUTES\n", !ipp_1_1_only, params);
}
