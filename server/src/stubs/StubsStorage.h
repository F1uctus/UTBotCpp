#ifndef UTBOTCPP_STUBSSTORAGE_H
#define UTBOTCPP_STUBSSTORAGE_H

#include <string>
#include <unordered_map>
#include <memory>

#include "types/Types.h"

class StubsStorage {
public:
    void registerStub(const std::string &methodName, std::shared_ptr<types::FunctionInfo>,
                      std::optional<fs::path> stubsHeaderPath = std::nullopt);

    std::optional<std::shared_ptr<types::FunctionInfo>>
    getFunctionInfoByKTestObjectName(const std::string &objectName) const;

    std::unordered_set<std::string> getStubsHeaders();

    /// Records a function that is called but defined nowhere, so KLEE answered
    /// its calls with symbolic values instead of dispatching them.
    ///
    /// Kept apart from the stubs above because it is reached differently: those
    /// have a generated source file that KLEE runs and the test build links,
    /// and their values arrive under the name of the variable in it. Nothing is
    /// generated for these before the run, so KLEE names their values after the
    /// function itself, and the stand-in is written into the test file
    /// afterwards, from what came back.
    void registerMockedFunction(const std::shared_ptr<types::FunctionInfo> &functionInfo);

    std::optional<std::shared_ptr<types::FunctionInfo>>
    getMockedFunctionByKTestObjectName(const std::string &objectName) const;

    const std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>> &
    getMockedFunctions() const;

private:
    std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>> _functions;
    std::unordered_set<std::string> _stubsHeaders;
    std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>> _mockedFunctions;
};


#endif //UTBOTCPP_STUBSSTORAGE_H
