#ifndef UNITTESTBOT_CTESTRUNNER_H
#define UNITTESTBOT_CTESTRUNNER_H

#include <string>
#include <vector>

/**
 * The parts of a generated C test binary that are not the tests.
 *
 * A C++ test file gets its TEST macro, its assertions and its main() from
 * gtest. There is no such library for C that the answer could be "link
 * against it" -- and a target board would have to be able to build it anyway
 * -- so a C test file carries them: the runtime below goes into the generated
 * header, and the entry point below closes the test file.
 *
 * Between them they cover exactly what the generated tests use, so the pieces
 * here have to keep pace with what the visitors emit rather than with gtest.
 */
namespace printer {
namespace CTestRunner {
    /// One generated test, as the table in the entry point names it.
    struct TestEntry {
        std::string suite;
        std::string name;
    };

    /**
     * The assertion macros, the reporting behind them, and the runner main()
     * hands control to. Belongs in the generated header, before anything that
     * uses it.
     */
    const std::string &runtime();

    /**
     * The table of tests and the main() that runs them.
     *
     * The names are the functions TEST(suite, name) defined, in the order they
     * were printed, so listing them reproduces the grouping gtest would show.
     */
    std::string entryPoint(const std::vector<TestEntry> &tests);
}
}

#endif // UNITTESTBOT_CTESTRUNNER_H
