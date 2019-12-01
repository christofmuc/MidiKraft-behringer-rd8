#include "RD8Plugin.h"

midikraft::BehringerRD8 * CreateObjectInstance()
{
	return new midikraft::BehringerRD8();
}

void ReleaseObject(midikraft::BehringerRD8 *rd8)
{
	delete rd8;
}
