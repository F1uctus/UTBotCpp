#include "VerboseAssertsParamVisitor.h"
#include "utils/KleeUtils.h"
#include "utils/PrinterUtils.h"

namespace visitor {
    VerboseAssertsParamVisitor::VerboseAssertsParamVisitor(
            const types::TypesHandler *typesHandler,
            printer::TestsPrinter *printer)
            : VerboseAssertsVisitor(typesHandler, printer, {}) {
        usage = types::PointerUsage::PARAMETER;
    }

    static thread_local std::string expectedVariable;

    void VerboseAssertsParamVisitor::visit(const Tests::MethodParam &param, const std::string &name) {
        expectedVariable = PrinterUtils::getExpectedVarName(name);
        std::string paramName = param.dataVariableName();
        types::Type paramType = param.type.arrayCloneMultiDim(usage);
        visitAny(paramType, paramName, nullptr, PrinterUtils::DEFAULT_ACCESS, 0);
        expectedVariable = {};
    }

    void VerboseAssertsParamVisitor::visitGlobal(const Tests::MethodParam &param,
                                                 const std::string &name,
                                                 const std::string &plainName) {
        // Two names, because how a test reaches the global and what it calls the
        // copy it compares against are not always the same string: a global the
        // source kept to itself is reached through a getter, and "expected_"
        // followed by a call expression is not an identifier.
        expectedVariable = PrinterUtils::getExpectedVarName(plainName.empty() ? name : plainName);
        visitAny(param.type, name, nullptr, PrinterUtils::DEFAULT_ACCESS, 0);
        expectedVariable = {};
    }

    void VerboseAssertsParamVisitor::visitPointer(const types::Type &type,
                                                  const std::string &name,
                                                  const tests::AbstractValueView *view,
                                                  const std::string &access,
                                                  int depth) {
        if (depth == 0) {
            const auto sizes = type.arraysSizes(usage);
            std::string newName = type.maybeJustPointer() ? name :
                                  PrinterUtils::getDereferencePointer(name, sizes.size());
            visitAny(type.baseTypeObj(), newName, view, access, depth);
        }
    }

    void VerboseAssertsParamVisitor::visitArray(const types::Type &type,
                                                const std::string &name,
                                                const tests::AbstractValueView *view,
                                                const std::string &access,
                                                size_t size,
                                                int depth) {
        if (depth == 0) {
            if (type.isObjectPointer()) {
                return visitPointer(type, name, view, access, depth);
            }
        }
        bool assignPointersToNull = type.isTypeContainsPointer() && depth > 0;
        if (!assignPointersToNull) {
            VerboseAssertsVisitor::visitArray(type, name, view, access, size, depth);
        }
    }

    void VerboseAssertsParamVisitor::visitPrimitive(const types::Type &type,
                                                    const std::string &name,
                                                    const tests::AbstractValueView *view,
                                                    const std::string &access,
                                                    int depth) {
        auto signature = processExpect(type, PrinterUtils::EQ,
                                       {PrinterUtils::fillVarName(access, expectedVariable), name});
        printer->strFunctionCall(signature.name, signature.args);
    }
}
