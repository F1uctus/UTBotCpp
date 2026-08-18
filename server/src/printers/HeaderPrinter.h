#ifndef UNITTESTBOT_HEADERPRINTER_H
#define UNITTESTBOT_HEADERPRINTER_H

#include "Paths.h"
#include "Printer.h"
#include "Tests.h"
#include "types/Types.h"

namespace printer {
    class HeaderPrinter : Printer {
    public:
        explicit HeaderPrinter(fs::path sourceFilePath)
            : Printer(Paths::getSourceLanguage(sourceFilePath)),
              sourceFilePath(std::move(sourceFilePath)) {
        }

        utbot::Language getLanguage() const override;

        void print(const fs::path &testHeaderFilePath,
                   const fs::path &sourceFilePath,
                   std::string &headerCode);

    private:
        /// The source the header is generated for; it decides whether the test
        /// that includes this header is C or C++.
        fs::path sourceFilePath;

        void printForC(const fs::path &testHeaderFilePath, std::string &headerCode);

        void processHeader(const Include &relatedHeader);
    };
}

#endif //UNITTESTBOT_HEADERPRINTER_H
