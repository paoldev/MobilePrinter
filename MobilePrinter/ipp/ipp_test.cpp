#include "pch.h"
#include "ipp_packet.h"
#include "ipp_types.h"
#include "CommonUtils.h"

void testFromBuffer(const void* i_buffer, const size_t i_size)
{
	if (i_buffer && i_size)
	{
		ipp::packet pk;
		bool p = pk.parse(static_cast<const uint8_t*>(i_buffer), i_size);
		assert(p);

		bool b = pk.build();
		assert(b);

		assert(pk.GetData().size() == i_size);
		assert(memcmp(i_buffer, pk.GetData().data(), i_size) == 0);
	}
}

void testPrintJobRequest()
{
	ipp::request pk;
	pk.SetVersion(1, 1);
	pk.SetOperationId(ipp::operations::Print_Job);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "printer-uri", "ipp://printer.example.com/ipp/print/pinetree");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "job-name", "foobar");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "ipp-attribute-fidelity", true);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "copies", 20);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "sides", "two-sided-long-edge");
//	pk.SetData(pdf)

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testPrintJobResponse()
{
	ipp::response pk;
	pk.SetVersion(1, 1);
	pk.SetStatusCode(ipp::status_codes::successful_ok);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "status-message", "successful-ok");
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-id", 147);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-uri", "ipp://printer.example.com/ipp/print/pinetree/147");
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-state", 3/*pending*/);
	//	pk.SetData(pdf)

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testPrintJobResponseFailure()
{
	ipp::response pk;
	pk.SetVersion(1, 1);
	pk.SetStatusCode(ipp::status_codes::client_error_attributes_or_values_not_supported);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "status-message", "client-error-attributes-or-values-not-supported");
	pk.SetAttribute(ipp::tags::unsupported_attributes_tag, "copies", 20);
	pk.SetAttribute(ipp::tags::unsupported_attributes_tag, "sides", ipp::variant::unsupported);
	//	pk.SetData(pdf)

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testPrintJobResponseSuccessWithAttributesIgnored()
{
	ipp::response pk;
	pk.SetVersion(1, 1);
	pk.SetStatusCode(ipp::status_codes::successful_ok_ignored_or_substituted_attributes);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "status-message", "successful-ok-ignored-or-substituted-attributes");
	pk.SetAttribute(ipp::tags::unsupported_attributes_tag, "copies", 20);
	pk.SetAttribute(ipp::tags::unsupported_attributes_tag, "sides", ipp::variant::unsupported);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-id", 147);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-uri", "ipp://printer.example.com/ipp/print/pinetree/147");
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-state", 3/*pending*/);
	//	pk.SetData(pdf)

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testPrintURIRequest()
{
	ipp::request pk;
	pk.SetVersion(1, 1);
	pk.SetOperationId(ipp::operations::Print_URI);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "printer-uri", "ipp://printer.example.com/ipp/print/pinetree");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "document-uri", "ftp://foo.example.com/foo");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "job-name", "foobar");
	pk.SetAttribute(ipp::tags::job_attributes_tag, "copies", 1);

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testCreateJobRequest()
{
	ipp::packet pk;

	uint8_t data[] = {

		0x01, 0x01,
		0x00, 0x05,
		0x00, 0x00, 0x00, 0x01,
		0x01,
		0x47,
		0x00, 0x12,
		0x61, 0x74 , 0x74 , 0x72 , 0x69 , 0x62 , 0x75 , 0x74 , 0x65 , 0x73 , 0x2d , 0x63 , 0x68 , 0x61 , 0x72 , 0x73 , 0x65 , 0x74, //'attributes-charset',
		0x00, 0x05,
		0x75 , 0x74 , 0x66 , 0x2d , 0x38  ,//'utf-8',
		0x48,
		0x00, 0x1b,
		0x61 , 0x74 , 0x74 , 0x72 , 0x69 , 0x62 , 0x75 , 0x74 , 0x65 , 0x73 , 0x2d , 0x6e , 0x61 , 0x74 , 0x75 , 0x72 , 0x61 , 0x6c , 0x2d , 0x6c , 0x61 , 0x6e , 0x67 , 0x75 , 0x61 , 0x67 , 0x65  ,//'attributes-natural-language',
		0x00, 0x05,
		0x65 , 0x6e , 0x2d , 0x75 , 0x73  ,//'en-us',
		0x45,
		0x00, 0x0b,
		0x70 , 0x72 , 0x69 , 0x6e , 0x74 , 0x65 , 0x72 , 0x2d , 0x75 , 0x72 , 0x69  ,//  'printer-uri'
		0x00, 0x2c,
		0x69 , 0x70 , 0x70 , 0x3a , 0x2f , 0x2f , 0x70 , 0x72 , 0x69 , 0x6e , 0x74 , 0x65 , 0x72 , 0x2e , 0x65 , 0x78 , 0x61 , 0x6d , 0x70 , 0x6c , 0x65 , 0x2e , 0x63 , 0x6f , 0x6d , 0x2f , 0x69 , 0x70 , 0x70 , 0x2f , 0x70 , 0x72 , 0x69 , 0x6e , 0x74 , 0x2f , 0x70 , 0x69 , 0x6e , 0x65 , 0x74 , 0x72 , 0x65 , 0x65  ,//'ipp://printer.example.com/ipp/print/pinetree',
		0x03
	};

	testFromBuffer(data, sizeof(data));
}

