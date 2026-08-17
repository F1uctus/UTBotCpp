#include "CoverageTool.h"

#include "GcovCoverageTool.h"
#include "LlvmCoverageTool.h"
#include "exceptions/CoverageGenerationException.h"
#include "utils/CompilationUtils.h"
#include "utils/StringUtils.h"

using namespace CompilationUtils;

CoverageTool::CoverageTool(utbot::ProjectContext projectContext, ProgressWriter const *progressWriter) :
        projectContext(std::move(projectContext)), progressWriter(progressWriter) {
}

std::unique_ptr<CoverageTool> getCoverageTool(const std::string &compileCommandsJsonPath,
                                              utbot::ProjectContext projectContext,
                                              ProgressWriter const *progressWriter) {
    auto compilationDatabase = CompilationUtils::getCompilationDatabase(compileCommandsJsonPath);
    fs::path compilerPath = compilationDatabase->getBuildCompilerPath();
    CompilerName compilerName = CompilationUtils::getCompilerName(compilerPath);
    switch (compilerName) {
    case CompilerName::GCC:
    case CompilerName::GXX:
        return std::make_unique<GcovCoverageTool>(projectContext, progressWriter);
    case CompilerName::CLANG:
    case CompilerName::CLANGXX:
        return std::make_unique<LlvmCoverageTool>(projectContext, progressWriter);
    default: {
        std::string message = "Coverage tool for your compiler is not implemented";
        LOG_S(ERROR) << message;
        throw CoverageGenerationException(message);
    }
    }
}

/// One make variable per flag, rather than one variable holding both.
///
/// A value with a space in it does not survive the trip. make escapes the
/// space when it writes the assignment into MAKEFLAGS for the sub-make that
/// the generated makefiles invoke, and the escape it uses is a backslash --
/// which the bundled busybox rewrites to a forward slash on the way through,
/// because that is what it does to backslashes in arguments on Windows. The
/// escape gone, the sub-make reads the value as ending at the space, so the
/// second flag arrives as a stray word and the first keeps the slash. The
/// result was a filter that matched no test and no --gtest_output at all, so
/// no results file was written and every test was reported as having died.
///
/// Each flag stays quoted: --gtest_filter carries a '*' that the shell would
/// otherwise try to glob.
std::vector<std::pair<std::string, std::string>>
CoverageTool::getGTestFlags(const UnitTest &unitTest) const {
    return {
        { "GTEST_FILTER_FLAG",
          StringUtils::stringFormat("\"--gtest_filter=*.%s\"", unitTest.testname) },
        { "GTEST_OUTPUT_FLAG",
          StringUtils::stringFormat("\"--gtest_output=json:%s\"",
                                    Paths::getGTestResultsJsonPath(projectContext)) },
    };
}
