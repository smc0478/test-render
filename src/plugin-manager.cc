/*
 * plugin-manager.h 구현. OS DLL API(LoadLibrary / GetProcAddress)는 이 파일에서만 쓴다.
 */
#include "plugin-manager.h"

#include <algorithm>
#include <filesystem>

#include <windows.h>

namespace RD {

PluginManager::~PluginManager() {
    unloadAll();
}

size_t PluginManager::loadAll(const std::string &dir, PluginHost &host) {
    std::error_code ec;
    if(!std::filesystem::is_directory(dir, ec)) {
        _diagnostics.push_back({Severity::Warning, "plugin directory '" + dir + "' not found"});
        return 0;
    }

    std::vector<std::filesystem::path> paths;
    for(const auto &entry : std::filesystem::directory_iterator(dir, ec))
        if(entry.is_regular_file() && entry.path().extension() == ".dll")
            paths.push_back(entry.path());
    std::sort(paths.begin(), paths.end());

    size_t count = 0;
    for(const auto &path : paths)
        if(load(path.string(), host))
            ++count;
    return count;
}

bool PluginManager::load(const std::string &path, PluginHost &host) {
    HMODULE module = LoadLibraryA(path.c_str());
    if(module == nullptr) {
        _diagnostics.push_back({Severity::Error, "cannot load plugin '" + path + "'"});
        return false;
    }
    auto entry = reinterpret_cast<PluginEntry>(reinterpret_cast<void *>(GetProcAddress(module, "rd_plugin_entry")));
    if(entry == nullptr) {
        FreeLibrary(module);
        _diagnostics.push_back({Severity::Error, "'" + path + "' has no rd_plugin_entry"});
        return false;
    }
    if(!entry(&host)) {
        FreeLibrary(module);
        _diagnostics.push_back({Severity::Error, "'" + path + "' rejected registration (ABI or schema conflict)"});
        return false;
    }
    _modules.push_back(module);
    _names.push_back(std::filesystem::path(path).filename().string());
    return true;
}

void PluginManager::unloadAll() {
    for(auto it = _modules.rbegin(); it != _modules.rend(); ++it)
        FreeLibrary(static_cast<HMODULE>(*it));
    _modules.clear();
    _names.clear();
}

}
