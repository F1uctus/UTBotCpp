#include "MockedFunctionUsageMatchCallback.h"

#include "NameDecorator.h"
#include "clang-utils/ClangUtils.h"
#include "clang-utils/Matchers.h"

#include "loguru.h"

using namespace clang;

namespace {

/// Whether a value of this type can be recorded in a test case and handed back
/// by a stand-in.
///
/// A scalar can: it is a fixed number of bytes with no interior structure to
/// rebuild. Anything else -- a struct returned by value, a pointer into an
/// object the test would also have to recreate -- would need the object graph
/// as well as the bytes, and a stand-in that returned the bytes alone would be
/// worse than no stand-in at all.
bool canBeStoodInFor(const types::Type &type) {
    return types::TypesHandler::isPrimitiveType(type) &&
           !types::TypesHandler::isVoid(type);
}

} // namespace

void MockedFunctionUsageMatchCallback::run(const MatchFinder::MatchResult &Result) {
    ExecUtils::throwIfCancelled();

    const auto *callee =
        Result.Nodes.getNodeAs<FunctionDecl>(Matchers::MOCKED_FUNCTION_USAGE);
    if (callee == nullptr) {
        return;
    }

    const std::string name = callee->getNameAsString();

    // isDefined() looks across every declaration the translation unit has seen,
    // so this is "the body is not here", not "this particular declaration has
    // no body". A function defined in another unit of the same project is still
    // caught, which is right: the bitcode KLEE runs is the whole target, so if
    // the definition is in the project it is in the bitcode and is executed
    // rather than mocked.
    if (callee->isDefined()) {
        return;
    }
    if (callee->getBuiltinID() != 0) {
        // Lowered by the compiler or modelled by KLEE; there is no call to
        // stand in for.
        return;
    }
    if (callee->isVariadic() || isa<CXXMethodDecl>(callee)) {
        // A stand-in would have to forward what it cannot see, or be declared
        // inside a class this file does not own.
        return;
    }
    if (name.rfind("klee_", 0) == 0 || name.rfind("__", 0) == 0) {
        // KLEE's own intrinsics and the compiler's reserved namespace.
        return;
    }

    SourceManager &sourceManager = Result.Context->getSourceManager();
    fs::path sourceFilePath = ClangUtils::getSourceFilePath(sourceManager);

    QualType returnQualType = callee->getReturnType();
    types::Type returnType =
        ParamsHandler::getType(returnQualType, returnQualType, sourceManager);
    if (!canBeStoodInFor(returnType)) {
        if (reported.insert(name).second) {
            LOG_S(DEBUG) << "Function \"" << name
                         << "\" has no definition, but its return type cannot be "
                            "reproduced in a test; its calls stay unmodelled.";
        }
        return;
    }

    auto functionInfo = std::make_shared<types::FunctionInfo>();
    functionInfo->name = name;
    functionInfo->isArray = false;
    functionInfo->returnType = returnType;
    for (const auto *parameter : callee->parameters()) {
        QualType parameterQualType = parameter->getType();
        functionInfo->params.push_back(
            {ParamsHandler::getType(parameterQualType, parameterQualType, sourceManager),
             NameDecorator::decorate(parameter->getNameAsString())});
    }

    tests::Tests &tests = (*parent->projectTests).at(sourceFilePath);
    if (tests.mockedFunctions.emplace(name, functionInfo).second) {
        LOG_S(DEBUG) << "Function \"" << name << "\" is called by " << sourceFilePath
                     << " and defined nowhere; recording its signature so a test can "
                        "stand in for it.";
    }
}
