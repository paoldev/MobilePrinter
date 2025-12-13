#include "pch.h"
#include "ipp_types.h"
#include "ipp_packet.h"
#include "CommonUtils.h"
#include <set>

#ifdef _DEBUG
extern void testFromBuffer(const std::vector<uint8_t>&);
#endif

namespace ipp
{
	extern std::vector<int32_t> find_attribute_tag(const char* AttributeName);

	const char* get_enum_description(const std::pair<int, const char*>* i_values, const size_t i_size, const int i_value)
	{
		if (i_values && i_size)
		{
			for (size_t i = 0; i < i_size; i++)
			{
				if (i_values[i].first == i_value)
				{
					return i_values[i].second;
				}
			}
		}

		return "<unknown description>";
	}

	packet::packet() : m_version_major(0), m_version_minor(0), m_operation_or_status_code(0), m_request_id(0), m_position(0), m_datasize(0), m_data(nullptr)
	{
	}

	void packet::SetAttribute(const int32_t i_group, const std::string& i_attribute, const variant& i_value)
	{
		auto it = m_ipp_attributes.find(i_group);
		if (it == m_ipp_attributes.end())
		{
			m_ipp_attributes.insert(std::make_pair(i_group, collection()));
			it = m_ipp_attributes.find(i_group);
		}

		if (it != m_ipp_attributes.end())
		{
			it->second[i_attribute].push_back(i_value);
		}
	}

	packet::group_it packet::AddGroup(const int32_t i_group)
	{
		return m_ipp_attributes.insert(std::make_pair(i_group, collection()));
	}

	void packet::SetAttribute(packet::group_it i_group, const std::string& i_attribute, const variant& i_value)
	{
		if (i_group != m_ipp_attributes.end())
		{
			i_group->second[i_attribute].push_back(i_value);
		}
	}

	void packet::SetAttribute(group_it i_group, const std::string& i_attribute, const std::vector<variant>& i_values)
	{
		if (i_group != m_ipp_attributes.end())
		{
			auto& attribs = i_group->second[i_attribute];
			for (auto it = i_values.cbegin(); it != i_values.cend(); it++)
			{
				attribs.push_back(*it);
			}
		}
	}

	bool packet::parse(const uint8_t* pData, const size_t datasize)
	{
		m_readError = 0;
		m_position = 0;
		m_datasize = datasize;
		m_data = pData;
		m_ipp_attributes.clear();
		m_ipp_data.clear();
		m_groups_from_request.clear();

		m_version_major = read1();
		m_version_minor = read1();
		m_operation_or_status_code = read2();
		m_request_id = read4();

		if (!isError())
		{
			read_attribute_groups();
		}

		if (!isError())
		{
			if (m_position < m_datasize)
			{
				m_ipp_data.assign(&m_data[m_position], &m_data[m_datasize]);
			}
		}

		return !isError();
	}

	std::string packet::read(const int32_t length, const char* encoding)
	{
		if ((isError()) || (length == 0))
		{
			return "";
		}

		if ((m_position + length) <= m_datasize)
		{
			m_position += length;
			return std::string(reinterpret_cast<const char*>(&m_data[m_position - length]), length);
		}

		m_readError = 1;
		return "";
	}

	void packet::read_attribute_groups()
	{
		int32_t group;
		while ((!isError()) && (m_position < m_datasize) && ((group = read1()) != tags::end_of_attributes_tag))
		{
			auto itGroup = m_ipp_attributes.insert(std::make_pair(group, collection()));

			m_groups_from_request.push_back(group);

			read_attribute_group(itGroup);
		}
	}

	void packet::read_attribute_group(group_it group)
	{
		if (!isError())
		{
			while ((!isError()) && (m_data[m_position] >= 0x0F))
			{
				read_attribute(group);
			}
		}
	}

	void packet::read_attribute(group_it group)
	{
		int32_t tag = read1();
		if (tag == tags::extension)
		{
			tag = read4();
		}
		std::string name = read(read2());

		if (!isError())
		{
			group->second[name] = read_values(tag, name);
		}
	}

	bool packet::has_array_value()
	{
		if ((!isError()) && ((m_position + 2) < m_datasize))
		{
			int32_t current = m_data[m_position];
			return ((current != tags::end_of_attributes_tag) &&
				(current != tags::endCollection) &&
				(current != tags::memberAttrName) &&
				(m_data[m_position + 1] == 0x00) &&
				(m_data[m_position + 2] == 0x00));
		}
		return false;
	}

	std::vector<variant> packet::read_values(int32_t type, const std::string& name)
	{
		std::vector<variant> value;
		value.push_back(read_value(type, name));
		while (has_array_value())
		{
			type = read1();
			read2();//ignore name length (empty)
			value.push_back(read_value(type, name));
		}
		return value;
	}

