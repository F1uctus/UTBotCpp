#include "environment/EnvironmentPaths.h"

#ifdef _WIN32
#include <windows.h>

#include <system_error>
#include <vector>
#endif

namespace Paths {
    fs::path getExecutablePath()
    {
#ifdef _WIN32
        std::vector<wchar_t> pathBuffer(MAX_PATH);
        while (true) {
            const DWORD length = GetModuleFileNameW(
                    nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
            if (length == 0) {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                        "GetModuleFileNameW");
            }
            if (length < pathBuffer.size()) {
                return fs::path(std::wstring(pathBuffer.data(), length));
            }
            pathBuffer.resize(pathBuffer.size() * 2);
        }
#else
        return fs::canonical("/proc/self/exe");
#endif
    }

    static bool isDevEnvironment() {
        return fs::exists(UTBOT_DEV_ROOT_DIR);
    }

    fs::path getUTBotRootDir() {
        if (isDevEnvironment()) {
            return UTBOT_DEV_ROOT_DIR;
        }
        return getExecutablePath().parent_path().parent_path();
    }

    fs::path getUTBotInstallDir() {
        return getUTBotRootDir() / "install";
    }

    fs::path getKleeIncludeDir() {
        return getUTBotInstallDir() / "include";
    }

    fs::path getKlee() {
        return getUTBotInstallDir() / "bin" / "klee";
    }

    fs::path getUTBotDebsInstallDir() {
        if (isDevEnvironment()) {
            return fs::current_path().root_path();
        }
        return getUTBotRootDir() / "debs-install";
    }

    fs::path getPython() {
        return getUTBotDebsInstallDir() / "usr" / "bin" / "python";
    }


    fs::path getBear() {
        return getUTBotRootDir() / "bear" / "bin" / "bear";
    }

    fs::path getCMake() {
        return getUTBotInstallDir() / "bin" / "cmake";
    }

    fs::path getMake() {
        return "make";
    }

    fs::path getUTBotClang() {
        return getUTBotInstallDir() / "bin" / "clang";
    }

    fs::path getUTBotClangPP() {
        return getUTBotInstallDir() / "bin" / "clang++";
    }

    fs::path getGcc() {
        return "gcc";
    }

    fs::path getGpp() {
        return "g++";
    }

    fs::path getLLVMnm() {
        return getUTBotInstallDir() / "bin" / "llvm-nm";
    }

    fs::path getGtestLibPath() {
        return getUTBotRootDir() / "gtest";
    }

    fs::path getAccessPrivateLibPath() {
        return getUTBotRootDir() / "access_private" / "include";
    }

    fs::path getLLVMprofdata() {
        return getUTBotInstallDir() / "bin" / "llvm-profdata";
    }

    fs::path getLLVMcov() {
        return getUTBotInstallDir() / "bin" / "llvm-cov";
    }

    fs::path getLLVMgold() {
        return getUTBotInstallDir() / "lib" / "LLVMgold.so";
    }

    fs::path getLLVMLink() {
        return getUTBotInstallDir() / "bin" / "llvm-link";
    }

    fs::path getAr() {
        // llvm-ar, not binutils ar: it reads and indexes bitcode members
        // natively, where binutils needs the LLVMgold plugin to do it at all.
        return getUTBotInstallDir() / "bin" / "llvm-ar";
    }

    fs::path getLdGold() {
        return getUTBotDebsInstallDir() / "usr" / "bin" / "ld.gold";
    }

    fs::path getLd() {
        return getUTBotDebsInstallDir() / "usr" / "bin" / "ld";
    }

    fs::path getAsanLibraryPath() {
        return Paths::getUTBotDebsInstallDir() / "usr" / "lib" / "gcc" / "x86_64-linux-gnu" / "9" / "libasan.so";
    }
}
