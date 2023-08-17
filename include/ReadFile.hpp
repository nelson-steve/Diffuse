#pragma once

#include <vector>
#include <string>

namespace Utils {
	class File {
	public:
		static std::vector<char> ReadFile(const std::string& filename);
	};
}