	variant packet::read_value(const int32_t tag, const std::string& name)
	{
		int32_t length = read2();
		variant res;

		if (isError())
		{
			return res;
		}

		switch (tag)
		{
		case tags::enum_tag:
			res.Set(read4());
			break;

		case tags::integer:
			res.Set(read4());
			break;

		case tags::boolean:
			res.Set(read1() != 0);
			break;

		case tags::rangeOfInteger:
		{
			int32_t param1 = read4();
			int32_t param2 = read4();
			res.Set(std::make_pair(param1, param2));
		}
		break;

		case tags::resolution:
		{
			int32_t param1 = read4();
			int32_t param2 = read4();
			int32_t param3 = read1(); /*== 0x03 ? "dpi" : "dpcm"*/
			res.Set(std::make_tuple(param1, param2, param3));
		}
		break;

		case tags::dateTime:
		{
			// http://tools.ietf.org/html/rfc1903 page 17

			date_time date = {};
			date.year = read2();
			date.month = read1();
			date.day = read1();
			date.hour = read1();
			date.minute = read1();
			date.second = read1();
			date.millisecond = 100 * read1();
			date.isPositive = (read1() != '-');//?
			date.TZHour = read1();
			date.TZMinute = read1();
			date.TZIsLocal = false;//?
			date.TZIsPositive = (date.TZHour > 0) ? true : ((date.TZHour < 0) ? false : (date.TZMinute >= 0));	//?
			res.Set(date);
		}
		break;

		case tags::textWithLanguage:
		case tags::nameWithLanguage:
		{
			int32_t languageLen = read2();
			std::string language = read(languageLen);
			int32_t textLen = read2();
			std::string text = read(textLen);
			res.Set(text, language);
		}
		break;

		case tags::nameWithoutLanguage:
		case tags::textWithoutLanguage:
		case tags::octetString:
		case tags::memberAttrName:
			res.Set(read(length));
			break;

		case tags::keyword:
		case tags::uri:
		case tags::uriScheme:
		case tags::charset:
		case tags::naturalLanguage:
		case tags::mimeMediaType:
			res.Set(read(length, "ascii"));
			break;

		case tags::begCollection:
			(void)read(length);
			res.Set(read_collection());
			break;

		case tags::no_value:
			res.Set(variant::novalue);
			break;

		case tags::unsupported:
			res.Set(variant::unsupported);
			break;

		case tags::unknown:
			res.Set(variant::unknown);
			break;

		default:
			assert(0 && "TODO");
			m_readError = 1;
			break;
		}

		return res;
	}

	collection packet::read_collection()
	{
		int32_t tag;
		collection collect = {};

		while ((tag = read1()) != tags::endCollection)
		{
			if (tag != tags::memberAttrName)
			{
				ELOG("unexpected tag %d\n", tag);
				m_readError = 1;
				return collection();
			}
			(void)read(read2());
			std::string name = read_value(tags::memberAttrName, "").GetString();
			auto values = read_collection_member(name);
			collect[name] = values;
		}
		(void)read(read2());
		(void)read(read2());

		return collect;
	}

	std::vector<variant> packet::read_collection_member(const std::string& name)
	{
		int32_t tag = read1();
		if (tag == tags::extension)
		{
			tag = read4();
		}
		(void)read(read2());

		return read_values(tag, name);
	}

	bool packet::are_groups_valid() const
	{
		if (m_groups_from_request.empty())
		{
			// If missing attributes, return error.
			return (!m_ipp_attributes.empty());
		}

		if (m_groups_from_request.size() != m_ipp_attributes.size())
		{
			return false;
		}

		// Groups have to be requested in order (ignore multimap ordering)
		int32_t group = m_groups_from_request.at(0);
		for (size_t i = 1; i < m_groups_from_request.size(); i++)
		{
			// Also avoid group duplication
			if (group >= m_groups_from_request.at(i))
			{
				return false;
			}
			group = m_groups_from_request.at(i);
		}

		//Unknwon groups have to be at the end of the list
		bool bUnknown = false;
		for (size_t i = 0; i < m_groups_from_request.size(); i++)
		{
			switch (m_groups_from_request.at(i))
			{
			case tags::begin_attribute_group_tag:
			case tags::operation_attributes_tag:
			case tags::job_attributes_tag:
			case tags::end_of_attributes_tag:
			case tags::printer_attributes_tag:
			case tags::unsupported_attributes_tag:
				if (bUnknown)
				{
					return false;
				}
				break;

			default:
				bUnknown = true;
				break;
			}
		}

		return true;
	}

