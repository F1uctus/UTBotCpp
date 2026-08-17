#include "StubsStorage.h"

#include <utility>
#include "utils/StubsUtils.h"

void StubsStorage::registerStub(const std::string &methodName, std::shared_ptr<types::FunctionInfo> functionInfo,
                                std::optional<fs::path> stubsHeaderPath) {
    if (stubsHeaderPath.has_value()) {
        _stubsHeaders.insert(stubsHeaderPath.value());
    }
    _functions[StubsUtils::getStubSymbolicVarName(functionInfo->name, methodName)] = std::move(functionInfo);
}

std::optional<std::shared_ptr<types::FunctionInfo>>
StubsStorage::getFunctionInfoByKTestObjectName(const std::string &objectName) const {
    return CollectionUtils::getOptionalValue(_functions, objectName);
}

std::unordered_set<std::string> StubsStorage::getStubsHeaders() {
    return _stubsHeaders;
}

void StubsStorage::registerMockedFunction(const std::shared_ptr<types::FunctionInfo> &functionInfo) {
    // Keyed by the bare function name: that is what KLEE calls the object it
    // records the mocked value in, and matching it is the whole point.
    _mockedFunctions[functionInfo->name] = functionInfo;
}

std::optional<std::shared_ptr<types::FunctionInfo>>
StubsStorage::getMockedFunctionByKTestObjectName(const std::string &objectName) const {
    return CollectionUtils::getOptionalValue(_mockedFunctions, objectName);
}

const std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>> &
StubsStorage::getMockedFunctions() const {
    return _mockedFunctions;
}
