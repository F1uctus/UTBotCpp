#include "MakefileUtils.h"

#include "CLIUtils.h"
#include "ExecUtils.h"
#include "LogUtils.h"
#include "Paths.h"
#include "StringUtils.h"
#include "commands/Commands.h"
#include "environment/EnvironmentPaths.h"
#include "exceptions/ExecutionProcessException.h"

#include "loguru.h"

#include <fstream>
#include <thread>

namespace MakefileUtils {
    std::vector<std::string> getMakeCommand(std::string makefile, std::string target, bool nested) {
        std::vector<std::string> command;
        if (nested) {
            command.emplace_back("$(MAKE)");
        } else {
            command.emplace_back(Paths::getMake());
            command.emplace_back(threadFlag());
            command.emplace_back("-s");
        }
        command.emplace_back("-f");
        command.emplace_back(makefile);
        command.emplace_back(target);
        return command;
    }

    MakefileCommand::MakefileCommand(const utbot::ProjectContext &projectContext,
                                     fs::path makefile,
                                     std::string target,
                                     const std::vector<std::pair<std::string, std::string>> &gtestFlags,
                                     std::vector<std::string> env)
            : makefile(std::move(makefile)), target(std::move(target)),
              projectName(projectContext.projectName) {
        this->makefile = this->makefile.lexically_normal();
        this->makefile = this->makefile.lexically_normal();
        fs::path logDir = Paths::getLogDir(projectContext.projectName);
        logFile = logDir / "makefile.log";
        fs::create_directories(logDir);
        std::vector<std::string> argv = std::move(env);
        for (const auto &[variable, value] : gtestFlags) {
            argv.emplace_back(variable + "=" + value);
        }
        std::vector<std::string> makeCommand = getMakeCommand(this->makefile, this->target, false);
        argv.insert(argv.begin(), makeCommand.begin(), makeCommand.end());
        // No "env" in front. The variables sit after the make command, so they
        // were never env's to set -- make reads a trailing NAME=VALUE as one of
        // its own overrides. The wrapper did nothing but require a program that
        // does not exist on Windows.
        runCommand = ShellExecTask::ExecutionParameters(makeCommand.front(), argv);
        printCommand = ShellExecTask::ExecutionParameters(makeCommand.front(), argv);
        printCommand.argv.emplace_back("-n");
    }

    ExecUtils::ExecutionResult
    MakefileCommand::run(const fs::path &buildPath,
                         bool redirectStderr,
                         bool ignoreErrors,
                         const std::optional<std::chrono::seconds> &timeout) const {
        auto print = ShellExecTask::runShellCommandTaskToFile(printCommand, logFile, buildPath);
        if (print.status != 0) {
            failedCommand = &printCommand;
            return print;
        }
        // Separate one run from the next in the log. Appending a newline does
        // not need a process, and spawning "echo" for it needed one that
        // Windows has only as a shell builtin.
        std::ofstream(logFile, std::ios::app) << '\n';
        auto exec = ShellExecTask::runShellCommandTask(
                runCommand, buildPath, projectName, redirectStderr, false, ignoreErrors, timeout);
        if (exec.status != 0) {
            failedCommand = &runCommand;
        }
        return exec;
    }

    std::string MakefileCommand::getFailedCommand() const {
        if (failedCommand) {
            return failedCommand->toString();
        } else {
            LOG_S(ERROR) << "No command failed";
            return "";
        }
    }

    std::string threadFlag() {
        if (Commands::threadsPerUser != 0) {
            return "-j" + std::to_string(Commands::threadsPerUser);
        }
        unsigned int threads = std::thread::hardware_concurrency();
        if (threads == 0) {
            return "";
        } else {
            return "-j" + std::to_string(threads);
        }
    }
}