	//builder
	bool packet::build()
	{
		std::vector<uint8_t> additional_data(std::move(m_ipp_data));

		m_ipp_data.clear();
		m_ipp_data.reserve(1024);

		write1(m_version_major);
		write1(m_version_minor);
		write2(m_operation_or_status_code);
		write4(m_request_id);

		write_attribute_groups();

		m_ipp_data.insert(m_ipp_data.end(), additional_data.begin(), additional_data.end());

		return true;
	}

	void packet::append(size_t n)
	{
		m_ipp_data.reserve(m_ipp_data.size() + n);
	}

	void packet::write1(int32_t value)
	{
		m_ipp_data.push_back(value & 0xFF);
	}

	void packet::write2(int32_t value)
	{
		m_ipp_data.push_back((value >> 8) & 0xFF);
		m_ipp_data.push_back((value >> 0) & 0xFF);
	}

	void packet::write4(int32_t value)
	{
		write2(value >> 16);
		write2(value & 0xFFFF);
	}

	void packet::write(const char* str, const int32_t length, bool writelenght, const char* encoding)
	{
		if (writelenght)
		{
			write2(length);
		}
		m_ipp_data.insert(m_ipp_data.end(), str, str + length);
	}

	void packet::write(const std::string& str, bool writelenght, const char* encoding)
	{
		write(str.c_str(), static_cast<int32_t>(str.size()), writelenght, encoding);
	}

	void packet::write_attribute_groups()
	{
		for (auto it = m_ipp_attributes.begin(); it != m_ipp_attributes.end(); it++)
		{
			write_attribute_group(it);
		}

		write1(tags::end_of_attributes_tag);
	}

	int32_t packet::todo_tag(const std::string& key, const variant& value)
	{
		switch (value.GetType())
		{
		case variant::Unsupported: return tags::unsupported;
		case variant::Unknown: return tags::unknown;
		case variant::NoValue: return tags::no_value;
		default: break;
		}

		std::vector<int32_t> tags = find_attribute_tag(key.c_str());
		if (tags.size() == 1)
		{
			return tags.at(0);
		}
		else //if (tags.size() > 1)
		{
			struct VariantToTag
			{
				variant::Type varType;
				int32_t tag;
			};
			static VariantToTag s_varMap[] = {
				{ variant::Int32, tags::integer },
				{ variant::Int32, tags::enum_tag },
				{ variant::Bool, tags::boolean },
				{ variant::Int32Range, tags::rangeOfInteger },
				{ variant::Resolutions, tags::resolution },
				{ variant::DateTime, tags::dateTime},
				{ variant::Collection, tags::begCollection },
				{ variant::Unsupported, tags::unsupported },
				{ variant::Unknown, tags::unknown },
				{ variant::NoValue, tags::no_value },
				{ variant::StringWithLanguage, tags::nameWithLanguage },
				{ variant::StringWithLanguage, tags::textWithLanguage },
				{ variant::String, tags::keyword },
				{ variant::String, tags::nameWithoutLanguage },
				{ variant::String, tags::textWithoutLanguage },
				{ variant::String, tags::uri },
				{ variant::String, tags::uriScheme },
				{ variant::String, tags::charset },
				{ variant::String, tags::naturalLanguage },
				{ variant::String, tags::mimeMediaType },
			};

			if (tags.size() > 1)
			{
				for (int i = 0; i < _countof(s_varMap); i++)
				{
					if ((value.GetType() == s_varMap[i].varType) && (std::find(tags.begin(), tags.end(), s_varMap[i].tag) != tags.end()))
					{
						return s_varMap[i].tag;
					}
				}
			}
			else
			{
				//Unregistered attributes shouldn't exist, unless this code is compiled in IPP_1 mode and the client is 
				//querying for IPP_2 printer attributes: it may happen on first client-printer connection (search phase).
				//So, choose the first valid tag according to variant type.
				//TODO: always register all IPP_2 attributes or set IPP_2 attributes as unsupported.
#if !IPP_SUPPORTS_2_0
				for (int i = 0; i < _countof(s_varMap); i++)
				{
					if (value.GetType() == s_varMap[i].varType)
					{
						return s_varMap[i].tag;
					}
				}
#endif
			}
		}

		ELOG("TODO: unknown tag %s", key.c_str());
		assert(0);
		return 0;
	}

	bool todo_is_extension(const int32_t tag)
	{
		return (tag > 255);//?
	}

	void packet::write_attribute_group(group_it group)
	{
		if (group != m_ipp_attributes.end())
		{
			write1(group->first);

			for (auto it2 = group->second.begin(); it2 != group->second.end(); it2++)
			{
				if (it2->second.size())
				{
					write_values(it2->first, it2->second);
				}
			}
		}
	}

