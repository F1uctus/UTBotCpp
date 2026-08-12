#include "HeaderPrinter.h"

#include "utils/FileSystemUtils.h"
#include "utils/KleeOptions.h"

#include <fstream>

namespace printer {
    void HeaderPrinter::print(const fs::path &testHeaderFilePath,
                              const fs::path &sourceFilePath,
                              std::string &headerCode) {
        processHeader(Include(true, "cstring"));
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
