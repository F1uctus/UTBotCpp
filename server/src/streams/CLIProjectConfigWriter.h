#ifndef UNITTESTBOT_CLIPROJECTCONFIGWRITER_H
#define UNITTESTBOT_CLIPROJECTCONFIGWRITER_H

#include "ProjectConfigWriter.h"

/**
 * Reports what configuring the project did, to a terminal rather than a stream.
 *
 * The gRPC writer sends each status to the client that asked; with no client
 * there is nothing to send it to, and the base class would quietly drop it. The
 * CLI has to say the same things out loud instead, because the exit status
 * cannot: a failed import is reported as a status, not as a non-zero return.
 */
class CLIProjectConfigWriter : public ProjectConfigWriter {
public:
    CLIProjectConfigWriter() : ProjectConfigWriter(nullptr) {}

    void writeResponse(testsgen::ProjectConfigStatus status,
                       std::optional<std::string> const &message = std::nullopt) const override;

    /// Whether any status so far means the project was not imported.
    [[nodiscard]] bool failed() const { return sawFailure; }

private:
    mutable bool sawFailure = false;
};

#endif // UNITTESTBOT_CLIPROJECTCONFIGWRITER_H
