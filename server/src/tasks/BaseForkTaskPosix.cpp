/**
 * The POSIX half of BaseForkTask: fork, redirect, exec, and wait.
 *
 * Guarded as a whole because the sources are picked up by a glob, so this file
 * is compiled on Windows too and has to contribute nothing there.
 */
#ifndef _WIN32

#include "BaseForkTask.h"
#include "RequestEnvironment.h"
#include "exceptions/BaseException.h"
#include "utils/ExecUtils.h"
#include "utils/StringFormat.h"

#include "loguru.h"

#include <grpc/impl/codegen/fork.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <thread>

namespace {
    class NoSuchProcessException : public std::exception {
        std::string errnoMessage;

    public:
        explicit NoSuchProcessException(std::string errnoMessage)
                : errnoMessage(std::move(errnoMessage)) {
        }

        [[nodiscard]] const char *what() const noexcept override {
            return errnoMessage.c_str();
        }
    };

    const int SETPGID_FAIL_CODE = 10;

    /**
     * Signals sent in order when the process has to go.
     *
     * A single entry: the process being run is an external tool, not something
     * that has cleanup of its own to do, and KLEE -- the one that does -- is
     * given its budget as --max-time so that it stops on its own terms long
     * before this is reached.
     */
    const std::vector<int> shutDownSignals = { SIGKILL };
}

/**
 * @brief Redirects child stdout (and, optionally, stderr) to the output file.
 */
static bool redirectOutput(const fs::path &logFilePath, const std::string &processName,
                           bool redirectStderr) {
    fs::create_directories(logFilePath.parent_path());
    int fd = open(logFilePath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    bool ok = true;
    if (fd == -1) {
        ok = false;
        LOG_S(ERROR) << "Failed to create temporary logging file for " << processName << ": "
                     << LogUtils::errnoMessage() << "(" << logFilePath << ")";
    }
    if (dup2(fd, 1) == -1 || (redirectStderr ? (dup2(fd, 2) == -1) : false)) {
        ok = false;
        LOG_S(ERROR) << "Failed to create descriptors for child process: "
                     << LogUtils::errnoMessage();
    }
    close(fd);
    return ok;
}

static void checkForExist(pid_t pid) {
    int existStatus = kill(pid, 0);
    if (existStatus == -1 && errno == ESRCH) {
        LOG_S(DEBUG) << "Child process was killed";
    } else {
        LOG_S(WARNING) << "Child process was not killed";
    }
}

static void throwIfNoSuchProcess() {
    if (errno == ESRCH) {
        LOG_S(ERROR) << "NO such process";
        throw NoSuchProcessException(strerror(errno));
    }
}

/**
 * @brief wait on pid and then proceed to kill it on timeout
 * @return status of the finished process.
 */
static int waitForFinishedOrCancelled(pid_t pid, const std::string &processName,
                                      const std::optional<std::chrono::seconds> &timeout,
                                      bool &cancelled,
                                      const std::function<void()> &timeoutMessage,
                                      const std::function<void()> &waitMessage,
                                      const std::function<void()> &logFailMessage,
                                      const std::function<void(int)> &killMessage) {
    try {
        int status = 0;
        size_t signalId = 0;
        int spent_ms = 0;
        bool sendSignals = false;
        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (timeout.has_value()) {
                auto now = std::chrono::steady_clock::now();
                if ((now - start) > timeout.value()) {
                    timeoutMessage();
                    sendSignals = true;
                }
            }
            if (RequestEnvironment::isCancelled()) {
                LOG_S(DEBUG) << "Stopping " << processName << " as cancellation was received";
                sendSignals = true;
            }
            if (sendSignals) {
                if (signalId == shutDownSignals.size()) {
                    LOG_S(WARNING) << "Process was not killed";
                    break;
                }
                int signal = shutDownSignals[signalId];
                throwIfNoSuchProcess();
                LOG_S(DEBUG) << "Sending signal to " << processName << ": " << signal;

                pid_t pgid = getpgid(pid);
                std::string pkillCommand = StringUtils::stringFormat("pkill -g %d -%d", pgid, signal);
                int killChildStatus = std::system(pkillCommand.c_str());
                if (killChildStatus == -1 || !WIFEXITED(killChildStatus) || WEXITSTATUS(killChildStatus) > 1) {
                    LOG_S(DEBUG) << "Failed to send signal to " << processName << ": "
                                 << LogUtils::errnoMessage();
                    continue; // Trying next level of shutdown
                }
                LOG_S(DEBUG) << "Successfully sent signal to " << processName;
                signalId++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED);
            if (result == 0) {
                spent_ms++;
                if (spent_ms == 1000) {
                    spent_ms = 0;
                    waitMessage();
                }
            } else if (result == pid && WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                if (exitStatus == BaseForkTask::LOG_FAIL_CODE) {
                    logFailMessage();
                }
                cancelled &= sendSignals;
                return exitStatus;
            } else if (WIFSIGNALED(status)) {
                killMessage(status);
                cancelled &= sendSignals;
                return status;
            } else if (WIFSTOPPED(status)) {
                sendSignals = true;
                continue;
            } else {
                LOG_S(WARNING) << "Received undefined status: " << status << ". Ignoring that.";
                continue;
            }
        }
        checkForExist(pid);
        return -1;
    } catch (NoSuchProcessException const &e) {
        return -1;
    }
}

ExecUtils::ExecutionResult BaseForkTask::run() {
    const Spawn spawn = spawnDescription();
    pid_t pid;

    grpc_prefork();
    switch (pid = fork()) {
        case -1: {
            auto message = processName + " fork failed.";
            LOG_S(ERROR) << message << LogUtils::errnoMessage();
            throw BaseException(message);
        }
        case 0: {
            grpc_postfork_child();
            int pgidStatus = setpgid(pid, pid);
            // This is child process
            if (!redirectOutput(logFilePath, processName, redirectStderr)) {
                exit(LOG_FAIL_CODE);
            }
            if (pgidStatus != 0) {
                std::string message = StringUtils::stringFormat("Failed setpgid(pid = %d, pgid = %d)");
                LOG_S(ERROR) << message << LogUtils::errnoMessage();
                exit(SETPGID_FAIL_CODE);
            }
            std::vector<char *> cargv, cenvp;
            std::vector<std::string> tmp = spawn.argv;
            ExecUtils::toCArgumentsPtr(tmp, const_cast<std::vector<std::string> &>(spawn.envp),
                                       cargv, cenvp, true);
            if (chdir(spawn.workDir.string().c_str()) != 0) {
                // Written to cerr rather than logged: this is the child, and
                // collectAndCleanup indents whatever the log file holds.
                std::cerr << "Failed to change working directory: " << LogUtils::errnoMessage()
                          << " " << spawn.workDir << '\n';
                exit(-1);
            }
            execvpe(spawn.executable.c_str(), cargv.data(), cenvp.data());
            exit(-1);
        }
        default: {
            grpc_postfork_parent();
            // This is parent process
            LOG_S(DEBUG) << "Running " << processName << " out of process from pid: " << getpid();
            initMessage();
            int status = waitForFinishedOrCancelled(
                    pid, processName, timeout, cancelled,
                    [this] { timeoutMessage(); },
                    [this] { waitMessage(); },
                    [this] { logFailMessage(); },
                    [this](int s) { killMessage(s); });
            return finish(status);
        }
    }
}

#endif // !_WIN32
