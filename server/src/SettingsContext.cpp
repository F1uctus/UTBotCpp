#include "SettingsContext.h"

#include <protobuf/testgen.grpc.pb.h>

namespace utbot {
    SettingsContext::SettingsContext(bool generateForStaticFunctions,
                                     bool verbose,
                                     int32_t timeoutPerFunction,
                                     int32_t timeoutPerTest,
                                     bool useDeterministicSearcher,
                                     bool useStubs,
                                     testsgen::ErrorMode errorMode,
                                     bool differentVariablesOfTheSameType,
                                     bool skipObjectWithoutSource,
                                     bool instrumentUndefinedBehaviour,
                                     Language testLanguage)
            : generateForStaticFunctions(generateForStaticFunctions),
              verbose(verbose),
              timeoutPerFunction(timeoutPerFunction > 0
                                 ? std::make_optional(std::chrono::seconds{timeoutPerFunction})
                                 : std::nullopt),
              timeoutPerTest(timeoutPerTest > 0
                             ? std::make_optional(std::chrono::seconds{timeoutPerTest})
                             : std::nullopt),
              useDeterministicSearcher(useDeterministicSearcher), useStubs(useStubs),
              errorMode(errorMode),
              differentVariablesOfTheSameType(differentVariablesOfTheSameType),
              skipObjectWithoutSource(skipObjectWithoutSource),
              instrumentUndefinedBehaviour(instrumentUndefinedBehaviour),
              testLanguage(testLanguage) {
        // Everything downstream of here reads the choice from one place; see
        // the field's documentation for why it cannot simply be passed along.
        TestLanguage::set(testLanguage);
    }

    SettingsContext::SettingsContext(const testsgen::SettingsContext &settingsContext)
            : SettingsContext(settingsContext.generateforstaticfunctions(),
                          settingsContext.verbose(),
                          settingsContext.timeoutperfunction(),
                          settingsContext.timeoutpertest(),
                          settingsContext.usedeterministicsearcher(),
                          settingsContext.usestubs(),
                          settingsContext.errormode(),
                          settingsContext.differentvariablesofthesametype(),
                          settingsContext.skipobjectwithoutsource(),
                          settingsContext.instrumentundefinedbehaviour(),
                          settingsContext.testlanguage() == testsgen::TEST_LANGUAGE_C
                              ? Language::C
                              : Language::CXX) {
    }
}
