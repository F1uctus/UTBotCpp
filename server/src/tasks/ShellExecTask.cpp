#include "ShellExecTask.h"

#include "building/BaseCommand.h"
#include "utils/ExecUtils.h"

#include "loguru.h"

#include <fstream>

namespace utbot {
    ShellExecTask::ExecutionParameters BaseCommand::toExecutionParameters() const {
        auto environment = CollectionUtils::transformTo<std::vector<std::string>>(
            environmentVariables, [](auto const &it) { return it.first + "=" + it.second; });
        auto executable = *(commandLine.begin());
        std::vector <std::string> arguments(std::begin(commandLine), std::end(commandLine));
        return ShellExecTask::ExecutionParameters(executable, arguments, environment);
    }
}

std::string ShellExecTask::toString() const {
    return params.toString();
}

void ShellExecTask::initMessage() const {
    LOG_S(DEBUG) << "Execute: " << params.toString() << "\nfrom directory: " << workDir.string();
}

BaseForkTask::Spawn ShellExecTask::spawnDescription() const {
    return { params.executable, params.argv, params.envp, workDir };
}

std::string ShellExecTask::collectAndCleanup() {
    std::ifstream logFile(logFilePath);
    std::string buf;
    std::stringstream ss;
    while (std::getline(logFile, buf)) {
        LOG_IF_S(DEBUG, logOut) << buf;
        ss << buf << '\n';
    }
    return ss.str();
}

ShellExecTask
ShellExecTask::getShellCommandTask(const std::string &executable,
                                const std::vector<std::string> &arguments,
                                const std::string &fromDir,
                                const std::string &projectName,
                                bool redirectStderr,
                                bool logOut,
                                bool ignoreErrors,
                                const std::optional<std::chrono::seconds> &timeout) {
    ExecutionParameters params(executable, arguments);
    auto task =
        ShellExecTask(params, fromDir, Paths::getExecLogPath(projectName), redirectStderr, logOut, ignoreErrors, timeout);
    return task;
}

ShellExecTask
ShellExecTask::getShellCommandTask(const std::string &executable,
                                const std::vector<std::string> &arguments,
                                const std::optional<std::chrono::seconds> &timeout) {
    ExecutionParameters params(executable, arguments);
    auto task =
        ShellExecTask(params, "",  Paths::getExecLogPath(""), true, false, false, timeout);
    return task;
}

ExecUtils::ExecutionResult
ShellExecTask::runShellCommandTask(const ExecutionParameters &params,
                                const std::string &fromDir,
                                const std::string &projectName,
                                bool redirectStderr,
                                bool logOut,
                                bool ignoreErrors,
                                const std::optional<std::chrono::seconds> &timeout) {
    auto task =
        ShellExecTask(params, fromDir, Paths::getExecLogPath(projectName), redirectStderr, logOut, ignoreErrors, timeout);
    return task.run();
}

ExecUtils::ExecutionResult ShellExecTask::runShellCommandTaskToFile(const ExecutionParameters &params,
                                     const std::string &execLogFile,
                                     const std::string &fromDir,
                                     bool redirectStderr,
                                     bool logOut,
                                     bool ignoreErrors,
                                     const std::optional<std::chrono::seconds> &timeout) {
    auto task = ShellExecTask(params, fromDir, execLogFile, redirectStderr, logOut, ignoreErrors, timeout);
    return task.run();
}

ExecUtils::ExecutionResult ShellExecTask::executeUtbotCommand(const utbot::BaseCommand &command,
                                               const std::string &fromDir, const std::string& projectName) {
    auto task = ShellExecTask(command.toExecutionParameters(), fromDir, Paths::getExecLogPath(projectName), true, true, false, std::nullopt);
    return task.run();
}


std::string ShellExecTask::ExecutionParameters::toString() const {
    std::stringstream ss;
    for (const auto &arg : argv) {
        ss << arg << ' ';
    }
    return ss.str();
}

ShellExecTask::ExecutionParameters::ExecutionParameters(std::string _executable,
                                         std::vector<std::string> _argv,
                                         std::vector<std::string> _envp)
    : executable(std::move(_executable)), envp(std::move(_envp)) {
    if (_argv.empty() || _argv[0] != executable) {
        argv = { executable };
        for (const std::string& arg : _argv) {
            argv.emplace_back(arg);
        }
    } else {
        argv = std::move(_argv);
    }
}


ExecUtils::ExecutionResult ShellExecTask::runPlainShellCommand(const std::string &command,
                 const std::string &fromDir,
                 const std::string &projectName,
                 bool redirectStderr,
                 bool logOut,
                 bool ignoreErrors,
                 const std::optional<std::chrono::seconds> &timeout) {
    ExecutionParameters bashShellParams("/bin/sh", {"-c", command});
    auto task = ShellExecTask(bashShellParams, fromDir, Paths::getExecLogPath(projectName), redirectStderr, logOut, ignoreErrors, timeout);
    return task.run();
}

ShellExecTask::ShellExecTask(ExecutionParameters _params,
                             const std::string &fromDir,
                             const std::string &execLogPath,
                             bool redirectStderr,
                             bool logOut,
                             bool ignoreErrors,
                             const std::optional<std::chrono::seconds> &timeout)
    : BaseForkTask(
          _params.executable, timeout, execLogPath, redirectStderr, ignoreErrors),
      params(std::move(_params)), logOut(logOut) {
    workDir = fromDir.empty() ? fs::current_path() : fs::path(fromDir);
    for (const auto &var : ExecUtils::environAsVector()) {
        params.envp.emplace_back(var);
    }
}
