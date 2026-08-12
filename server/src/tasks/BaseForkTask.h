#ifndef UNITTESTBOT_BASEFORKTASK_H
#define UNITTESTBOT_BASEFORKTASK_H

#include "utils/LogUtils.h"
#include "utils/ExecutionResult.h"

#include <protobuf/testgen.grpc.pb.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

/**
 * Runs one external program with a timeout, capturing its output to a file.
 *
 * The name is historical. On POSIX this forks and execs; Windows has no fork,
 * so there the same description is handed to CreateProcess. That is why the
 * task describes what to run rather than doing the running itself: a forked
 * child can execute arbitrary code before the exec, and on Windows there is no
 * such moment, so anything a subclass wants to arrange has to be expressible up
 * front.
 *
 * The platform halves live in BaseForkTaskPosix.cpp and
 * BaseForkTaskWindows.cpp. Both are compiled -- the sources are picked up by a
 * glob -- and each is guarded so that only one contributes any code.
 */
class BaseForkTask {
public:
    /**
     * Everything needed to start the program.
     *
     * envp entries are "NAME=VALUE". An empty workDir means the current
     * directory.
     */
    struct Spawn {
        std::string executable;
        std::vector<std::string> argv;
        std::vector<std::string> envp;
        fs::path workDir;
    };

    BaseForkTask() = delete;
    virtual ExecUtils::ExecutionResult run();
    /**
     * @brief Sets the output file path. This path will be used for
     * processes communication. The file is deleted on exit code 0
     * and kept if an error happens.
     * @param path - the path of output file.
     */
    void setLogFilePath(fs::path path);
    /**
     * @brief Disables deletion of the output file in all cases.
     * @param retain - the boolean which indicated should output file
     * be retained in all cases.
     */
    void setRetainOutputFile(bool retain);
    /**
     * @brief Checks if the task was interrupted via its exit code.
     * @param exitCode - the task exit code.
     */
    static bool wasInterrupted(int exitCode);

    /**
     * Exit codes used to signal conditions the program itself never
     * reports: a log file that could not be opened, and a run that was
     * stopped rather than finished.
     */
    static const int LOG_FAIL_CODE = 8;
    static const int TIMEOUT_CODE = 9;
protected:
    explicit BaseForkTask(std::string processName,
                          const std::optional<std::chrono::seconds> &timeout,
                          fs::path logFilePath,
                          bool redirectStderr,
                          bool ignoreErrors);
    virtual ~BaseForkTask() = default;
    /*
     * Actions that triggers on child process events.
     * You can use those for logging.
     */
    /**
     * @brief Triggers on timeout of the child process.
     */
    virtual void timeoutMessage() const;
    /**
     * @brief Triggers when child process is waited for.
     */
    virtual void waitMessage() const;
    /**
     * @brief Triggers when an error on opening or using
     * child process output file happens.
     */
    virtual void logFailMessage() const;
    /**
     * @brief Triggers when the child process is killed.
     */
    virtual void killMessage(int status) const;
    /**
     * @brief Triggers on child process creation.
     */
    virtual void initMessage() const;

    /**
     * @brief Reads the output file to std::string.
     */
    virtual std::string collectAndCleanup() = 0;

    /**
     * @brief The program this task runs.
     */
    virtual Spawn spawnDescription() const = 0;

    /**
     * @brief Builds the result, shared by both platforms.
     */
    ExecUtils::ExecutionResult finish(int status);

    /**
     * Name of the binary running in the child process,
     * used for logging.
     */
    std::string processName;
    /**
     * Output file path.
     */
    fs::path logFilePath;
    /**
     * Timeout after which the task is automatically cancelled.
     * Set to std::nullopt if you do not want the process to be
     * cancellable.
     */
    const std::optional<std::chrono::seconds> timeout;
    /**
     * Should the process stderr be redirected to output file.
     */
    bool redirectStderr;
    /**
     * Should UTBot log out nonzero exit codes or not.
     */
    bool ignoreErrors;
    /**
     * Internal value used for setting appropriate exit code.
     */
    bool cancelled = false;
    /**
     * Should output file be retained on exit code 0.
     */
    bool retainOutputFile = false;
};


#endif // UNITTESTBOT_BASEFORKTASK_H
