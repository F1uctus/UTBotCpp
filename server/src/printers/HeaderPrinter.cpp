#include "HeaderPrinter.h"

#include "CTestRunner.h"
#include "Paths.h"
#include "utils/FileSystemUtils.h"
#include "utils/KleeOptions.h"

#include <fstream>

namespace printer {
    void HeaderPrinter::print(const fs::path &testHeaderFilePath,
                              const fs::path &sourceFilePath,
                              std::string &headerCode) {
        if (getLanguage() == utbot::Language::C) {
            printForC(testHeaderFilePath, headerCode);
            return;
        }
        processHeader(Include(true, "cstring"));
        // A test case whose expected value is a NaN is printed as NAN, which is
        // a macro rather than a literal. Without this the generated test does
        // not compile at all, and it is the float-heavy units -- the ones the
        // NaN came from -- that lose their whole suite.
        processHeader(Include(true, "cmath"));
        // unistd.h and the stdin redirection below are only reachable
        // through --sym-stdin, which needs the POSIX runtime. Emitting
        // them regardless makes the generated header unbuildable on a
        // platform that has neither.
        if (KleeOptions::targetHasPosixRuntime()) {
            processHeader(Include(true, "unistd.h"));
        }
        processHeader(Include(true, "stdio.h"));
        ss << printer::NL;
        if (KleeOptions::targetHasPosixRuntime()) {
            ss << PrinterUtils::redirectStdin << printer::NL;
        }
        ss << PrinterUtils::writeToFile << printer::NL;
        ss << PrinterUtils::fromBytes << printer::NL;
        ss << PrinterUtils::constCast;
        headerCode += ss.str();
        FileSystemUtils::writeToFile(testHeaderFilePath, headerCode);
    }

    /**
     * The same header for a C test, with the runner in it and the order
     * reversed.
     *
     * Reversed because the C declarations are not wrapped in a namespace and a
     * few of them are macros -- a global the test reaches through a getter is
     * one. A macro named after a project global that landed before <math.h>
     * would rewrite whatever <math.h> happens to declare under that name, so
     * everything this header includes is included first and the project's own
     * names come last.
     */
    void HeaderPrinter::printForC(const fs::path &testHeaderFilePath, std::string &headerCode) {
        // math.h for the NAN and INFINITY macros an expected value may be
        // printed as; the runner brings the rest of what it needs itself.
        processHeader(Include(true, "math.h"));
        ss << printer::NL;
        ss << CTestRunner::runtime() << printer::NL;
        ss << PrinterUtils::writeToFileC << printer::NL;
        headerCode = ss.str() + headerCode;
        FileSystemUtils::writeToFile(testHeaderFilePath, headerCode);
    }

    void HeaderPrinter::processHeader(const Include &relatedHeader) {
        if (relatedHeader.is_angled) {
            strIncludeSystem(relatedHeader.path);
        } else {
            strInclude(relatedHeader.path);
        }
    }

    utbot::Language HeaderPrinter::getLanguage() const {
        return Paths::generateCTestsFor(sourceFilePath) ? utbot::Language::C
                                                        : utbot::Language::CXX;
    }
}
