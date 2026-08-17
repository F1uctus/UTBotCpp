#ifndef UTBOTCPP_STUBUTILS_H
#define UTBOTCPP_STUBUTILS_H

#include <string>
#include <optional>

namespace StubsUtils {
    std::string getFunctionPointerStubName(const std::optional<std::string> &scopeName,
                                           const std::string &methodName,
                                           const std::string &paramName,
                                           bool omitSuffix);

    std::string getFunctionPointerAsStructFieldStubName(const std::string &structName,
                                                        const std::string &fieldName,
                                                        bool omitSuffix);

    std::string getStubSymbolicVarName(const std::string &methodName, const std::string &parentMethodName);

    /// The array a test fills with the values KLEE's mock of \p methodName
    /// returned, one per call, in order.
    std::string getMockedFunctionVarName(const std::string &methodName);

    /// How far through that array the stand-in has read. Reset by each test, so
    /// that tests in one binary do not consume each other's values.
    std::string getMockedFunctionCounterName(const std::string &methodName);
}

#endif //UTBOTCPP_STUBUTILS_H
