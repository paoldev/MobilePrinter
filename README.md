# MobilePrinter

[![License: MIT](https://img.shields.io/badge/License-MIT-red.svg)](LICENSE.txt)

**MobilePrinter** is an hobbystic project to print simple documents and images from Windows 10 Mobile phone and Android devices to an USB printer, which doesn't expose any mobile-compatible printer service on LAN.
  
It's a merge of a couple of programs I wrote in 2015-2020, so it's written with mixed coding styles.  
  
This application exposes three services:  
* **Wsprint**, used by Windows 10 Mobile print service.  
* **Ipp**, partially compatible with Mopria and Samsung Print Services for Android (I didn't test other Print Services).  
* **Dnssd**, to advertise the printer for ipp discovery on LAN.  
  
Do not use this project in any production environment, since all three services don't implement the complete set of interfaces required to be fully compliant with their standards.
This is just a proof-of-concept I wrote for my needs years ago and it may not work anymore with latest Windows 10 updates.

## Some notes

### Wsprint
This project implements minimal wsprint 1.1 interface, mainly based on documents and samples found here
* https://docs.microsoft.com/en-us/windows-hardware/drivers/print/ws-print-v1-1
* https://docs.microsoft.com/en-us/windows-hardware/drivers/3dprint/enabling-wsprint-on-a-device
* https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/web/wsdapi/fileservice

### Ipp
This project implements minimal ipp 1.1 interface, mainly based on documents and samples found here
* https://www.pwg.org/
* https://mopria.org/
* https://www.iana.org/assignments/ipp-registrations/ipp-registrations.xhtml
* https://github.com/istopwg/ippsample

Ipp support is based on cpprestsdk http_listener class (https://github.com/microsoft/cpprestsdk): it can be built against both WinHttp API (but it requires Administrator privileges to run) or against ASIO API based upon my custom minimal boost excerpt (see boost_emul.h and boost_emul.cpp files).

### Dnssd
Dnssd support is mainly based on sample found here
* https://github.com/microsoft/Windows-appsample-networkhelper

It also uses the new WinRT technology, which requires c++17 compiler: https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/

### Xps and Pdf print
Support for these file formats is mainly based on sample found here
* https://github.com/Microsoft/Windows-classic-samples/tree/master/Samples/D2DPrintingFromDesktopApps

## Usage
Before you can access your printer from a mobile device, you have to run `MobilePrinter.exe` from a PC where the printer is attached to; it also works from a PC that accesses a shared printer. 

From a command prompt (use Administrator command prompt if you compile the DebugWinHttp or ReleaseWinHttp configurations):

    MobilePrinter "printer name" [-options]

where options are
* -(no-)wsprint: enable/disable wsprint service
* -(no-)ipp: enable/disable ipp service
* -(no-)ignore-small-xps-elements: some xps can't be printed if they contain very small elements.
* -(no-)ignore-small-xps-abs-elements: some xps can't be printed if they contain very small elements.
* -log[debug/verbose/default]: enable different levels of log.

**NB**: -no-wsprint and -no-ipp can't be declared together (otherwise all printer services will be disabled).

**Default parameters if not specified:**  

    -wsprint -ipp -ignore-small-xps-elements -no-ignore-small-xps-abs-elements -logdefault

From your mobile device, you can reference the printer with the following name
* "\\MYCOMPUTER\MobilePrinter\My Home Printer Name"
* "\\MYCOMPUTER\MobilePrinter\My Printer shared from another PC"
	
### Examples

	1. MobilePrinter.exe
		List all available printers and options
		
	2. MobilePrinter.exe "My Home Printer Name"
		where "My Home Printer Name" is the name of your printer you read from command output in example 1 or in Windows 10 settings -> Devices -> Printers and Scanners
		
	3. MobilePrinter.exe "My Home Printer Name" -wsprint -no-ipp
		to disable ipp service
		
	4. MobilePrinter.exe "\\MYOTHERPC\MyPrinterSharedFromAnotherPC"
		to advertise and use a printer shared from another PC
		
		
### TODO

* Complete all required interfaces
* Create a Windows service
