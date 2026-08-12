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

    /// Where klee/klee.h lives. The generated harnesses include it, so the
    /// path has to be given to the compiler explicitly rather than relied on
    /// to be somewhere the toolchain already looks.
    fs::path getKleeIncludeDir();

    fs::path getUTBotClang();
    
    fs::path getUTBotClangPP();

    fs::path getLLVMnm();

    fs::path getGtestLibPath();

    fs::path getAccessPrivateLibPath();

    fs::path getLLVMprofdata();

    fs::path getLLVMcov();

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
