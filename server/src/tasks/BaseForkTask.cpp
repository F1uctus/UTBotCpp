#include "BaseForkTask.h"

#include "loguru.h"

#include <utility>

BaseForkTask::BaseForkTask(std::string processName,
                           const std::optional<std::chrono::seconds> &timeout,
                           fs::path logFilePath,
                           bool redirectStderr,
                           bool ignoreErrors)
    : processName(std::move(processName)), logFilePath(std::move(logFilePath)), timeout(timeout),
      redirectStderr(redirectStderr), ignoreErrors(ignoreErrors) {
}

bool BaseForkTask::wasInterrupted(int exitCode) {
    return (exitCode == TIMEOUT_CODE);
}

void BaseForkTask::timeoutMessage() const {
}

void BaseForkTask::waitMessage() const {
}

void BaseForkTask::logFailMessage() const {
}

void BaseForkTask::killMessage(int status) const {
}

void BaseForkTask::initMessage() const {
}

ExecUtils::ExecutionResult BaseForkTask::finish(int status) {
    std::string output = collectAndCleanup();
    if (cancelled) {
        status = TIMEOUT_CODE;
    }
    if (!ignoreErrors && status && status != TIMEOUT_CODE) {
        LOG_S(ERROR) << "Exit status '" << processName << "': " << status;
        LOG_S(ERROR) << "Output: " << output;
        LOG_S(ERROR) << "See details in " << logFilePath;
    }
    LOG_IF_S(DEBUG, status == 0) << "Exit status: 0";
    if (status == 0 && !retainOutputFile) {
        fs::remove(logFilePath);
        return {output, status, std::nullopt};
    }
    return {output, status, logFilePath};
}

void BaseForkTask::setLogFilePath(fs::path path) {
    logFilePath = std::move(path);
}

void BaseForkTask::setRetainOutputFile(bool retain) {
    retainOutputFile = retain;
}
