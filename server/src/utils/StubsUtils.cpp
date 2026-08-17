#include "StubsUtils.h"
#include "PrinterUtils.h"

namespace StubsUtils {
    std::string getFunctionPointerStubName(const std::optional<std::string> &scopeName,
                                           const std::string &methodName,
                                           const std::string &paramName,
                                           bool omitSuffix) {
        std::string stubName = "*" + scopeName.value_or("") + "_" + methodName;
        if (!omitSuffix) {
            stubName += "_" + paramName + Paths::STUB_SUFFIX;
        } else {
            stubName = stubName.substr(1);
        }
        StringUtils::replaceColon(stubName);
        return stubName;
    }

    std::string getFunctionPointerAsStructFieldStubName(const std::string &structName,
                                                        const std::string &fieldName,
                                                        bool omitSuffix) {
        std::string stubName = "*" + structName;
        if (!omitSuffix) {
            stubName += "_" + fieldName + Paths::STUB_SUFFIX;
        } else {
            stubName = stubName.substr(1);
        }
        StringUtils::replaceColon(stubName);
        return stubName;
    }

    std::string getMockedFunctionVarName(const std::string &methodName) {
        std::string name = methodName + "_utbot_mock";
        StringUtils::replaceColon(name);
        return name;
    }

    std::string getMockedFunctionCounterName(const std::string &methodName) {
        return getMockedFunctionVarName(methodName) + "_call";
    }

    std::string getStubSymbolicVarName(const std::string &methodName, const std::string &parentMethodName) {
        std::string stubName;
        if (!parentMethodName.empty()) {
            stubName = parentMethodName + "_";
        }
        stubName += methodName + PrinterUtils::KLEE_SYMBOLIC_SUFFIX;
        StringUtils::replaceColon(stubName);
        return stubName;
    }
}
