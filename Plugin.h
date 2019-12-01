#pragma once

#include "RD8.h"

#if WIN32
#define DllExport   __declspec( dllexport )
#else
#define DllExport /* */
#endif

extern "C" {

	DllExport midikraft::BehringerRD8 * CreateObjectInstance();

	DllExport void  ReleaseObject(midikraft::BehringerRD8 *rd8);

}

