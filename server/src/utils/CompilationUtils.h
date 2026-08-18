#ifndef UNITTESTBOT_COPMILATIONUTILS_H
#define UNITTESTBOT_COPMILATIONUTILS_H

#include "building/CompilationDatabase.h"

#include "json.hpp"

#include "ProjectContext.h"
#include "utils/path/FileSystemPath.h"
#include <memory>

namespace CompilationUtils {
    using json = nlohmann::json;

    enum class CompilerName {
        GCC,
        GXX,
        CLANG,
        CLANGXX,
        UNKNOWN
    };

    std::string to_string(CompilerName compilerName);

    inline static const std::string GCC_PATH_PATTERN = "gcc";
    inline static const std::string GXX_PATH_PATTERN = "g++";
    inline static const std::string CLANG_PATH = "clang";
    inline static const std::string CLANGXX_PATH = "clang++";

    static inline const std::string UTBOT_FILES_DIR_NAME = "utbot_files";
    static inline const std::string UTBOT_BUILD_DIR_NAME = "utbot_build";

    // A build recipe runs through a POSIX shell on both platforms: the
    // makefile preamble sets SHELL, and on Windows that is the busybox the
    // distribution carries rather than cmd.exe. So there is one spelling.
    //
    // These were once written in cmd's dialect for Windows, which is why the
    // shape looks defensive. It has to be POSIX now, and plainly so: cmd's
    // mkdir creates intermediate directories on its own, where a POSIX one
    // needs -p and silently fails on a nested path without it -- after which
    // the "|| cd ." swallowed the failure and the compiler was left to fail
    // on a directory that was never made. "cd /d" is likewise cmd's, and no
    // POSIX shell reads it as anything but a directory named /d.
    static inline const std::string FULL_COMMAND_PATTERN_WITH_CD = R"(cd "%s" && mkdir -p %s && %s)";
    static inline const std::string FULL_COMMAND_PATTERN = R"(mkdir -p %s && %s)";

    std::string getBuildDirectoryName(CompilerName compilerName);

    std::shared_ptr<CompilationDatabase>
    getCompilationDatabase(const fs::path &buildCommandsJsonPath);

    CompilerName getCompilerName(fs::path compilerPath);

    fs::path substituteRemotePathToCompileCommandsJsonPath(const utbot::ProjectContext &projectContext);

    fs::path getClangCompileCommandsJsonPath(const fs::path &buildCommandsJsonPath);

    fs::path removeSharedLibraryVersion(const fs::path &sharedObjectFile);

    std::string getDefaultCompilerForSourceFile(const fs::path& sourceFilePath);

    fs::path getBundledCompilerPath(CompilerName compilerName);

    std::optional<fs::path> getResourceDirectory(const fs::path& buildCompilerPath);

    /**
     * What \p buildCompilerPath compiles for, as it spells it itself.
     *
     * Needed because the clang the server links and the compiler the project
     * is built with are different builds of clang with different defaults. The
     * AST the server reads has to be the one the project's compiler would
     * produce: the target decides which branch of a vendor header is taken,
     * how wide int_fast16_t is, and whether __GNUC__ is defined at all.
     */
    std::optional<std::string> getTargetTriple(const fs::path &buildCompilerPath);

    std::string getIncludePath(const fs::path &includePath);
}

#endif //UNITTESTBOT_COPMILATIONUTILS_H
