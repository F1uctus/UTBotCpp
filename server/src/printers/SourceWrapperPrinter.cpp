#include "SourceWrapperPrinter.h"

#include "Paths.h"
#include "utils/FileSystemUtils.h"
#include "utils/StubsUtils.h"

namespace printer {
    SourceWrapperPrinter::SourceWrapperPrinter(utbot::Language srcLanguage) : Printer(srcLanguage) {
    }

    void SourceWrapperPrinter::printStandInsForMockedFunctions(
        const std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>>
            &mockedFunctions) {
        if (mockedFunctions.empty()) {
            return;
        }

        const size_t capacity =
            types::TypesHandler::getElementsNumberInPointerOneDim(types::PointerUsage::PARAMETER);

        strComment("Stand-ins for the functions KLEE answers instead of calling.");
        strComment("One that returns a value reads it from an array a test fills with what its");
        strComment("run recorded, one entry per call; one that returns nothing has nothing to");
        strComment("replay and only has to answer the call here.");
        ss << printer::NL;

        // Weak, so that a binary can hold more than one unit's wrappers.
        //
        // Two units that both call the same undefined function each get a
        // stand-in for it, and a binary that carries both -- which is what
        // happens as soon as a unit reaches a global or a helper defined in its
        // neighbour -- could not be linked: two definitions of one name. Weak
        // makes them interchangeable, which they are: each is the same function
        // reading its own recorded answers, and the one the linker keeps is fed
        // by whichever test is running.
        //
        // Spelled per compiler, because the ones that build these tests do not
        // agree: IAR takes __weak before the declaration, GCC and clang take an
        // attribute. A compiler that has neither gets a strong definition and
        // the old behaviour.
        //
        // The arrays need a second spelling on PE, where a weak definition is
        // emitted as an alias that the linker's auto-export pass skips. The
        // definition is in the shared object either way, but nothing outside it
        // can name one that is not exported, and a test that cannot name the
        // array cannot fill it -- so every test linked against a wrapper that
        // stands in for anything failed to link. selectany is the COMDAT the
        // export pass does keep, and it folds duplicates the same way weak
        // does. It wants an initializer, which is why the arrays carry one.
        ss << "#if defined(__ICCARM__)" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE __weak" << printer::NL
           << "#define UTBOT_MOCK_STORAGE __weak" << printer::NL
           << "#elif defined(_WIN32) && (defined(__GNUC__) || defined(_MSC_VER))" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE __attribute__((weak))" << printer::NL
           << "#define UTBOT_MOCK_STORAGE __declspec(selectany)" << printer::NL
           << "#elif defined(__GNUC__)" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE __attribute__((weak))" << printer::NL
           << "#define UTBOT_MOCK_STORAGE __attribute__((weak))" << printer::NL
           << "#else" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE" << printer::NL
           << "#define UTBOT_MOCK_STORAGE" << printer::NL
           << "#endif" << printer::NL << printer::NL;

        for (const auto &[functionName, functionInfo]: mockedFunctions) {
            const std::string varName = StubsUtils::getMockedFunctionVarName(functionName);
            const std::string counterName =
                StubsUtils::getMockedFunctionCounterName(functionName);
            const std::string returnType = functionInfo->returnType.usedType();

            // A function that returns nothing has no answer to hand back, so it
            // gets no array and no counter: what a test needs from it is that
            // the call is answered here rather than escaping the test binary,
            // and that it resolves at link time.
            const bool returnsNothing =
                types::TypesHandler::isVoid(functionInfo->returnType);

            if (!returnsNothing) {
                ss << "UTBOT_MOCK_STORAGE " << returnType << " " << varName << "[" << capacity
                   << "] = {0};" << printer::NL;
                ss << "UTBOT_MOCK_STORAGE int " << counterName << " = 0;" << printer::NL;
            }

            ss << "UTBOT_MOCK_LINKAGE " << returnType << " " << functionName << "(";
            for (size_t i = 0; i < functionInfo->params.size(); i++) {
                const auto &param = functionInfo->params[i];
                ss << param.type.usedType();
                if (!param.name.empty()) {
                    ss << " " << param.name;
                }
                if (i + 1 != functionInfo->params.size()) {
                    ss << ", ";
                }
            }
            if (functionInfo->params.empty()) {
                ss << "void";
            }
            ss << ") {" << printer::NL;
            for (const auto &param: functionInfo->params) {
                if (!param.name.empty()) {
                    // What was recorded already accounts for the arguments;
                    // naming them only to ignore them would warn.
                    ss << "    (void) " << param.name << ";" << printer::NL;
                }
            }
            if (!returnsNothing) {
                // A run that answered more calls than the array holds keeps the
                // last answer rather than reading past the end. The path a test
                // replays never gets there, and repeating is the lesser wrong.
                ss << "    if (" << counterName << " >= " << capacity << ") {" << printer::NL;
                ss << "        return " << varName << "[" << capacity - 1 << "];" << printer::NL;
                ss << "    }" << printer::NL;
                ss << "    return " << varName << "[" << counterName << "++];" << printer::NL;
            }
            ss << "}" << printer::NL;
        }
        ss << printer::NL;
    }

    void SourceWrapperPrinter::print(const utbot::ProjectContext &projectContext,
                                     const fs::path &sourceFilePath,
                                     const std::string &wrapperDefinitions,
                                     const std::unordered_map<
                                         std::string, std::shared_ptr<types::FunctionInfo>>
                                         &mockedFunctions) {
        if (Paths::isCXXFile(sourceFilePath))
            return;
        writeCopyrightHeader();

        strDefine("main", "main__");

        fs::path wrapperFilePath = Paths::getWrapperFilePath(projectContext, sourceFilePath);

        fs::path sourcePathRelativeToProjectDir = fs::relative(sourceFilePath, projectContext.projectPath);
        fs::path projectDirRelativeToWrapperFile =
                fs::relative(projectContext.projectPath, wrapperFilePath.parent_path());

        strInclude(Include(false, projectDirRelativeToWrapperFile / sourcePathRelativeToProjectDir));

        ss << "#pragma GCC visibility push (default)" << printer::NL;

        // After the include: the source is where the callee is declared, and a
        // definition has to agree with a declaration that is already in scope.
        printStandInsForMockedFunctions(mockedFunctions);

        ss << wrapperDefinitions;

        ss << "#pragma GCC visibility pop" << printer::NL;

        FileSystemUtils::writeToFile(wrapperFilePath, ss.str());
    }
}
