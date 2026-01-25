#include "pch.h"
#include "Config.h"

#include <iostream>
#include <fstream>

ConfigLoader& GConfigLoader = ConfigLoader::Instance();

bool ConfigLoader::Load(const std::wstring& path)
{
	return false;
}
