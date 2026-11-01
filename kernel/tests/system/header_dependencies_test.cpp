#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>

namespace fs = std::filesystem;

TEST(KernelHeaders, SharedInterfacesDoNotDependOnApplications)
{
	const fs::path root = HOST_REPOSITORY;
	std::set<std::string> applicationHeaders;
	for(auto entry = fs::recursive_directory_iterator(root / "applications");
		entry != fs::recursive_directory_iterator(); ++entry)
	{
		if(!entry->is_directory()) continue;
		auto name = entry->path().filename();
		if(name == ".git" || name == "bin" || name == "obj")
		{
			entry.disable_recursion_pending();
			continue;
		}
		if(name != "inc" && name != "include") continue;
		auto includes = entry->path();
		for(const auto& header : fs::recursive_directory_iterator(includes))
		{
			if(header.is_regular_file())
				applicationHeaders.insert(header.path().lexically_relative(includes).generic_string());
		}
		entry.disable_recursion_pending();
	}

	const std::regex include(R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");
	for(const auto& directory : {root / "kernel/src", root / "kernel/tests", root / "libapi/inc"})
	{
		for(auto entry = fs::recursive_directory_iterator(directory); entry != fs::recursive_directory_iterator(); ++entry)
		{
			if(entry->is_directory() && entry->path().filename() == "bin")
			{
				entry.disable_recursion_pending();
				continue;
			}
			const auto& path = entry->path();
			if(!entry->is_regular_file() || (path.extension() != ".cpp" && path.extension() != ".hpp" &&
				path.extension() != ".c" && path.extension() != ".h" && path.extension() != ".inc")) continue;
			std::ifstream source(path);
			ASSERT_TRUE(source.is_open()) << path;
			std::string line;
			unsigned number = 0;
			while(std::getline(source, line))
			{
				++number;
				std::smatch match;
				if(!std::regex_search(line, match, include)) continue;
				auto header = match[1].str();
				auto resolved = (path.parent_path() / header).lexically_normal();
				auto relative = resolved.lexically_relative(root).generic_string();
				EXPECT_TRUE(!applicationHeaders.count(header) && relative.rfind("applications/", 0) != 0 &&
					header.rfind("applications/", 0) != 0)
					<< path << ':' << number << " includes application header " << header;
			}
		}
	}
}
