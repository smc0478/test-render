/*
 * pass-registry.h 구현.
 */
#include "pass-registry.h"

namespace RD {

bool GraphBuildContext::hasPass(const std::string &type) const {
    return _registry != nullptr && _registry->has(type);
}

bool GraphBuildContext::getBool(const std::string &key, bool fallback) const {
    auto it = _settings.find(key);
    if(it != _settings.end())
        return it->second;
    return fallback;
}

bool PassRegistry::addFactory(const std::string &type, PassFactory factory) {
    if(type.empty() || factory == nullptr || has(type))
        return false;
    _passes.emplace(type, factory);
    return true;
}

bool PassRegistry::addGraphFactory(const std::string &type, GraphPassFactory factory) {
    if(type.empty() || factory == nullptr || has(type))
        return false;
    _graphs.emplace(type, factory);
    return true;
}

std::unique_ptr<Pass> PassRegistry::create(const std::string &type) const {
    auto it = _passes.find(type);
    if(it != _passes.end())
        return it->second();
    return nullptr;
}

std::unique_ptr<GraphPass> PassRegistry::createGraph(const std::string &type) const {
    auto it = _graphs.find(type);
    if(it != _graphs.end())
        return it->second();
    return nullptr;
}

bool PassRegistry::has(const std::string &type) const {
    return _passes.count(type) != 0 || _graphs.count(type) != 0;
}

}
