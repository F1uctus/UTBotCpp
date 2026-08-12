#include "ExecUtils.h"

#include <cstdlib>

namespace ExecUtils {
    void throwIfCancelled() {
        auto context = RequestEnvironment::getServerContext();
        if (context && context->IsCancelled()) {
            LOG_S(ERROR) << "Cancel";
            throw CancellationException();
        }
    }

    void toCArgumentsPtr(std::vector<std::string> &argv,
                         std::vector<std::string> &envp,
                         std::vector<char *> &cargv,
                         std::vector<char *> &cenvp,
                         bool appendNull) {
        for (auto &s : argv) {
            cargv.emplace_back((char*)s.data());
        }
        for (auto &s : envp) {
            cenvp.emplace_back((char*)s.data());
        }
        if (appendNull) {
            cargv.emplace_back(nullptr);
        }
        cenvp.emplace_back(nullptr);
    }

    std::vector<std::string> environAsVector() {
        static std::vector<std::string> res;
        if (res.empty()) {
            // The MSVC runtime spells the process environment _environ; environ
            // is the POSIX name and is not declared there.
#ifdef _WIN32
            char **env = _environ;
#else
            char **env = environ;
#endif
            for (int i = 0; env[i]; i++) {
                res.emplace_back(env[i]);
            }
        }
        return res;
    }
}
