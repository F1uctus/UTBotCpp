#ifndef UNITTESTBOT_TESTSPRINTER_H
#define UNITTESTBOT_TESTSPRINTER_H

#include "CTestRunner.h"
#include "Printer.h"
#include "Tests.h"
#include "building/BuildDatabase.h"
#include "stubs/Stubs.h"
#include "types/Types.h"
#include "utils/PrinterUtils.h"
#include "utils/path/FileSystemPath.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using tests::Tests;
using namespace ::testsgen;

namespace printer {
    class TestsPrinter : public Printer {
    public:
        explicit TestsPrinter(const utbot::ProjectContext &projectContext,
                              const types::TypesHandler *typesHandler,
                              utbot::Language srcLanguage);

        utbot::Language getLanguage() const override;

        void genCode(Tests::MethodDescription &methodDescription,
                     const std::optional<LineInfo::PredicateInfo> &predicateInfo = {},
                     bool verbose = false,
                     ErrorMode errorMode = ErrorMode::FAILING);

        void joinToFinalCode(Tests &tests, const fs::path &generatedHeaderPath);

        static bool needsMathHeader(const Tests &tests);

        void genHeaders(Tests &tests, const fs::path &generatedHeaderPath);

        void genParametrizedTestCase(const tests::Tests::MethodDescription &methodDescription,
                                     const Tests::MethodTestCase &testCase,
                                     const std::optional<LineInfo::PredicateInfo>& predicateInfo,
                                     ErrorMode errorMode);

        void genVerboseTestCase(const tests::Tests::MethodDescription &methodDescription,
                                const Tests::MethodTestCase &testCase,
                                const std::optional<LineInfo::PredicateInfo> &predicateInfo,
                                ErrorMode errorMode);

        void testHeader(const Tests::MethodTestCase &testCase);

        void redirectStdin(const tests::Tests::MethodDescription &methodDescription,
                           const Tests::MethodTestCase &testCase,
                           bool verbose);

        void verboseParameter(const tests::Tests::MethodDescription &method,
                              const Tests::MethodParam &param,
                              const Tests::TestCaseParamValue &value,
                              bool needDeclaration);

        void printClassObject(const tests::Tests::MethodDescription &methodDescription,
                              const Tests::MethodTestCase &testCase);

        void verboseParameters(const tests::Tests::MethodDescription &methodDescription,
                               const Tests::MethodTestCase &testCase);

        void printFunctionParameters(const tests::Tests::MethodDescription &methodDescription,
                                     const Tests::MethodTestCase &testCase,
                                     bool all);

        void verboseOutputVariable(const tests::Tests::MethodDescription &methodDescription,
                                   const Tests::MethodTestCase &testCase);

        void verboseFunctionCall(const tests::Tests::MethodDescription &methodDescription,
                                 const Tests::MethodTestCase &testCase,
                                 ErrorMode errorMode);

        void verboseAsserts(const tests::Tests::MethodDescription &methodDescription,
                            const Tests::MethodTestCase &testCase,
                            const std::optional<LineInfo::PredicateInfo> &predicateInfo);

        void classAsserts(const Tests::MethodDescription &methodDescription,
                          const Tests::MethodTestCase &testCase);

        void changeableParamsAsserts(const Tests::MethodDescription &methodDescription,
                                     const Tests::MethodTestCase &testCase);

        void globalParamsAsserts(const Tests::MethodDescription &methodDescription,
                                 const Tests::MethodTestCase &testCase);

        /**
         * How this test spells a global: its own name, or, for one the source
         * declared static in C, a dereference of the getter the wrapper exports.
         */
        std::string globalAccess(const Tests::MethodDescription &methodDescription,
                                 const Tests::MethodParam &param) const;

        /// \p param with globalAccess() in place of its name.
        Tests::MethodParam globalParamAs(const Tests::MethodDescription &methodDescription,
                                         const Tests::MethodParam &param) const;

