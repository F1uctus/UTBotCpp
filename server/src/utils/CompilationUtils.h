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

    // A build recipe runs through the platform's shell, and the two do not
    // share a spelling for any of this.
    //
    // cmd has no "mkdir -p", but its mkdir already creates intermediate
    // directories; what it lacks is tolerance of the directory existing, hence
    // swallowing the failure. "cd ." is the no-op that makes the || succeed --
    // cmd has no "true". "cd /d" is needed because a bare cd will not cross
    // drives, and the build tree and the sources often sit on different ones.
#ifdef _WIN32
    static inline const std::string FULL_COMMAND_PATTERN_WITH_CD =
            R"(cd /d "%s" && (mkdir "%s" 2>nul || cd .) && %s)";
    static inline const std::string FULL_COMMAND_PATTERN =
            R"((mkdir "%s" 2>nul || cd .) && %s)";
#else
    static inline const std::string FULL_COMMAND_PATTERN_WITH_CD = R"(cd "%s" && mkdir -p %s && %s)";
    static inline const std::string FULL_COMMAND_PATTERN = R"(mkdir -p %s && %s)";
#endif

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

    std::string getIncludePath(const fs::path &includePath);
}

#endif //UNITTESTBOT_COPMILATIONUTILS_H
