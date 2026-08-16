#ifndef UNITTESTBOT_ENVIRONMENT_PATHS_H
#define UNITTESTBOT_ENVIRONMENT_PATHS_H

#include "utils/path/FileSystemPath.h"

namespace Paths {
    fs::path getUTBotRootDir();
    
    fs::path getUTBotInstallDir();

    fs::path getUTBotDebsInstallDir();

    fs::path getPython();

    fs::path getBear();

    fs::path getCMake();

    fs::path getMake();

    /// The shell the generated makefiles are run under. Their recipes are
    /// POSIX, so on Windows this is the bundled busybox rather than cmd.exe.
    fs::path getShell();

    /// Renames main out of the way when a project's own executable is relinked
    /// as a library for the test binary to call into.
    fs::path getObjcopy();

    /// Where klee/klee.h lives. The generated harnesses include it, so the
    /// path has to be given to the compiler explicitly rather than relied on
    /// to be somewhere the toolchain already looks.
    fs::path getKleeIncludeDir();

    /// KLEE is shipped in the install tree and must not be resolved through
    /// the launching user's PATH.
    fs::path getKlee();

    fs::path getUTBotClang();
    
    fs::path getUTBotClangPP();

    fs::path getLLVMnm();

    fs::path getGtestLibPath();

    fs::path getAccessPrivateLibPath();

    fs::path getLLVMprofdata();

    fs::path getLLVMcov();

    /// The bitcode linker. Replaces ld.gold with the LLVMgold plugin, which
    /// exists only on Linux and only when binutils was built with plugin
    /// support -- llvm-link is part of LLVM itself and works everywhere.
    fs::path getLLVMLink();

    fs::path getLLVMgold();

    fs::path getAr();

    fs::path getLdGold();

    fs::path getLd();

    fs::path getAsanLibraryPath();

  // Gcc is used only in tests
    fs::path getGcc();
    fs::path getGpp();
}

#endif //UNITTESTBOT_ENVIRONMENT_PATHS_H
