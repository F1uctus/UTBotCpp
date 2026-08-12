#include "KleeOptions.h"

#include "loguru.h"

#include <algorithm>
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
