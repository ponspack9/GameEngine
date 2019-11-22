#include "Resource.h"
#include "Config.h"
#include "Application.h"
#include "ModuleFileSystem.h"

Resource::Resource(UID Uid, Resource::Type Type) : uid(Uid), type(Type)
{
}

Resource::~Resource()
{
}

bool Resource::LoadToMemory()
{
	if (times_loaded > 0)
		times_loaded++;
	else
		times_loaded = LoadtoScene() ? 1 : 0;

	return times_loaded > 0;
}

void Resource::ReleaseFromMemory()
{
	if (--times_loaded == 0)
		UnLoad();
}
