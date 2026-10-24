#include "mod/ThirstPacks.h"

#include <algorithm>
#include <filesystem>

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "mc/deps/core/file/Path.h"
#include "mc/resources/CompositePackSource.h"
#include "mc/resources/DirectoryPackSource.h"
#include "mc/resources/PackSourceFactory.h"
#include "mc/resources/RepositorySources.h"
#include "mc/resources/ResourcePackRepository.h"
#include "mod/MyMod.h"

namespace thirsty {
namespace {

bool addPackSources(RepositorySources& sources, PackSourceFactory& factory) {
    auto& mod = my_mod::MyMod::getInstance().getSelf();
    auto& list = *sources.mPackSource->mPackSources;
    bool added = false;
    auto add = [&](std::filesystem::path const& directory, PackType type) {
        // System packs are enabled by the game's addSystemPacksToStack path.
        // The native factory owns the source and the repository owns its packs.
        auto& source = factory.createDirectoryPackSource(
            Core::Path(std::filesystem::absolute(directory)), type, PackOrigin::System, false
        );
        if (std::find(list.begin(), list.end(), &source) == list.end()) {
            list.push_back(&source);
            added = true;
        }
    };
    add(mod.getResourceDir(), PackType::Resources);
    add(mod.getBehaviorDir(), PackType::Behavior);
    return added;
}

LL_TYPE_INSTANCE_HOOK(
    ThirstPackSourcesHook,
    ll::memory::HookPriority::Normal,
    RepositorySources,
    &RepositorySources::initializePackSource,
    void,
    PackSourceFactory& factory
) {
    origin(factory);
    addPackSources(*this, factory);
}

void addToExistingRepository(bool clientSide) {
    auto repository = ll::service::getResourcePackRepository(clientSide);
    if (repository && repository->isInitialized()
        && addPackSources(*repository->mSources, repository->getPackSourceFactory())) {
        repository->refreshPacks();
    }
}

} // namespace

bool loadThirstPacks() {
    auto& mod = my_mod::MyMod::getInstance().getSelf();
    if (!std::filesystem::is_regular_file(mod.getResourceDir() / "thirsty/manifest.json")
        || !std::filesystem::is_regular_file(mod.getBehaviorDir() / "thirsty/manifest.json")) {
        mod.getLogger().error("饮水物品包缺失，请安装完整 thirsty 文件夹（包含 resource_packs 和 behavior_packs）。");
        return false;
    }

    // Register during mod load, before world packs and item registries are built.
    // This also covers repositories created later for new single-player worlds.
    ThirstPackSourcesHook::hook();
#ifdef LL_PLAT_C
    addToExistingRepository(true);
#endif
    addToExistingRepository(false);
    mod.getLogger().info("饮水物品包已接入自动加载。");
    return true;
}

} // namespace thirsty
