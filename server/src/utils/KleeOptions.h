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

    /**
     * True if the target KLEE builds the POSIX runtime.
     *
     * It models a POSIX environment -- stdin, a filesystem, argv -- and is
     * POSIX by construction, so the Windows build does not include it. The
     * generated harness has to know, because klee_init_env and the stdin check
     * live there and calling them without it fails the run outright.
     */
    bool targetHasPosixRuntime();

    /**
     * True if the target KLEE reasons about floating point symbolically.
     *
     * Upstream does not: Executor.cpp turns both operands of an FCmp into
     * constants and compares those, so a comparison between two symbolic floats
     * is decided before the solver sees it. UnitTestBot's fork adds this behind
     * --fp-runtime.
     *
     * It matters to the generated harness rather than only to the command line,
     * because the post-state of a float has to be captured by a comparison that
     * the solver can actually reason about.
     */
    bool targetHasSymbolicFloatingPoint();
}

#endif // UNITTESTBOT_KLEEOPTIONS_H
