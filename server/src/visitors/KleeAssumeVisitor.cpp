#include "KleeAssumeVisitor.h"

#include "KleeAssumeReturnValueVisitor.h"
#include "utils/KleeOptions.h"
#include "utils/PrinterUtils.h"

namespace visitor {
    KleeAssumeVisitor::KleeAssumeVisitor(const types::TypesHandler *typesHandler,
                                         printer::KleePrinter *printer)
        : AbstractValueViewVisitor(typesHandler, types::PointerUsage::RETURN), printer(printer) {
    }

    void KleeAssumeVisitor::visitPointer(const types::Type &type,
                                         const std::string &name,
                                         const tests::AbstractValueView *view,
                                         const std::string &access,
                                         int depth) {
        if (depth == 0) {
            AbstractValueViewVisitor::visitPointer(type, name, view, access, depth);
        } else {
            std::string assumption = PrinterUtils::getEqualString(name, PrinterUtils::C_NULL);
            kleeAssume(assumption);
        }
    }

    void KleeAssumeVisitor::kleeAssume(const std::string &assumption) {
        printer->strFunctionCall(PrinterUtils::KLEE_ASSUME, { assumption });
    }

    std::string KleeAssumeVisitor::equalityAssumption(const types::Type &type,
                                                      const std::string &lhs,
                                                      const std::string &rhs) {
        if (types::TypesHandler::isFloatingPointType(type)) {
            // Representations rather than values, whether or not the float is
            // symbolic: == is false when either side is NaN, so assuming it
            // kills the path instead of recording what happened on it. Equal
            // bits is the equality that means "this is the value the run had".
            return PrinterUtils::getBitsEqualString(lhs, rhs);
        }
        return PrinterUtils::getEqualString(lhs, rhs);
    }
}