	void packet::write_new_array_value(int32_t tag, const std::string& name, const variant& value)
	{
		if (todo_is_extension(tag))
		{
			write1(tags::extension);
			write4(tag);
		}
		else
		{
			write1(tag);
		}
		write2(0);
		write_variant(tag, value);
	}

	void packet::write_values(const std::string& name, const std::vector<variant>& values)
	{
		assert(values.size());
		int32_t tag = todo_tag(name, values[0]);
		write_first_value(tag, name, values[0]);
		for (size_t i = 1; i < values.size(); i++)
		{
			write_new_array_value(tag, name, values[i]);
		}
	}

	void packet::write_first_value(int32_t tag, const std::string& name, const variant& value)
	{
		if (todo_is_extension(tag))
		{
			write1(tags::extension);
			write4(tag);
		}
		else
		{
			write1(tag);
		}
		write(name, true);
		write_variant(tag, value);
	}

	void packet::write_variant(int32_t tag, const variant& value)
	{
		write2(value.GetSize());

		switch (value.GetType())
		{
		case variant::Bool:
			write1(value.GetBool() ? 1 : 0);
			break;

		case variant::Int32:
			write4(value.GetInt32());
			break;

		case variant::Int32Range:
			write4(std::get<0>(value.GetRange()));
			write4(std::get<1>(value.GetRange()));
			break;

		case variant::Resolutions:
			write4(std::get<0>(value.GetResolution()));
			write4(std::get<1>(value.GetResolution()));
			write1(std::get<2>(value.GetResolution()));
			break;

		case variant::DateTime:
		{
			const date_time& dt = value.GetDate();
			write2(dt.year);
			write1(dt.month);
			write1(dt.day);
			write1(dt.hour);
			write1(dt.minute);
			write1(dt.second);
			write1(dt.millisecond / 100);
			write1(!dt.isPositive ? '-' : '+');
			write1(dt.TZHour);
			write1(dt.TZMinute);
		}
		break;

		case variant::String:
		{
			const std::string& s = value.GetString();
			switch (tag)
			{
			case tags::nameWithoutLanguage:
			case tags::textWithoutLanguage:
			case tags::octetString:
			case tags::memberAttrName:
				write(s.c_str(), static_cast<int32_t>(s.size()), false);
				break;

			case tags::keyword:
			case tags::uri:
			case tags::uriScheme:
			case tags::charset:
			case tags::naturalLanguage:
			case tags::mimeMediaType:
				write(s.c_str(), static_cast<int32_t>(s.size()), false, "ascii");
				break;

			default:
				assert(0);
			}
		}
		break;

		case variant::StringWithLanguage:
		{
			const string_with_language& s = value.GetStringWithLanguage();
			switch (tag)
			{
			case tags::textWithLanguage:
			case tags::nameWithLanguage:
			{
				write(s.language().c_str(), static_cast<int32_t>(s.language().size()), true);
				write(s.text().c_str(), static_cast<int32_t>(s.text().size()), true);
			}
			break;

			default:
				assert(0);
			}
		}
		break;

		case variant::Collection:
		{
			const auto& collect = value.GetCollection();
			for (auto it = collect.cbegin(); it != collect.cend(); it++)
			{
				if (it->second.size())
				{
					write1(tags::memberAttrName);
					write2(0);	//name-length
					write(it->first, true);
					int32_t tag2 = todo_tag(it->first, it->second.at(0));
					//All values are saved as additional values, ignoring the passed name.
					for (size_t i = 0; i < it->second.size(); i++)
					{
						write_new_array_value(tag2, it->first, it->second.at(i));
					}
				}
			}
			write1(tags::endCollection);
			write2(0);	//end-name-length
			write2(0);	//end-value-length
		}
		break;

		case variant::Unsupported:
		case variant::Unknown:
		case variant::NoValue:
			//case variant::Default:
			//case variant::NotSettable:
			//case variant::DeleteAttr:
			//case variant::AdminDefine:
			break;

		default:
			assert(0 && "TODO");
			//write2(new_type.size()); //Already done above
			//write(new_type.data(), new_type.size());
			break;
		}
	}

	response::response()
	{
		SetVersion(1, 1);
	}

	request::request()
	{
		SetVersion(1, 1);
	}

