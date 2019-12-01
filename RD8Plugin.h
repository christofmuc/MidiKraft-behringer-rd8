/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#pragma once

#include "RD8.h"
#include "DynamicPlugin.h"

#if WIN32
#define DllExport   __declspec( dllexport )
#else
#define DllExport /* */
#endif

extern "C" {

	DllExport midikraft::BehringerRD8 * CreateObjectInstance();

	DllExport void  ReleaseObject(midikraft::BehringerRD8 *rd8);

}

template<> struct midikraft::PluginTraits<midikraft::BehringerRD8> {
	static const char *filename;
};
const char *midikraft::PluginTraits < midikraft::BehringerRD8 >::filename = "midikraft-behringer-rd8.dll";
typedef midikraft::DynamicPlugin<midikraft::BehringerRD8> RD8Plugin;
