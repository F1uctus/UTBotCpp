#ifndef UNITTESTBOT_SOURCEWRAPPERPRINTER_H
#define UNITTESTBOT_SOURCEWRAPPERPRINTER_H

#include "Printer.h"
#include "ProjectContext.h"
#include "building/BuildDatabase.h"

namespace printer {
    class SourceWrapperPrinter : public Printer {

    public:
        explicit SourceWrapperPrinter(utbot::Language srcLanguage);

        void print(const utbot::ProjectContext &projectContext,
                      const fs::path &sourceFilePath,
                      const std::string &wrapperDefinitions,
                      const std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>>
                          &mockedFunctions);

    private:
        /**
         * Defines the functions this file calls that nothing defines.
         *
         * KLEE answers those calls with symbolic values, and a generated test
         * replays them by filling the array each stand-in reads. The definition
         * belongs here rather than in the test file because the test build
         * links the project into a shared library before it links the tests,
         * and that link has to resolve every symbol the project references.
         *
         * The wrapper is compiled in place of the source it wraps, so a
         * stand-in defined here is in the same object as the code that calls
         * it, and there is no build to teach about a new file.
         */
        void printStandInsForMockedFunctions(
            const std::unordered_map<std::string, std::shared_ptr<types::FunctionInfo>>
                &mockedFunctions);
    };
}


#endif // UNITTESTBOT_SOURCEWRAPPERPRINTER_H
