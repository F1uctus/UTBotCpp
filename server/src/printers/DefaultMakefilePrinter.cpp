#include "DefaultMakefilePrinter.h"

#include "environment/EnvironmentPaths.h"
#include "utils/Copyright.h"

namespace printer {

const std::string DefaultMakefilePrinter::TARGET_ALL = "all";
const std::string DefaultMakefilePrinter::TARGET_BUILD = "build";
const std::string DefaultMakefilePrinter::TARGET_RUN = "run";
const std::string DefaultMakefilePrinter::TARGET_FORCE = ".FORCE";

DefaultMakefilePrinter::DefaultMakefilePrinter() {
    writeCopyrightHeader();
    declareShell();
    declareTarget(TARGET_FORCE, {}, {});
}

void DefaultMakefilePrinter::declareShell() {
#ifdef _WIN32
    // The recipes below are POSIX. Left alone, make on Windows would hand them
    // to cmd.exe, which has no mkdir -p, no mv -f, and no { ...; } grouping.
    //
    // The shipped shell is a busybox copy named sh.exe, and the name is load
    // bearing twice over: busybox picks its applet from argv[0], and make
    // decides from the same name that this is a Unix shell and so quotes
    // recipes and splits command lines the Unix way.
    ss << StringUtils::stringFormat("SHELL = %s\n", Paths::getShell());
    ss << ".SHELLFLAGS = -c\n";
    // Two directories have to be reachable while a recipe runs. The first
    // holds the busybox copies, because mkdir, mv and rm are spelled
    // unqualified in the recipes. The second holds the compiler runtime DLLs:
    // ASan on Windows is dynamic, so a test built with it loads
    // libclang_rt.asan_dynamic at startup and dies at once if it is not on the
    // path. Prepending rather than replacing leaves the user's own tools
    // reachable behind both.
    ss << StringUtils::stringFormat("export PATH := %s;%s;$(PATH)\n",
                                    Paths::getShell().parent_path(),
                                    Paths::getUTBotToolchainDir() / "bin");
#endif
}

void DefaultMakefilePrinter::comment(std::string const &message) {
    ss << StringUtils::stringFormat("# %s\n", message);
}

void DefaultMakefilePrinter::declareVariable(std::string const &name, std::string const &value) {
    ss << StringUtils::stringFormat("%s = %s\n", name, value);
}

void DefaultMakefilePrinter::declareVariableIfNotDefined(std::string const &variableName, std::string const &ifNotDefinedValue) {
    ss << StringUtils::stringFormat("ifndef %s\n", variableName);
    ss << TAB;
    declareVariable(variableName, ifNotDefinedValue);
    ss << "endif\n";
}

void DefaultMakefilePrinter::declareVariableWithPriority(std::string const &variableName,
                                                        std::string const &variablePath) {
    std::string pattern = R"(ifneq ("$(wildcard %s)\", ""))";
    ss << StringUtils::stringFormat(pattern + "\n", variablePath);
    ss << TAB;
    declareVariable(variableName, variablePath);
    ss << "endif\n";
}

void DefaultMakefilePrinter::declareAction(const std::string &name) {
    ss << name << "\n";
}
void DefaultMakefilePrinter::declareInclude(const std::string &otherMakefileName) {
    ss << StringUtils::stringFormat("include %s\n", otherMakefileName);
}

void DefaultMakefilePrinter::writeCopyrightHeader() {
    ss << Copyright::GENERATED_MAKEFILE_HEADER << printer::NL;
}

}
