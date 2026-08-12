/**
 * The Windows half of BaseForkTask.
 *
 * Windows has no fork, so there is no child context in which to redirect
 * descriptors and exec: everything the POSIX side does between the fork and the
 * exec has to be expressed as arguments to a single CreateProcess call. That is
 * why the redirection is set up in the parent, on inheritable handles, and why
 * the working directory is a parameter rather than a chdir.
 *
 * Killing is done through a job object rather than by process id. The programs
 * this runs -- a build, KLEE -- start children of their own, and terminating
 * only the process that was started leaves those running; the POSIX side has
 * the same requirement and meets it with a process group.
 *
 * Guarded as a whole because the sources are picked up by a glob, so this file
 * is compiled on POSIX too and has to contribute nothing there.
 */
#ifdef _WIN32

#include "BaseForkTask.h"
#include "RequestEnvironment.h"
#include "exceptions/BaseException.h"

#include "loguru.h"

#include <windows.h>

#include <thread>

namespace {
    /**
     * Quotes one argument the way the Windows CRT parses it back.
     *
     * CreateProcess takes a single command line, not a vector, so the argument
     * boundaries have to be re-encoded. The rule that makes this non-obvious is
     * that a backslash is only an escape when a quote eventually follows it, so
     * the run of backslashes before a quote is doubled while any other run is
     * left alone -- otherwise a path ending in a separator would swallow the
     * closing quote.
     */
    std::string quoteArgument(const std::string &argument) {
        if (!argument.empty() &&
            argument.find_first_of(" \t\n\v\"") == std::string::npos) {
            return argument;
        }
        std::string quoted = "\"";
        for (auto it = argument.begin();; ++it) {
            unsigned backslashes = 0;
            while (it != argument.end() && *it == '\\') {
                ++it;
                ++backslashes;
            }
            if (it == argument.end()) {
                quoted.append(backslashes * 2, '\\');
                break;
            }
            if (*it == '"') {
                quoted.append(backslashes * 2 + 1, '\\');
            } else {
                quoted.append(backslashes, '\\');
            }
            quoted.push_back(*it);
        }
        quoted.push_back('"');
        return quoted;
    }

    std::string buildCommandLine(const std::vector<std::string> &argv) {
        std::string commandLine;
        for (const auto &argument : argv) {
            if (!commandLine.empty()) {
                commandLine.push_back(' ');
            }
            commandLine += quoteArgument(argument);
        }
        return commandLine;
    }

    /**
     * The environment block: NAME=VALUE separated by NULs and terminated by an
     * extra one. An empty vector yields an empty block, which is a valid
     * environment with no variables rather than "inherit", so the caller has to
     * pass everything it wants the program to see.
     */
    std::string buildEnvironmentBlock(const std::vector<std::string> &envp) {
        std::string block;
        for (const auto &variable : envp) {
            block += variable;
            block.push_back('\0');
        }
        block.push_back('\0');
        return block;
    }

    std::string lastErrorMessage() {
        const DWORD error = GetLastError();
        char *buffer = nullptr;
        const DWORD length = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, error, 0, reinterpret_cast<char *>(&buffer), 0, nullptr);
        std::string message = length && buffer ? std::string(buffer, length) : std::string();
        if (buffer) {
            LocalFree(buffer);
        }
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        return "(" + std::to_string(error) + ") " + message;
    }

    /** Closes a handle unless it is already empty, so cleanup paths stay short. */
    struct HandleCloser {
        HANDLE handle = nullptr;
        ~HandleCloser() {
            if (handle && handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
        }
    };
}

