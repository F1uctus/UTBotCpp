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

    fs::path getUTBotToolchainDir() {
        return getUTBotRootDir() / "toolchain";
    }

    namespace {
        /**
         * A tool from the toolchain that compiles the user's code.
         *
         * On Windows that is a separate tree from the install prefix, and the
         * separation is not tidiness. The clang in a clang+llvm-*-windows-msvc
         * release targets the MSVC ABI, and its headers and import libraries
         * belong to Visual Studio, which LLVM cannot ship -- so a machine
         * without Visual Studio cannot compile so much as #include <stdio.h>
         * with it, and the distribution would not be portable at all.
         *
         * llvm-mingw is the same LLVM 22.1.8, built for the mingw-w64 target
         * and carrying its own CRT, libc++ and headers. Same version matters:
         * KLEE reads bitcode from its own LLVM or older, and it is that LLVM.
         *
         * clang finds its sysroot relative to its own location, which is why
         * the tree is kept whole rather than having its binaries copied in
         * beside KLEE's, and why the driver keeps its target prefix -- clang
         * reads the target it defaults to out of its own argv[0].
         */
        fs::path userToolchainTool(const std::string &name) {
#ifdef _WIN32
            return getUTBotToolchainDir() / "bin" / ("x86_64-w64-mingw32-" + name);
#else
            return getUTBotInstallDir() / "bin" / name;
#endif
        }

        fs::path userToolchainUtility(const std::string &name) {
#ifdef _WIN32
            return getUTBotToolchainDir() / "bin" / name;
#else
            return getUTBotInstallDir() / "bin" / name;
#endif
        }
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

    fs::path getNinja() {
        return getUTBotInstallDir() / "bin" / "ninja";
    }

    fs::path getMake() {
#ifdef _WIN32
        // Windows has no make of its own, so the distribution carries one and
        // the generated makefiles are driven by that rather than by whatever
        // the user happens to have on PATH.
        return getUTBotRootDir() / "bin" / "make";
#else
        return "make";
#endif
    }

    fs::path getShell() {
#ifdef _WIN32
        // The recipes are written for a POSIX shell -- mkdir -p, mv -f, rm -rf,
        // "cmd && { cmd; exit $?; }". The distribution carries a busybox that
        // dispatches on argv[0], so the copy named sh.exe is a shell; make
        // recognises the name and quotes recipes the Unix way because of it.
        return getUTBotRootDir() / "bin" / "sh.exe";
#else
        return "/bin/sh";
#endif
    }

    fs::path getUTBotClang() {
        return userToolchainTool("clang");
    }

    fs::path getUTBotClangPP() {
        return userToolchainTool("clang++");
    }

    fs::path getGcc() {
        return "gcc";
    }

    fs::path getGpp() {
        return "g++";
    }

    fs::path getLLVMnm() {
        return userToolchainUtility("llvm-nm");
    }

    fs::path getGtestLibPath() {
        return getUTBotRootDir() / "gtest";
    }

    fs::path getAccessPrivateLibPath() {
        return getUTBotRootDir() / "access_private" / "include";
    }

    fs::path getLLVMprofdata() {
        return userToolchainUtility("llvm-profdata");
    }

    fs::path getLLVMcov() {
        return userToolchainUtility("llvm-cov");
    }

    fs::path getLLVMgold() {
        return getUTBotInstallDir() / "lib" / "LLVMgold.so";
    }

    fs::path getLLVMLink() {
        return getUTBotInstallDir() / "bin" / "llvm-link";
    }

    fs::path getObjcopy() {
        // llvm-objcopy rather than binutils objcopy: it is part of the LLVM the
        // distribution already carries, so it needs nothing installed and
        // behaves the same on both platforms.
        return userToolchainUtility("llvm-objcopy");
    }

    fs::path getAr() {
        // llvm-ar, not binutils ar: it reads and indexes bitcode members
        // natively, where binutils needs the LLVMgold plugin to do it at all.
        return userToolchainUtility("llvm-ar");
    }

    fs::path getLdGold() {
#ifdef _WIN32
        return userToolchainUtility("ld.lld");
#else
        return getUTBotDebsInstallDir() / "usr" / "bin" / "ld.gold";
#endif
    }

    fs::path getLd() {
        // Relinking a project's executable as a library goes through the
        // linker directly rather than the compiler driver. There is no
        // system ld on Windows to reach for, and ld.lld speaks the GNU
        // linker's options, which is what the flags are translated into.
#ifdef _WIN32
        return userToolchainUtility("ld.lld");
#else
        return getUTBotDebsInstallDir() / "usr" / "bin" / "ld";
#endif
    }

    fs::path getAsanLibraryPath() {
        return Paths::getUTBotDebsInstallDir() / "usr" / "lib" / "gcc" / "x86_64-linux-gnu" / "9" / "libasan.so";
    }
}
