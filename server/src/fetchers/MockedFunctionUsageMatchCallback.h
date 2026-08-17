#ifndef UNITTESTBOT_MOCKEDFUNCTIONUSAGEMATCHCALLBACK_H
#define UNITTESTBOT_MOCKEDFUNCTIONUSAGEMATCHCALLBACK_H

#include "Fetcher.h"
#include "FetcherUtils.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <string>
#include <unordered_set>

class Fetcher;

/**
 * Records the functions a parsed source calls but that nothing defines.
 *
 * KLEE answers such a call with a symbolic value instead of dispatching it,
 * which is the only workable thing to do when the callee is a driver entry
 * point compiled for another architecture, or simply absent. Those answers show
 * up in the test case under the callee's name, and a generated test can only
 * reproduce the path it was recorded from if it can feed them back -- which
 * means standing in for the callee, which means knowing its signature.
 *
 * The signature is only available here, at the call site: nothing else in the
 * project declares the function in a form UnitTestBot already fetches, since
 * every other fetcher looks for definitions.
 */
class MockedFunctionUsageMatchCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
    using MatchFinder = clang::ast_matchers::MatchFinder;

public:
    explicit MockedFunctionUsageMatchCallback(const Fetcher *parent) : parent(parent) {
    }

    void run(const MatchFinder::MatchResult &Result) override;

private:
    Fetcher const *const parent;
    std::unordered_set<std::string> reported;
};

#endif // UNITTESTBOT_MOCKEDFUNCTIONUSAGEMATCHCALLBACK_H