	template<typename T>
	void add_operation_attributes(const T& i_operation_attribs, response& o_response)
	{
		auto& opAttribs = i_operation_attribs;
		auto it = o_response.AddGroup(tags::operation_attributes_tag);
		o_response.SetAttribute(it, "attributes-charset", opAttribs.attributes_charset);
		o_response.SetAttribute(it, "attributes-natural-language", opAttribs.attributes_natural_language);
		if (opAttribs.status_message.size())
		{
			o_response.SetAttribute(it, "status-message", opAttribs.status_message);
		}
		if (opAttribs.detailed_status_message.size())
		{
			o_response.SetAttribute(it, "detailed-status-message", opAttribs.detailed_status_message);
		}
	}

	template<typename T>
	void add_unsupported_attributes(const int32_t i_status_code, const T& i_operation_attribs, response& o_response)
	{
		switch (i_status_code)
		{
		case status_codes::successful_ok_conflicting_attributes:
		case status_codes::successful_ok_ignored_or_substituted_attributes:
		case status_codes::client_error_attributes_or_values_not_supported:
		case status_codes::client_error_conflicting_attributes:
		{
			auto& unsupportedAttribs = i_operation_attribs.attribs;
			if (unsupportedAttribs.size())
			{
				auto it = o_response.AddGroup(tags::unsupported_attributes_tag);
				for (auto it2 = unsupportedAttribs.cbegin(); it2 != unsupportedAttribs.cend(); it2++)
				{
					o_response.SetAttribute(it, it2->first, it2->second);
				}
			}
		}
		break;

		case status_codes::server_error_version_not_supported:
		case status_codes::server_error_operation_not_supported:
		case status_codes::client_error_charset_not_supported:
		case status_codes::client_error_compression_not_supported:
		case status_codes::client_error_document_format_not_supported:
		case status_codes::client_error_uri_scheme_not_supported:
		case status_codes::client_error_document_access_error:
			break;

		default:
			break;
		}
	}

	template<> void request::build_response(const int32_t i_status_code, const ErrorResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const PrintJobResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		{
			auto& jobAttribs = i_responseDesc.jobAttribs;
			auto it = o_response.AddGroup(tags::job_attributes_tag);
			o_response.SetAttribute(it, "job-id", jobAttribs.job_id);
			o_response.SetAttribute(it, "job-uri", jobAttribs.job_uri);
			o_response.SetAttribute(it, "job-state", jobAttribs.job_state);
			o_response.SetAttribute(it, "job-state-reasons", jobAttribs.job_state_reasons);
			if (jobAttribs.job_state_message.size())
			{
				o_response.SetAttribute(it, "job-state-message", jobAttribs.job_state_message);
			}
			if (jobAttribs.number_of_intervening_jobs.has_value())
			{
				o_response.SetAttribute(it, "number-of-intervening-jobs", jobAttribs.number_of_intervening_jobs.value());
			}
		}

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const ValidateJobResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const GetPrinterAttributesResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		{
			auto& printerAttribs = i_responseDesc.printerAttributes.attribs;
			if (printerAttribs.size())
			{
				auto it = o_response.AddGroup(tags::printer_attributes_tag);
				for (auto it2 = printerAttribs.cbegin(); it2 != printerAttribs.cend(); it2++)
				{
					o_response.SetAttribute(it, it2->first, it2->second);
				}
			}
		}

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const GetJobsResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		for (auto itJ = i_responseDesc.jobsAttributes.cbegin(); itJ != i_responseDesc.jobsAttributes.cend(); itJ++)
		{
			auto& jobAttribs = *itJ;
			auto it = o_response.AddGroup(tags::job_attributes_tag);
			o_response.SetAttribute(it, "job-id", jobAttribs.job_id);
			o_response.SetAttribute(it, "job-uri", jobAttribs.job_uri);
			o_response.SetAttribute(it, "job-state", jobAttribs.job_state);
			o_response.SetAttribute(it, "job-state-reasons", jobAttribs.job_state_reasons);
			if (jobAttribs.job_state_message.size())
			{
				o_response.SetAttribute(it, "job-state-message", jobAttribs.job_state_message);
			}
			o_response.SetAttribute(it, "number-of-intervening-jobs", jobAttribs.number_of_intervening_jobs);
		}

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const CancelJobResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		build_response(i_status_code, o_response);
	}

	template<> void request::build_response(const int32_t i_status_code, const GetJobAttributesResponse& i_responseDesc, response& o_response) const
	{
		o_response = {};

		add_operation_attributes(i_responseDesc.opAttribs, o_response);
		add_unsupported_attributes(i_status_code, i_responseDesc.unsupportedAttribs, o_response);

		{
			auto& jobAttribs = i_responseDesc.jobAttributes;
			auto it = o_response.AddGroup(tags::job_attributes_tag);
			o_response.SetAttribute(it, "job-id", jobAttribs.job_id);
			o_response.SetAttribute(it, "job-uri", jobAttribs.job_uri);
			o_response.SetAttribute(it, "job-state", jobAttribs.job_state);
			o_response.SetAttribute(it, "job-state-reasons", jobAttribs.job_state_reasons);
			if (jobAttribs.job_state_message.size())
			{
				o_response.SetAttribute(it, "job-state-message", jobAttribs.job_state_message);
			}
			o_response.SetAttribute(it, "number-of-intervening-jobs", jobAttribs.number_of_intervening_jobs);
		}

		build_response(i_status_code, o_response);
	}

