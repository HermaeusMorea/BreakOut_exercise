#ifndef ASSET_PATHS_H
#define ASSET_PATHS_H

#include <filesystem>
#include <string>

namespace AssetPaths
{
void Initialize(const char* executablePath);
std::filesystem::path Resolve(const std::string& relativePath);
}

#endif
