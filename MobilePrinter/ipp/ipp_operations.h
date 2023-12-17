#pragma once

#include "ipp_types.h"

namespace ipp
{
	class ErrorResponse
	{
	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
	};

	class PrintJobRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			name<NAME_MAX> requesting_user_name;
			name<NAME_MAX> job_name;
			bool ipp_attribute_fidelity = false;
			name<NAME_MAX> document_name;
			keyword compression;
			mimeMediaType document_format;
			naturalLanguage document_natural_language;
			integer_t<0, INT_MAX> job_k_octets = 0;
			integer_t<0, INT_MAX> job_impressions = 0;
			integer_t<1, INT_MAX> job_media_sheets = 1;
		};

		struct JobTemplateAttributes
		{
			std::map<std::string, std::vector<variant>> attribs;
		};

		OperationAttributes opAttribs;
		JobTemplateAttributes jobtAttribs;
		std::vector<std::uint8_t> docData;
	};

	class PrintJobResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		struct JobAttributes
		{
			integer_t<1, INT_MAX> job_id = 1;
			uri job_uri;
			enum_t job_state = 0;
			std::vector<keyword> job_state_reasons;
			text<TEXT_MAX> job_state_message;
			std::optional<integer_t<0, INT_MAX>> number_of_intervening_jobs;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
		JobAttributes jobAttribs;
	};

	//class PrintURIReques/Response: OPTIONAL

	class ValidateJobRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			name<NAME_MAX> requesting_user_name;
			name<NAME_MAX> job_name;
			bool ipp_attribute_fidelity = false;
			name<NAME_MAX> document_name;
			keyword compression;
			mimeMediaType document_format;
			naturalLanguage document_natural_language;
			integer_t<0, INT_MAX> job_k_octets = 0;
			integer_t<0, INT_MAX> job_impressions = 0;
			integer_t<1, INT_MAX> job_media_sheets = 1;
		};

		struct JobTemplateAttributes
		{
			std::map<std::string, std::vector<variant>> attribs;
		};

		OperationAttributes opAttribs;
		JobTemplateAttributes jobtAttribs;
	};

	class ValidateJobResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
	};

	//Create-Job RECOMMENDED

	class GetPrinterAttributesRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			name<NAME_MAX> requesting_user_name;
			std::vector<keyword> requested_attributes;
			mimeMediaType document_format;
		};

		OperationAttributes opAttribs;
	};

	class GetPrinterAttributesResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		struct PrinterAttributes
		{
			std::map<std::string, std::vector<variant>> attribs;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
		PrinterAttributes printerAttributes;
	};


	class GetJobsRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			name<NAME_MAX> requesting_user_name;
			integer_t<1, INT_MAX> limit = 1;
			std::vector<keyword> requested_attributes;
			keyword which_jobs;
			bool my_jobs = false;
		};

		OperationAttributes opAttribs;
	};

	class GetJobsResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		struct JobAttributes
		{
			integer_t<1, INT_MAX> job_id = 1;
			uri job_uri;
			enum_t job_state = 0;
			std::vector<keyword> job_state_reasons;
			text<TEXT_MAX> job_state_message;
			integer_t<0, INT_MAX> number_of_intervening_jobs = 0;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
		std::vector<JobAttributes> jobsAttributes;
	};

	//PausePrinter optional
	//ResumePrinter optional
	//PurgeJobs deprecated
	//SendDocument recommended
	//SendURI recommended

	class CancelJobRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			integer_t<1, INT_MAX> job_id = 1;
			uri job_uri;
			name<NAME_MAX> requesting_user_name;
			text<127> message;
		};

		OperationAttributes opAttribs;
	};

	class CancelJobResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
	};

	class GetJobAttributesRequest
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			uri printer_uri;
			integer_t<1, INT_MAX> job_id = 1;
			uri job_uri;
			name<NAME_MAX> requesting_user_name;
			std::vector<keyword> requested_attributes;
		};

		OperationAttributes opAttribs;
	};

	class GetJobAttributesResponse
	{

	public:

		struct OperationAttributes
		{
			std::string attributes_charset;
			std::string attributes_natural_language;
			text<255> status_message;
			text<TEXT_MAX> detailed_status_message;
		};

		struct UnsupportedAttributes
		{
			std::map<std::string, variant> attribs;
		};

		struct JobAttributes
		{
			integer_t<1, INT_MAX> job_id = 1;
			uri job_uri;
			enum_t job_state = 0;
			std::vector<keyword> job_state_reasons;
			text<TEXT_MAX> job_state_message;
			integer_t<0, INT_MAX> number_of_intervening_jobs = 0;
		};

		OperationAttributes opAttribs;
		UnsupportedAttributes unsupportedAttribs;
		JobAttributes jobAttributes;
	};


}