	void request::build_response(const int32_t i_status_code, response& o_response) const
	{
		DBGLOG("id %d - status code %d (%s)", GetRequestId(), i_status_code, status_codes::get_description(i_status_code));

		o_response.SetVersion(GetIppMajor(), GetIppMinor());
		if (i_status_code == status_codes::server_error_version_not_supported)
		{
			// Set the nearest supported version
			if (GetIppMajor() < 1)
			{
				o_response.SetVersion(1, 0);
			}
			else
			{
				o_response.SetVersion(1, 1);
			}
		}
		o_response.SetRequestId(GetRequestId());
		o_response.SetStatusCode(i_status_code);
		//o_response.SetData();
		o_response.build();

#ifdef _DEBUG
		const std::vector<uint8_t>& body_data = o_response.GetData();
		testFromBuffer(body_data);
#endif
	}

	bool get_valid_string(const variant& i_variant, std::string& o_value)
	{
		if (i_variant.GetType() == variant::String)
		{
			o_value = i_variant.GetString();
			return true;
		}
		else if (i_variant.GetType() == variant::StringWithLanguage)
		{
			o_value = i_variant.GetStringWithLanguage().text();
			return true;
		}

		return false;
	}

	bool get_valid_string(const collection& collect, const char* i_attrib, std::string& o_value)
	{
		auto itAttr = collect.find(i_attrib);
		if ((itAttr != collect.end()) && (itAttr->second.size() == 1))
		{
			return get_valid_string(itAttr->second.at(0), o_value);
		}

		return false;
	}

	bool get_valid_boolean(const collection& collect, const char* i_attrib, bool& o_value)
	{
		auto itAttr = collect.find(i_attrib);
		if ((itAttr != collect.end()) && (itAttr->second.size() == 1) && (itAttr->second.at(0).GetType() == variant::Bool))
		{
			o_value = itAttr->second.at(0).GetBool();
			return true;
		}

		return false;
	}

	bool get_valid_integer(const collection& collect, const char* i_attrib, int32_t& o_value)
	{
		auto itAttr = collect.find(i_attrib);
		if ((itAttr != collect.end()) && (itAttr->second.size() == 1) && (itAttr->second.at(0).GetType() == variant::Int32))
		{
			o_value = itAttr->second.at(0).GetInt32();
			return true;
		}

		return false;
	}

