#include "KleeOptions.h"

#include "loguru.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>

namespace {

/**
 * Options that exist only in UnitTestBot's KLEE fork.
 *
 * Each names a capability rather than a preference, so dropping one changes
 * what the run can find -- which is why they are reported instead of quietly
 * removed:
 *
 *   --utbot                        fork-wide mode switch
 *   --skip-not-lazy-initialized    ) lazy initialisation of symbolic pointers,
 *   --min-number-elements-li       ) which is what lets a function taking a
 *                                  ) pointer be tested without a harness
 *   --skip-not-symbolic-objects    narrows what is reported as an input
 *   --use-advanced-type-system     ) type reconstruction used when rendering
 *   --use-tbaa                     ) pointer-typed values
 *   --fp-runtime                   floating point model
 *   --use-cov-check                coverage-based stopping criterion
 *   --ubsan-runtime                only meaningful with the fork's UBSan model
 */
const std::set<std::string> forkOnlyOptions = {
    "--utbot",
    "--skip-not-lazy-initialized",
    "--min-number-elements-li",
    "--skip-not-symbolic-objects",
    "--use-advanced-type-system",
    "--use-tbaa",
    "--fp-runtime",
    "--use-cov-check",
    "--interactive",
    "--process-number",
    // The fork's own multi-entry-point and per-function timeout mechanism.
    // Upstream has neither; the timeout is passed as --max-time instead, and
    // each entry point is a separate run.
    "--entrypoints-file",
    "--timeout-per-function",
};

/**
 * Options the Windows KLEE does not have because the POSIX runtime is not part
 * of it -- it models a POSIX environment and is POSIX by construction, so that
 * build deliberately excludes it.
 *
 * The value is how many following arguments belong to the option and have to go
 * with it: --sym-stdin takes a size, --sym-files a count and a size. Dropping
 * the flag but leaving its numbers behind would hand KLEE a bare "4096", which
 * it reads as the bitcode file to run.
 *
 * These model stdin and a filesystem for the code under test. A firmware
 * function has neither, so nothing is lost here that the analysis wanted.
 */
const std::map<std::string, unsigned> posixOptions = {
    {"--posix-runtime", 0},
    {"--sym-stdin", 1},
    {"--sym-files", 2},
    {"--sym-arg", 1},
    {"--sym-args", 3},
};

/**
 * The deterministic allocator is spelled differently upstream, where the sizes
 * also carry a unit.
 */
const std::vector<std::pair<std::string, std::string>> renamedOptions = {
    {"--allocate-determ-size", "--kdalloc-heap-size"},
    {"--allocate-determ-start-address", "--kdalloc-heap-start-address"},
    {"--allocate-determ", "--kdalloc"},
};

std::string optionName(const std::string &argument) {
    if (argument.rfind("--", 0) != 0) {
        return {};
    }
    const auto eq = argument.find('=');
    return eq == std::string::npos ? argument : argument.substr(0, eq);
}

} // namespace

bool KleeOptions::targetHasPosixRuntime() {
    // The Windows KLEE excludes it deliberately; this build targets that KLEE.
    return false;
}

bool KleeOptions::targetHasUnitTestBotExtensions() {
    // This build is compiled against upstream-derived KLEE headers. If the
    // server is ever pointed back at the fork, this is the single place that
    // has to learn about it.
    return false;
}

std::vector<std::string> KleeOptions::adapt(const std::vector<std::string> &argv) {
    if (targetHasUnitTestBotExtensions()) {
        return argv;
    }

    static std::unordered_set<std::string> alreadyReported;

    std::vector<std::string> adapted;
    adapted.reserve(argv.size());

    for (size_t i = 0; i < argv.size(); i++) {
        const std::string &argument = argv[i];
        if (i == 0) {
            adapted.push_back(argument);
            continue;
        }

        const std::string name = optionName(argument);
        if (name.empty()) {
            adapted.push_back(argument);
            continue;
        }

        if (forkOnlyOptions.count(name)) {
            if (alreadyReported.insert(name).second) {
                LOG_S(WARNING)
                    << name << " is not available in this KLEE and is being "
                    << "dropped; results will be narrower than the fork's.";
            }
            continue;
        }

        auto posix = posixOptions.find(name);
        if (posix != posixOptions.end()) {
            if (alreadyReported.insert(name).second) {
                LOG_S(WARNING)
                    << name << " needs the POSIX runtime, which this KLEE does "
                    << "not build; dropping it and its arguments.";
            }
            // Only skip the values when they are separate arguments;
            // --sym-stdin=4096 carries its own.
            if (argument == name) {
                i += posix->second;
            }
            continue;
        }

        auto renamed = std::find_if(
            renamedOptions.begin(), renamedOptions.end(),
            [&name](const auto &pair) { return pair.first == name; });
        if (renamed != renamedOptions.end()) {
            std::string value = argument.size() > name.size()
                                    ? argument.substr(name.size())
                                    : std::string{};
            adapted.push_back(renamed->second + value);
            continue;
        }

        adapted.push_back(argument);
    }

    return adapted;
}
