#include "HeaderPrinter.h"

#include "utils/FileSystemUtils.h"

#include <fstream>

namespace printer {
    void HeaderPrinter::print(const fs::path &testHeaderFilePath,
                              const fs::path &sourceFilePath,
                              std::string &headerCode) {
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
        ss << PrinterUtils::redirectStdin << printer::NL;
        ss << PrinterUtils::writeToFile << printer::NL;
        ss << PrinterUtils::fromBytes << printer::NL;
        ss << PrinterUtils::constCast;
        headerCode += ss.str();
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
        return utbot::Language::CXX;
    }
}
