#ifndef UNITTESTBOT_KLEEASSUMEVISITOR_H
#define UNITTESTBOT_KLEEASSUMEVISITOR_H

#include "AbstractValueViewVisitor.h"
#include "Tests.h"
#include "printers/KleePrinter.h"

namespace visitor {
    class KleeAssumeVisitor : public AbstractValueViewVisitor {
    protected:
        printer::KleePrinter *const printer;

    public:
        KleeAssumeVisitor(const types::TypesHandler *typesHandler, printer::KleePrinter *printer);

    protected:
        void visitPointer(const types::Type &type,
                          const std::string &name,
                          const tests::AbstractValueView *view,
                          const std::string &access,
                          int depth) override;

        void kleeAssume(std::string const &assumption);

        /**
         * Builds an equality assumption the target KLEE can actually decide.
         *
         * For a type it compares symbolically this is plain ==. For floating
         * point on a KLEE without symbolic floating point it is an equality of
         * representations, because == would be concretized into a constant
         * before the solver saw it -- and a wrong constant kills the path with
         * "invalid klee_assume call (provably false)" rather than merely losing
         * precision.
         *
         * Both operands must be lvalues.
         */
        [[nodiscard]] static std::string equalityAssumption(const types::Type &type,
                                                            const std::string &lhs,
                                                            const std::string &rhs);
    };
}


#endif // UNITTESTBOT_KLEEASSUMEVISITOR_H