        void parametrizedAsserts(const tests::Tests::MethodDescription &methodDescription,
                                 const Tests::MethodTestCase &testCase,
                                 const std::optional<LineInfo::PredicateInfo>& predicateInfo,
                                 ErrorMode errorMode);

        void markTestedFunctionCallIfNeed(const std::string &name,
                                          const Tests::MethodTestCase &testCase);

        void printFinalCodeAndAlterJson(Tests &tests);

        std::vector<std::string>
        methodParametersListParametrized(const tests::Tests::MethodDescription &methodDescription,
                                         const Tests::MethodTestCase &testCase);

        static std::vector<std::string>
        methodParametersListVerbose(const tests::Tests::MethodDescription &methodDescription,
                                    const Tests::MethodTestCase &testCase);


        std::string constrVisitorFunctionCall(const tests::Tests::MethodDescription &methodDescription,
                                              const Tests::MethodTestCase &testCase,
                                              bool verboseMode,
                                              ErrorMode errorMode);

        struct FunctionSignature {
            std::string name;
            std::vector<std::string> args;
        };

    private:
        utbot::ProjectContext const projectContext;
        types::TypesHandler const *typesHandler;

        static bool paramNeedsMathHeader(const Tests::TestCaseParamValue &paramValue);

        void
        parametrizedInitializeGlobalVariables(const Tests::MethodDescription &methodDescription,
                                              const Tests::MethodTestCase &testCase);

        /// Puts every stand-in back at its first recorded answer, so that a
        /// test replays its own path rather than continuing the previous one.
        void resetMockedFunctionCounters(const Tests::MethodDescription &methodDescription);

        void parametrizedInitializeSymbolicStubs(const Tests::MethodDescription &methodDescription,
                                                 const Tests::MethodTestCase &testCase);

        void printLazyVariables(const Tests::MethodDescription &methodDescription,
                                const Tests::MethodTestCase &testCase,
                                bool verbose);

        void initializeFiles(const Tests::MethodDescription &methodDescription,
                             const Tests::MethodTestCase &testCase);

        void openFiles(const Tests::MethodDescription &methodDescription,
                       const Tests::MethodTestCase &testCase);

        void printLazyVariables(const std::vector<Tests::MethodParam> &lazyParams,
                                const std::vector<Tests::TestCaseParamValue> &lazyValues);

        void printLazyReferences(const Tests::MethodDescription &methodDescription,
                                 const Tests::MethodTestCase &testCase,
                                 bool verbose);

        void printStubVariablesForParam(const Tests::MethodDescription &methodDescription,
                                        const Tests::MethodTestCase &testCase);

        void printPointerParameter(const tests::Tests::MethodDescription &methodDescription,
                                   const Tests::MethodTestCase &testCase,
                                   int param_num);

        static Tests::MethodParam getValueParam(const Tests::MethodParam &param);

        void genCodeBySuiteName(const std::string &targetSuiteName,
                                Tests::MethodDescription &methodDescription,
                                const std::optional<LineInfo::PredicateInfo> &predicateInfo,
                                bool verbose,
                                int &testNum,
                                ErrorMode errorMode);

        /// Prints one suite. \p printedTests, when given, collects what was
        /// printed, which is what the C runner's table has to name.
        std::uint32_t
        printSuiteAndReturnMethodsCount(const std::string &suiteName,
                                        const Tests::MethodsMap &methods,
                                        std::vector<CTestRunner::TestEntry> *printedTests = nullptr);

        std::string strFail(const std::string &message);

    public:
        /// A brace list made into something that can stand where a value is
        /// expected; see the definition.
        std::string constrStructValue(const types::Type &type, const std::string &value);

    private:
        std::string constrArgumentValue(const Tests::MethodParam &param,
                                        const Tests::TestCaseParamValue &value);

        void printFailAssertion(ErrorMode errorMode);
    };
}
#endif // UNITTESTBOT_TESTSPRINTER_H
