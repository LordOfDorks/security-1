// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Windoes stuff
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <Strsafe.h>
#include <Wincrypt.h>
#include <ntstatus.h>

// TSS stuff
#include <exception>
#include <numeric>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <initializer_list>
#include <cstdarg>
#include <typeinfo>
#include <chrono>
#include <system_error>
#include "Tpm2.h"
using namespace TpmCpp;

// DfuSe stuff
#pragma push_macro("EXPORT")
#define EXPORT
#include "usb100.h"
#include "STDFU.h"
#include "STDFUPRTINC.h"
#include "STDFUFILESINC.h"
#pragma pop_macro("EXPORT")

// Local headers
#include "Dice.h"
#include "IdStore.h"

// TODO: reference additional headers your program requires here
