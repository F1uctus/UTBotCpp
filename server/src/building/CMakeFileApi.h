#ifndef UNITTESTBOT_CMAKEFILEAPI_H
#define UNITTESTBOT_CMAKEFILEAPI_H

#include "utils/path/FileSystemPath.h"

/**
 * link_commands.json, from what CMake already knows.
 *
 * UTBot needs two build databases: compile_commands.json, which CMake writes
 * itself, and link_commands.json, which describes what each binary is linked
 * from. On Linux both come out of a build that Bear watches -- it interposes on
 * exec and records every compiler and archiver invocation. That mechanism is
 * ELF-specific: there is no LD_PRELOAD on Windows, and the UTBot fork of CMake
 * that emits link_commands.json directly only does so from the Makefile
 * generators.
 *
 * The information is not actually secret, though. CMake's file API reports, per
 * target, what it produces and which sources go into it, and compile_commands
 * says which object each of those sources becomes. Joining the two gives the
 * same graph Bear would have observed, without running the build, without a
 * patched CMake, and from any generator.
 */
namespace CMakeFileApi {
    /**
     * Ask CMake for a codemodel reply on its next configure.
     *
     * Has to happen before cmake runs: the query is a file cmake looks for
     * while generating, and a build tree configured without it answers nothing.
     */
    void writeQuery(const fs::path &buildDirPath);

    /**
     * Turn the reply into link_commands.json beside compile_commands.json.
     *
     * Throws if the reply is missing -- that means the configure did not run,
     * or ran without the query, and a silently absent link database would
     * surface much later as a project with no targets in it.
     */
    void writeLinkCommands(const fs::path &buildDirPath);

    /// Whether a codemodel reply is present, i.e. whether writeLinkCommands
    /// has anything to read.
    bool hasReply(const fs::path &buildDirPath);
}

#endif // UNITTESTBOT_CMAKEFILEAPI_H
