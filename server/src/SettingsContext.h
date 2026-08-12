#ifndef UNITTESTBOT_SETTINGSCONTEXT_H
#define UNITTESTBOT_SETTINGSCONTEXT_H

#include <chrono>
#include <optional>
#include <protobuf/testgen.grpc.pb.h>

namespace testsgen {
    class SettingsContext;
}

namespace utbot {
    class SettingsContext {
    public:
        explicit SettingsContext(const testsgen::SettingsContext &settingsContext);

        SettingsContext(bool generateForStaticFunctions,
                        bool verbose,
                        int32_t timeoutPerFunction,
                        int32_t timeoutPerTest,
                        bool useDeterministicSearcher,
                        bool useStubs,
                        testsgen::ErrorMode errorMode,
                        bool differentVariablesOfTheSameType,
                        bool skipObjectWithoutSource,
                        bool instrumentUndefinedBehaviour);

        const bool generateForStaticFunctions;
        const bool verbose;
        const std::optional<std::chrono::seconds> timeoutPerFunction, timeoutPerTest;
        const bool useDeterministicSearcher;
        const bool useStubs;
        testsgen::ErrorMode errorMode;
        const bool differentVariablesOfTheSameType;
        const bool skipObjectWithoutSource;
        /**
         * Whether the bitcode handed to KLEE is compiled with UBSan checks.
         *
         * They let KLEE report undefined behaviour, and they are expensive: on
         * one T1100 harness the instrumentation was five sixths of the bitcode,
         * and every check is a call KLEE has to execute. Since UTBot links the
         * whole project into one module and reloads it for each method, that
         * cost is paid again per method.
         */
        const bool instrumentUndefinedBehaviour;
    };
}


#endif // UNITTESTBOT_SETTINGSCONTEXT_H
