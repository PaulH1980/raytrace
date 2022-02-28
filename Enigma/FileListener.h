#pragma once
#include "FrontEndDef.h"

namespace RayTrace
{
	class FileListener
	{
		FileListener(const std::string& _rootDir);


		std::vector<std::string> Check() const;


		std::map<std::string, uint64_t> m_files;
		std::string m_rootFolder;
	};
}