void testCreateJobRequestWithCollectionAttributes()
{
	ipp::request pk;
	pk.SetVersion(1, 1);
	pk.SetOperationId(ipp::operations::Create_Job);
	pk.SetRequestId(1);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "printer-uri", "ipp://printer.example.com/ipp/print/pinetree");
	
	ipp::collection media_col;
	ipp::collection media_size;
	collection_insert(media_size, "x-dimension", 21000);
	collection_insert(media_size, "y-dimension", 29700);
	collection_insert(media_col, "media-size", media_size);
	collection_insert(media_col, "media-type", "stationery");
	
	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testGetJobsRequest()
{
	ipp::response pk;
	pk.SetVersion(1, 1);
	pk.SetStatusCode(ipp::operations::Get_Jobs);
	pk.SetRequestId(123);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "printer-uri", "ipp://printer.example.com/ipp/print/pinetree");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "limit", 50);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "requested-attributes", "job-id");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "requested-attributes", "job-name");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "requested-attributes", "document-format");

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testGetJobsResponse()
{
	ipp::response pk;
	pk.SetVersion(1, 1);
	pk.SetStatusCode(ipp::status_codes::successful_ok);
	pk.SetRequestId(123);
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-charset", "utf-8");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "attributes-natural-language", "en-us");
	pk.SetAttribute(ipp::tags::operation_attributes_tag, "status-message", "successful-ok");
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-id", 147);
	pk.SetAttribute(ipp::tags::job_attributes_tag, "job-name", "fou");
	auto it1 = pk.AddGroup(ipp::tags::job_attributes_tag);
	auto it2 = pk.AddGroup(ipp::tags::job_attributes_tag);
	pk.SetAttribute(it2, "job-id", 149);
	pk.SetAttribute(it2, "job-name", "isch guet");

	bool b = pk.build();
	assert(b);

	ipp::packet pk2;

	bool p = pk2.parse(pk.GetData().data(), pk.GetData().size());
	assert(p);

	assert(pk.attributes() == pk2.attributes());
}

void testFromBuffer(const std::vector<uint8_t>& buffer)
{
	testFromBuffer(buffer.data(), buffer.size());
}

void testFromFile()
{
	std::vector<uint8_t> buffer;
	std::filesystem::path FileName = global_config::get().get_output_folder();
	FileName.append(L"ipp_tmp");
	FileName.append(L"f15_write_1564926383_1756.dat");
	LoadFile(FileName.c_str(), buffer);

	testFromBuffer(buffer);
}

void callTests()
{
	testPrintJobRequest();
	testPrintJobResponse();
	testPrintJobResponseFailure();
	testPrintJobResponseSuccessWithAttributesIgnored();
	testPrintURIRequest();
	testCreateJobRequest();
	testCreateJobRequestWithCollectionAttributes();
	testGetJobsRequest();
	testGetJobsResponse();

	testFromFile();
}
