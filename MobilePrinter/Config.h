#pragma once

//wsprint
#define WSPRINT_SUPPORTS_1_1	0
#define WSPRINT_SUPPORTS_1_2	0
#define WSPRINT_SUPPORTS_2_0	1
#define WSPRINT_MULTI_SERVICES	0

//ipp
#define IPP_SUPPORTS_2_0	0
#define IPP_SUPPORTS_2_0_ATTRIBUTES 0

//cpprestsdk
//NOTE:
//
//CPPREST_FORCE_HTTP_LISTENER_ASIO moved as preprocessor definition in Debug/Release configurations
//
//if CPPREST_FORCE_HTTP_LISTENER_ASIO is not defined and cpprestsdk\http_server_httpsys.cpp is used, the application
//as to be executed as administrator (requireAdministrator).
//if CPPREST_FORCE_HTTP_LISTENER_ASIO is defined and cpprestsdk\http_server_asio.cpp + boost_emul.h are used, no 
//administrator privileges are required (asInvoker).
//See MobilePrinter Project
//		-> Properties 
//			-> Linker 
//				-> Manifest File 
//					-> UAC Execution Level 
//						-> requireAdministrator (instead of asInvoker default value).
#define _NO_ASYNCRTIMP
//#define CPPREST_FORCE_HTTP_LISTENER_ASIO
#define CPPREST_EXCLUDE_WEBSOCKETS
#define CPPREST_EXCLUDE_COMPRESSION

#if !defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
#define IPP_WINHTTP_SSL_SUPPORTED 1
#else
#define IPP_WINHTTP_SSL_SUPPORTED 0
#endif
