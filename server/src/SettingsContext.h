#ifndef UNITTESTBOT_SETTINGSCONTEXT_H
#define UNITTESTBOT_SETTINGSCONTEXT_H

#include "Language.h"

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
                        bool instrumentUndefinedBehaviour,
                        Language testLanguage);

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
         * one large embedded harness the instrumentation was five sixths of the
         * bitcode, and every check is a call KLEE has to execute. Since UTBot links the
         * whole project into one module and reloads it for each method, that
         * cost is paid again per method.
         */
        const bool instrumentUndefinedBehaviour;
        /**
         * The language the generated tests are written in.
         *
         * Setting it also publishes it through utbot::TestLanguage, because
         * the pieces that have to agree -- the path a test file is written to,
         * the declarations its header carries, the makefile that builds it --
         * are reached from places that never see a SettingsContext.
         */
        const Language testLanguage;
    };
}


#endif // UNITTESTBOT_SETTINGSCONTEXT_H
