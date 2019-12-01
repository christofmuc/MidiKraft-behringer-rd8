/*
   Copyright (c) 2019 Christof Ruch. All rights reserved.

   Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
*/

#include "RD8Plugin.h"

midikraft::BehringerRD8 * CreateObjectInstance()
{
	return new midikraft::BehringerRD8();
}

void ReleaseObject(midikraft::BehringerRD8 *rd8)
{
	delete rd8;
}
