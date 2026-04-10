#include "breakout/assets/asset_paths.h"

#include <stdexcept>
#include <vector>

namespace AssetPaths
{
namespace
{
std::filesystem::path g_assetRoot;

std::filesystem::path CurrentExecutableDirectory(const char* executablePath)
{
    if (executablePath == nullptr || std::string(executablePath).empty())
        return std::filesystem::current_path();

    std::filesystem::path executable = std::filesystem::absolute(executablePath);
    if (executable.has_parent_path())
        return executable.parent_path();
    return std::filesystem::current_path();
}

bool HasExpectedAssetLayout(const std::filesystem::path& candidate)
{
    return std::filesystem::exists(candidate / "shaders") &&
        std::filesystem::exists(candidate / "textures") &&
        std::filesystem::exists(candidate / "levels");
}
}

void Initialize(const char* executablePath)
{
    if (!g_assetRoot.empty())
        return;

    std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path(),
        CurrentExecutableDirectory(executablePath)
    };
#ifdef BREAKOUT_ASSET_DIR
    candidates.emplace_back(BREAKOUT_ASSET_DIR);
#endif
    candidates.emplace_back(std::filesystem::current_path().parent_path());

    for (const std::filesystem::path& candidate : candidates)
    {
        if (!candidate.empty() && HasExpectedAssetLayout(candidate))
        {
            g_assetRoot = candidate;
            return;
        }
    }

    throw std::runtime_error("Unable to locate the Breakout asset directory.");
}

std::filesystem::path Resolve(const std::string& relativePath)
{
    if (g_assetRoot.empty())
        Initialize(nullptr);

    return g_assetRoot / relativePath;
}
}
