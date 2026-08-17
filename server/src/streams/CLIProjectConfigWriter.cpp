#include "CLIProjectConfigWriter.h"

#include "loguru.h"

void CLIProjectConfigWriter::writeResponse(testsgen::ProjectConfigStatus status,
                                           const std::optional<std::string> &message) const {
    const std::string detail = message.value_or("");
    if (status == testsgen::ProjectConfigStatus::IS_OK) {
        LOG_S(INFO) << "Project configuration succeeded. " << detail;
        return;
    }
    sawFailure = true;
    LOG_S(ERROR) << "Project configuration failed: "
                 << testsgen::ProjectConfigStatus_Name(status) << ". " << detail;
}
