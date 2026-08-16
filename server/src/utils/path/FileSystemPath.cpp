#include "FileSystemPath.h"
#include "utils/StringUtils.h"

namespace fs {
    path findInPATH(const path &p) {
        if (is_absolute(p)) {
            return p;
        }
        const char *pathVariable = std::getenv("PATH");
        if (pathVariable == nullptr) {
            return p;
        }
#ifdef _WIN32
        constexpr char pathSeparator = ';';
#else
        constexpr char pathSeparator = ':';
#endif
        std::vector<std::string> pathENV = StringUtils::split(pathVariable, pathSeparator);
        for (const std::string &pathFind: pathENV) {
            path fullPath = path(pathFind) / p;
            if (exists(fullPath)) {
                return fullPath;
            }
        }
        return p;
    }
}