ExecUtils::ExecutionResult BaseForkTask::run() {
    const Spawn spawn = spawnDescription();

    fs::create_directories(logFilePath.parent_path());

    SECURITY_ATTRIBUTES inheritable{};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;

    HandleCloser logFile{ CreateFileA(logFilePath.string().c_str(), GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };
    if (logFile.handle == INVALID_HANDLE_VALUE) {
        LOG_S(ERROR) << "Failed to create temporary logging file for " << processName << ": "
                     << lastErrorMessage() << " (" << logFilePath << ")";
        logFailMessage();
        return finish(LOG_FAIL_CODE);
    }

    // A job the process is killed through, so that anything it starts goes with
    // it. Without JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE a build left half-running
    // would outlive the request that asked for it.
    HandleCloser job{ CreateJobObjectA(nullptr, nullptr) };
    if (job.handle) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job.handle, JobObjectExtendedLimitInformation, &limits,
                                sizeof(limits));
    }

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = logFile.handle;
    startup.hStdError = redirectStderr ? logFile.handle : GetStdHandle(STD_ERROR_HANDLE);

    std::vector<std::string> argv = spawn.argv;
    if (argv.empty()) {
        argv.push_back(spawn.executable);
    }
    std::string commandLine = buildCommandLine(argv);
    std::string environment = buildEnvironmentBlock(spawn.envp);
    const std::string workDir =
            spawn.workDir.empty() ? fs::current_path().string() : spawn.workDir.string();

    PROCESS_INFORMATION process{};
    // lpApplicationName is left null so that CreateProcess searches PATH the
    // way execvp does; the executable is already the first token of the command
    // line, quoted.
    const BOOL started = CreateProcessA(
            nullptr, commandLine.data(), nullptr, nullptr, /*bInheritHandles=*/TRUE,
            CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP,
            environment.empty() ? nullptr : environment.data(), workDir.c_str(), &startup,
            &process);
    if (!started) {
        auto message = processName + " could not be started: " + lastErrorMessage();
        LOG_S(ERROR) << message;
        throw BaseException(message);
    }

    HandleCloser processHandle{ process.hProcess };
    HandleCloser threadHandle{ process.hThread };

    // Assigned while suspended, so nothing it starts can escape the job.
    if (job.handle) {
        AssignProcessToJobObject(job.handle, process.hProcess);
    }
    ResumeThread(process.hThread);

    LOG_S(DEBUG) << "Running " << processName << " out of process from pid: "
                 << GetCurrentProcessId();
    initMessage();

    int status = 0;
    bool sendSignals = false;
    const auto start = std::chrono::steady_clock::now();
    int spent_ms = 0;
    while (true) {
        if (timeout.has_value() && (std::chrono::steady_clock::now() - start) > timeout.value()) {
            timeoutMessage();
            sendSignals = true;
        }
        if (RequestEnvironment::isCancelled()) {
            LOG_S(DEBUG) << "Stopping " << processName << " as cancellation was received";
            sendSignals = true;
        }
        if (sendSignals) {
            // There is no gentler step to escalate through: Windows has no
            // signal that reliably reaches a non-console child, so the job is
            // terminated outright. KLEE is stopped by its own --max-time long
            // before this, which is what makes it acceptable.
            LOG_S(DEBUG) << "Terminating " << processName;
            if (job.handle) {
                TerminateJobObject(job.handle, TIMEOUT_CODE);
            } else {
                TerminateProcess(process.hProcess, TIMEOUT_CODE);
            }
            cancelled = true;
        }

        const DWORD waited = WaitForSingleObject(process.hProcess, 1);
        if (waited == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            GetExitCodeProcess(process.hProcess, &exitCode);
            status = static_cast<int>(exitCode);
            if (status == LOG_FAIL_CODE) {
                logFailMessage();
            }
            if (cancelled) {
                killMessage(status);
            }
            break;
        }
        if (waited != WAIT_TIMEOUT) {
            LOG_S(WARNING) << "Wait on " << processName << " failed: " << lastErrorMessage();
            status = -1;
            break;
        }
        spent_ms++;
        if (spent_ms == 1000) {
            spent_ms = 0;
            waitMessage();
        }
    }

    // Closed before the output is read, so the last of it has been flushed.
    CloseHandle(logFile.handle);
    logFile.handle = nullptr;

    return finish(status);
}

#endif // _WIN32
