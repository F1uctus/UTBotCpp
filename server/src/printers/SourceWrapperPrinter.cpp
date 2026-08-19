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

        strComment("Stand-ins for the functions KLEE answers instead of calling.") << printer::NL;
        strComment("A test fills the array with what its run recorded, one entry per call.")
            << printer::NL;

        // Weak, so that a binary can hold more than one unit's wrappers.
        //
        // Two units that both call the same driver each get a stand-in for it,
        // and a test image that carries both -- which is what happens as soon
        // as a unit reaches a global or a helper defined in its neighbour --
        // could not be linked: two definitions of PORT_ReadInputDataBit. Weak
        // makes them interchangeable, which they are: each is the same
        // function reading its own recorded answers, and the one the linker
        // keeps is fed by whichever test is running.
        //
        // Spelled per compiler, because the two that build these tests do not
        // agree: IAR takes __weak before the declaration, GCC and clang take
        // an attribute. A compiler that has neither gets a strong definition
        // and the old behaviour.
        ss << "#if defined(__ICCARM__)" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE __weak" << printer::NL
           << "#elif defined(__GNUC__)" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE __attribute__((weak))" << printer::NL
           << "#else" << printer::NL
           << "#define UTBOT_MOCK_LINKAGE" << printer::NL
           << "#endif" << printer::NL << printer::NL;

        for (const auto &[functionName, functionInfo]: mockedFunctions) {
            const std::string varName = StubsUtils::getMockedFunctionVarName(functionName);
            const std::string counterName =
                StubsUtils::getMockedFunctionCounterName(functionName);
            const std::string returnType = functionInfo->returnType.usedType();

            ss << "UTBOT_MOCK_LINKAGE " << returnType << " " << varName << "[" << capacity << "];"
               << printer::NL;
            ss << "UTBOT_MOCK_LINKAGE int " << counterName << ";" << printer::NL;

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
            // A run that answered more calls than the array holds keeps the
            // last answer rather than reading past the end. The path a test
            // replays never gets there, and repeating is the lesser wrong.
            ss << "    if (" << counterName << " >= " << capacity << ") {" << printer::NL;
            ss << "        return " << varName << "[" << capacity - 1 << "];" << printer::NL;
            ss << "    }" << printer::NL;
            ss << "    return " << varName << "[" << counterName << "++];" << printer::NL;
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
