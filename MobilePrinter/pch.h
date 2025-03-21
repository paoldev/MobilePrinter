// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

#ifndef PCH_H
#define PCH_H

// TODO: add headers that you want to pre-compile here
#define NOMINMAX
#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING	//TODO: VS2022 17.8.0 - remove this preprocessor macro after fixing checked_array_iterators deprecation warnings in cpprestsdk.
#include <assert.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wsdapi.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <ppltasks.h>
#include <span>

#include "Config.h"

struct IPrinterServiceType;
struct IPrinterServiceV12Type;
struct IPrinterServiceV20Type;

#endif //PCH_H
