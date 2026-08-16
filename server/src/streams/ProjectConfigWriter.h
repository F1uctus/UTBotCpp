#ifndef UNITTESTBOT_PROJECTCONFIGWRITER_H
#define UNITTESTBOT_PROJECTCONFIGWRITER_H

#include "ServerWriter.h"

#include <grpcpp/grpcpp.h>
#include <protobuf/testgen.grpc.pb.h>

class ProjectConfigWriter : public utbot::ServerWriter<testsgen::ProjectConfigResponse> {
public:
    explicit ProjectConfigWriter(grpc::ServerWriter<testsgen::ProjectConfigResponse> *writer)
        : ServerWriter(writer) {
    }

    /// Virtual because the configuration is handed a reference to this type
    /// and the CLI substitutes a writer that reports to a terminal instead.
    virtual void writeResponse(testsgen::ProjectConfigStatus status,
                               std::optional<std::string> const &message = std::nullopt) const;

    virtual ~ProjectConfigWriter() = default;
};


#endif // UNITTESTBOT_PROJECTCONFIGWRITER_H