	int32_t validate_operation_attributes(const collection& collect)
	{
		for (auto it = collect.cbegin(); it != collect.cend(); it++)
		{

		}
		return ipp::status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(PrintJobRequest& o_req) const
	{
		PrintJobRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				int32_t status_code = validate_operation_attributes(itOp->second);
				if (status_code != ipp::status_codes::successful_ok)
				{
					return status_code;
				}

				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.empty())
					{
						return ipp::status_codes::client_error_bad_request;
					}
					else if (req.opAttribs.attributes_charset.size() > CHARSET_MAX)
					{
						return ipp::status_codes::client_error_request_value_too_long;
					}
					else if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else if (req.opAttribs.attributes_natural_language.empty())
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else if (req.opAttribs.attributes_natural_language.size() > NATURAL_LANGUAGE_MAX)
				{
					return ipp::status_codes::client_error_request_value_too_long;
				}
				else
				{
					/*
					if (!is_natural_language_supported(req.opAttribs.attributes_natural_language))
					{
						req.opAttribs.attributes_natural_language = get_natural_language_configured();
					}
					*/
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}
				else if (req.opAttribs.requesting_user_name.size() > NAME_MAX)
				{
					return ipp::status_codes::client_error_request_value_too_long;
				}
				if (!get_valid_string(itOp->second, "job-name", req.opAttribs.job_name))
				{
					req.opAttribs.job_name.assign("<AutoGeneratedJobName>");	//TODO: use document-name or document-uri
				}
				else if (req.opAttribs.job_name.size() > NAME_MAX)
				{
					return ipp::status_codes::client_error_request_value_too_long;
				}
				if (!get_valid_string(itOp->second, "document-name", req.opAttribs.document_name))
				{
				}
				else if (req.opAttribs.document_name.size() > NAME_MAX)
				{
					return ipp::status_codes::client_error_request_value_too_long;
				}
				if (!get_valid_string(itOp->second, "compression", req.opAttribs.compression))
				{
					req.opAttribs.compression.assign("none");
				}
				else
				{
					//if (!is_supported(req.opAttribs.compression))
					//{
					//	return client_error_compression_not_supported;
					//}
				}
				if (!get_valid_string(itOp->second, "document-format", req.opAttribs.document_format))
				{
					//req.opAttribs.document_format = get_document_format_default();
				}
				else
				{
					//if (!is_supported(req.opAttribs.document_format))
					//{
					//	return client_error_document_format_not_supported;
					//}
					//else if (req.opAttribs.document_format == "application/octet-stream") && (!is_supported(detect_format(GetData()))))
					//{ 
					//	return client_error_document_format_not_supported;
					//}
				}
				if (!get_valid_string(itOp->second, "document-natural-language", req.opAttribs.document_natural_language))
				{
				}
				if (!get_valid_boolean(itOp->second, "ipp-attribute-fidelity", req.opAttribs.ipp_attribute_fidelity))
				{
					req.opAttribs.ipp_attribute_fidelity = false;
				}

				int32_t tmpInt = 0;
				if (!get_valid_integer(itOp->second, "job-k-octets", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_k_octets = tmpInt;
				}
				if (!get_valid_integer(itOp->second, "job-impressions", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_impressions = tmpInt;
				}
				if (!get_valid_integer(itOp->second, "job-media-sheets", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_media_sheets = tmpInt;
				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}
		{
			auto itOp = attribs.find(ipp::tags::job_attributes_tag);
			if (itOp != attribs.end())
			{
				//TODO
				req.jobtAttribs.attribs = itOp->second;
			}
			else
			{
				// Not mandatory
			}
		}
		req.docData = GetData();

		o_req = req;
		return status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(ValidateJobRequest& o_req) const
	{
		ValidateJobRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}
				if (!get_valid_string(itOp->second, "job-name", req.opAttribs.job_name))
				{
					req.opAttribs.job_name.assign("<AutoGeneratedJobName>");
				}
				if (!get_valid_string(itOp->second, "document-name", req.opAttribs.document_name))
				{
				}
				if (!get_valid_string(itOp->second, "compression", req.opAttribs.compression))
				{
					req.opAttribs.compression.assign("none");
				}
				else
				{
					//if (!is_supported(req.opAttribs.compression))
					//{
					//	return client_error_compression_not_supported;
					//}
				}
				if (!get_valid_string(itOp->second, "document-format", req.opAttribs.document_format))
				{
					//req.opAttribs.document_format = get_document_format_default();
				}
				else
				{
					//if (!is_supported(req.opAttribs.document_format))
					//{
					//	return client_error_document_format_not_supported;
					//}
					//else if (req.opAttribs.document_format == "application/octet-stream")) //&& (!is_supported(detect_format(req.GetData()))))
					//{ 
					//	return client_error_document_format_not_supported;
					//}
				}
				if (!get_valid_string(itOp->second, "document-natural-language", req.opAttribs.document_natural_language))
				{
				}
				if (!get_valid_boolean(itOp->second, "ipp-attribute-fidelity", req.opAttribs.ipp_attribute_fidelity))
				{
					req.opAttribs.ipp_attribute_fidelity = false;
				}

				int32_t tmpInt = 0;
				if (!get_valid_integer(itOp->second, "job-k-octets", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_k_octets = tmpInt;
				}
				if (!get_valid_integer(itOp->second, "job-impressions", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_impressions = tmpInt;
				}
				if (!get_valid_integer(itOp->second, "job-media-sheets", tmpInt))
				{
					//Do nothing if not supported
				}
				else
				{
					req.opAttribs.job_media_sheets = tmpInt;
				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}
		{
			auto itOp = attribs.find(ipp::tags::job_attributes_tag);
			if (itOp != attribs.end())
			{
				//TODO
				req.jobtAttribs.attribs = itOp->second;
			}
			else
			{
				// Not mandatory
			}
		}
		o_req = req;
		return status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(GetPrinterAttributesRequest& o_req) const
	{
		GetPrinterAttributesRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}
				if (!get_valid_string(itOp->second, "document-format", req.opAttribs.document_format))
				{
					//req.opAttribs.document_format = get_document_format_default();
				}
				else
				{
					//if (!is_supported(req.opAttribs.document_format))
					//{
					//	return client_error_document_format_not_supported;
					//}
					//else if (req.opAttribs.document_format == "application/octet-stream") && (!is_supported(detect_format(req.GetData()))))
					//{ 
					//	return client_error_document_format_not_supported;
					//}
				}

				auto itAttr = itOp->second.find("requested-attributes");
				if (itAttr != itOp->second.end())
				{
					std::string requested_attribute;
					const auto& p = itAttr->second;
					for (auto it2 = p.cbegin(); it2 != p.cend(); it2++)
					{
						auto& v = *it2;
						if (get_valid_string(v, requested_attribute) && requested_attribute.size())
						{
							req.opAttribs.requested_attributes.push_back(requested_attribute);
						}
						else
						{
						}
					}
				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}
		o_req = req;
		return status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(GetJobsRequest& o_req) const
	{
		GetJobsRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}
				if (!get_valid_string(itOp->second, "which-jobs", req.opAttribs.which_jobs))
				{
					req.opAttribs.which_jobs = "not-completed";
				}

				if (!get_valid_boolean(itOp->second, "my-jobs", req.opAttribs.my_jobs))
				{
					req.opAttribs.my_jobs = false;
				}

				int32_t tmpInt = 0;
				if (!get_valid_integer(itOp->second, "limit", tmpInt))
				{
					req.opAttribs.limit = INT_MAX;
				}
				else
				{
					req.opAttribs.limit = tmpInt;
				}

				auto itAttr = itOp->second.find("requested-attributes");
				if (itAttr != itOp->second.end())
				{
					std::string requested_attribute;
					const auto& p = itAttr->second;
					for (auto it2 = p.cbegin(); it2 != p.cend(); it2++)
					{
						auto& v = *it2;
						if (get_valid_string(v, requested_attribute) && requested_attribute.size())
						{
							req.opAttribs.requested_attributes.push_back(requested_attribute);
						}
						else
						{
						}
					}
				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}

		o_req = req;
		return status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(CancelJobRequest& o_req) const
	{
		CancelJobRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					if (!get_valid_string(itOp->second, "job-uri", req.opAttribs.job_uri))
					{
						return ipp::status_codes::client_error_bad_request;
					}
				}
				else
				{
					int32_t tmpInt = 0;
					if (!get_valid_integer(itOp->second, "job-id", tmpInt))
					{
						return ipp::status_codes::client_error_bad_request;
					}
					else
					{
						req.opAttribs.job_id = tmpInt;
					}
				}

				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}
				if (!get_valid_string(itOp->second, "message", req.opAttribs.message))
				{

				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}

		o_req = req;
		return status_codes::successful_ok;
	}

	template<> int32_t request::to_request_description(GetJobAttributesRequest& o_req) const
	{
		GetJobAttributesRequest req;

		if (!are_groups_valid())
		{
			return ipp::status_codes::client_error_bad_request;
		}

		auto& attribs = attributes();
		{
			auto itOp = attribs.find(ipp::tags::operation_attributes_tag);
			if (itOp != attribs.end())
			{
				if (!get_valid_string(itOp->second, "attributes-charset", req.opAttribs.attributes_charset))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				else
				{
					if (req.opAttribs.attributes_charset.compare("utf-8") != 0)
					{
						return ipp::status_codes::client_error_charset_not_supported;
					}
				}
				if (!get_valid_string(itOp->second, "attributes-natural-language", req.opAttribs.attributes_natural_language))
				{
					return ipp::status_codes::client_error_bad_request;
				}
				if (!get_valid_string(itOp->second, "printer-uri", req.opAttribs.printer_uri))
				{
					if (!get_valid_string(itOp->second, "job-uri", req.opAttribs.job_uri))
					{
						return ipp::status_codes::client_error_bad_request;
					}
				}
				else
				{
					int32_t tmpInt = 0;
					if (!get_valid_integer(itOp->second, "job-id", tmpInt))
					{
						return ipp::status_codes::client_error_bad_request;
					}
					else
					{
						req.opAttribs.job_id = tmpInt;
					}
				}

				if (!get_valid_string(itOp->second, "requesting-user-name", req.opAttribs.requesting_user_name))
				{
				}

				auto itAttr = itOp->second.find("requested-attributes");
				if (itAttr != itOp->second.end())
				{
					std::string requested_attribute;
					const auto& p = itAttr->second;
					for (auto it2 = p.cbegin(); it2 != p.cend(); it2++)
					{
						auto& v = *it2;
						if (get_valid_string(v, requested_attribute) && requested_attribute.size())
						{
							req.opAttribs.requested_attributes.push_back(requested_attribute);
						}
						else
						{
						}
					}
				}
			}
			else
			{
				return status_codes::client_error_bad_request;
			}
		}

		o_req = req;
		return status_codes::successful_ok;
	}
}
