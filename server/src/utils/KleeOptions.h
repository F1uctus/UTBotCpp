#ifndef UNITTESTBOT_KLEEOPTIONS_H
#define UNITTESTBOT_KLEEOPTIONS_H

#include <string>
#include <vector>

/**
 * Adapts the KLEE command line to the KLEE actually being run.
 *
 * The server was written against UnitTestBot's KLEE fork and passes options
 * that only exist there -- lazy initialisation, the advanced type system, its
 * own spelling of the deterministic allocator. Handing those to an
 * upstream-derived KLEE makes it exit immediately on an unknown option, so the
 * command has to be translated rather than passed through.
 *
 * The translation is deliberately explicit: an option that names a capability
 * the target KLEE does not have is dropped and reported once, so a reduced
 * result is visible rather than silently produced.
 */
namespace KleeOptions {
    /**
     * Rewrites \p argv for the KLEE at \p kleeExecutable.
     *
     * The first element is the program name and is left in place; the caller
     * decides how to spawn it.
     */
    std::vector<std::string> adapt(const std::vector<std::string> &argv);

    /**
     * True if this build is talking to a KLEE that provides UnitTestBot's
     * extensions, in which case nothing is dropped.
     */
    bool targetHasUnitTestBotExtensions();
}

#endif // UNITTESTBOT_KLEEOPTIONS_H
