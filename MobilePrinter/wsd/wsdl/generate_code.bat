@echo off

set CFG_PROJECTNAME=MyPrinter
set CFG_CODEGEN_NAME=codegen_host
set CFG_OUTPUT_ROOT=../src_host

set CFG_MANUFACTURER=MobilePrinter
set CFG_MANUFACTURERURL=
set CFG_MODELNAME=MobilePrinter
set CFG_MODELNUMBER=
set CFG_MODELURL=
set CFG_PRESENTATIONURL=
set CFG_MULTIPLE_TYPES=http://schemas.microsoft.com/windows/2006/08/wdp/print/:PrinterServiceType http://schemas.microsoft.com/windows/2012/10/wdp/printV12/:PrinterServiceV12Type http://schemas.microsoft.com/windows/2014/04/wdp/printV20/:PrinterServiceV20Type
set CFG_PNPXDEVICECATEGORY=Printers MobilePrinter
set CFG_PNPXHARDWAREID_PRINTERSERVICE=PnPX_MobilePrinter_HWID
set CFG_PNPXHARDWAREID_PRINTERSERVICE12=
set CFG_PNPXHARDWAREID_PRINTERSERVICE20=
set CFG_PNPXCOMPATIBLEID_PRINTERSERVICE=http://schemas.microsoft.com/windows/2006/08/wdp/print/PrinterServiceType
set CFG_PNPXCOMPATIBLEID_PRINTERSERVICE12=
set CFG_PNPXCOMPATIBLEID_PRINTERSERVICE20=

echo.
echo Step 1: create %CFG_CODEGEN_NAME%.config
"C:\Program Files (x86)\Windows Kits\10\bin\x64\wsdcodegen.exe" /generateconfig:host /pnpx /projectname:%CFG_PROJECTNAME% /outputfile:%CFG_CODEGEN_NAME%.config WSDPrintDevice.wsdl WSDPrinterService.wsdl WSDPrinterServiceV11.wsdl WSDPrinterServiceV12.wsdl WSDPrinterServiceV20.wsdl WSDIppEverywhere.wsdl

echo.
echo Step 2: update %CFG_CODEGEN_NAME%.config with ThisModelMetadata and other stuff; convert file from multiservice-singleport to singleservice-multiport type
powershell "$TheXml=[xml](Get-Content '%CFG_CODEGEN_NAME%.config'); $TheXml.selectsinglenode('//wsdcodegen/ThisModelMetadata').innerxml='<Manufacturer>%CFG_MANUFACTURER%</Manufacturer><ManufacturerURL>%CFG_MANUFACTURERURL%</ManufacturerURL><ModelName>%CFG_MODELNAME%</ModelName><ModelNumber>%CFG_MODELNUMBER%</ModelNumber><ModelURL>%CFG_MODELURL%</ModelURL><PresentationURL>%CFG_PRESENTATIONURL%</PresentationURL><PnPXDeviceCategory>%CFG_PNPXDEVICECATEGORY%</PnPXDeviceCategory>'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/Types')[0].innertext='%CFG_MULTIPLE_TYPES%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXHardwareId')[0].innertext='%CFG_PNPXHARDWAREID_PRINTERSERVICE%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXHardwareId')[1].innertext='%CFG_PNPXHARDWAREID_PRINTERSERVICE12%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXHardwareId')[2].innertext='%CFG_PNPXHARDWAREID_PRINTERSERVICE20%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXCompatibleId')[0].innertext='%CFG_PNPXCOMPATIBLEID_PRINTERSERVICE%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXCompatibleId')[1].innertext='%CFG_PNPXCOMPATIBLEID_PRINTERSERVICE12%'; $TheXml.selectnodes('//wsdcodegen/RelationshipMetadata/HostMetadata/Hosted/PnPXCompatibleId')[2].innertext='%CFG_PNPXCOMPATIBLEID_PRINTERSERVICE20%'; $TheXml.selectsinglenode('//wsdcodegen/RelationshipMetadata/HostMetadata').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/RelationshipMetadata/HostMetadata').LastChild);  $TheXml.selectsinglenode('//wsdcodegen/RelationshipMetadata/HostMetadata').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/RelationshipMetadata/HostMetadata').LastChild); $TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderDeclaration').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderDeclaration').LastChild); $TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderDeclaration').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderDeclaration').LastChild); $TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderImplementation').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderImplementation').LastChild); $TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderImplementation').RemoveChild($TheXml.selectsinglenode('//wsdcodegen/File/HostBuilderImplementation').LastChild); $TheXml.selectnodes('//wsdcodegen/File/LiteralInclude[text()=''stdafx.h'']') | Foreach-Object { $_.innertext='pch.h'}; $TheXml.Save('%CFG_CODEGEN_NAME%.config')"

echo.
echo Step 3: Fix uninitialized variables in %CFG_CODEGEN_NAME%.config
powershell "(Get-Content '%CFG_CODEGEN_NAME%.config') | Foreach-Object { $_.replace('m_cRef(1), m_host(NULL)', 'm_cRef(1), m_host(NULL), m_serviceId(NULL)') } | Set-Content '%CFG_CODEGEN_NAME%.config'"

echo.
echo Step 4: export source code to %CFG_OUTPUT_ROOT%
"C:\Program Files (x86)\Windows Kits\10\bin\x64\wsdcodegen.exe" /generatecode /gbc /outputroot:%CFG_OUTPUT_ROOT% /writeaccess:"attrib -r" /w0 %CFG_CODEGEN_NAME%.config

echo.
echo Step 5: Fix mispelled enums in %CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Types.cpp (WSDXML_NODE.ElementType and WSDXML_NODE.TextType to WSDXML_NODE::ElementType and WSDXML_NODE::TextType)
powershell "(Get-Content '%CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Types.cpp') | Foreach-Object { $_.replace('WSDXML_NODE.', 'WSDXML_NODE::') } | Set-Content '%CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Types.cpp'"

echo.
echo Step 6: Add multiport registration to %CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Stub.cpp
powershell "(Get-Content '%CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Stub.cpp') | Foreach-Object { $_.replace('hr = pHost->RegisterPortType(&PortType_PrinterServiceType);', 'hr = pHost->RegisterPortType(&PortType_PrinterServiceType);    }    if( S_OK == hr )    {        hr = pHost->RegisterPortType(&PortType_PrinterServiceV12Type);    }    if( S_OK == hr )    {        hr = pHost->RegisterPortType(&PortType_PrinterServiceV20Type);') } | Set-Content '%CFG_OUTPUT_ROOT%/%CFG_PROJECTNAME%Stub.cpp'"

echo.
echo Done
