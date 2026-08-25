#ifndef UNITTESTBOT_PRINTERUTILS_H
#define UNITTESTBOT_PRINTERUTILS_H

#include "ProjectContext.h"
#include "types/Types.h"
#include "utils/path/FileSystemPath.h"

#include <grpcpp/grpcpp.h>
#include <protobuf/testgen.grpc.pb.h>

#include <optional>
#include <string>

namespace PrinterUtils {
    const std::string constCast = "template<typename T>\n"
                                  "T& constCast(const T &val) {\n"
                                  "    return const_cast<T&>(val);\n"
                                  "}\n";

    const std::string fromBytes = "template<typename T, size_t N>\n"
                                  "T from_bytes(const char (&bytes)[N]) {\n"
                                  "    T result;\n"
                                  "    std::memcpy(&result, bytes, sizeof(result));\n"
                                  "    return result;\n"
                                  "}\n";

    const std::string redirectStdin = "void utbot_redirect_stdin(const char* buf, int &res) {\n"
                                      "    int fds[2];\n"
                                      "    if (pipe(fds) == -1) {\n"
                                      "        res = -1;\n"
                                      "        return;\n"
                                      "    }\n"
                                      "    close(STDIN_FILENO);\n"
                                      "    dup2(fds[0], STDIN_FILENO);\n"
                                      "    write(fds[1], buf, " +
                                      std::to_string(types::Type::symInputSize) +
                                      ");\n"
                                      "    close(fds[1]);\n"
                                      "}\n";

    const std::string writeToFile = "void write_to_file(const char *fileName, const char *buf) {\n"
                                    "    FILE *out = fopen(fileName, \"w\");\n"
                                    "    if (out == NULL) {\n"
                                    "        return;\n"
                                    "    }\n"
                                    "    fwrite(buf, 1, " +
                                    std::to_string(types::Type::symInputSize) +
                                    ", out);\n"
                                    "    fclose(out);\n"
                                    "}\n";

    /// The same, for a C test: static so an unused copy is not a link error,
    /// and empty where the runner was built without stdio.
    const std::string writeToFileC =
        "static void write_to_file(const char *fileName, const char *buf) {\n"
        "#ifdef UTBOT_TEST_NO_STDIO\n"
        "    (void) fileName;\n"
        "    (void) buf;\n"
        "#else\n"
        "    FILE *out = fopen(fileName, \"w\");\n"
        "    if (out == NULL) {\n"
        "        return;\n"
        "    }\n"
        "    fwrite(buf, 1, " +
        std::to_string(types::Type::symInputSize) +
        ", out);\n"
        "    fclose(out);\n"
        "#endif\n"
        "}\n";

    const std::string DEFAULT_ACCESS = "%s";
    const std::string KLEE_PREFER_CEX = "klee_prefer_cex";
    const std::string KLEE_ASSUME = "klee_assume";
    const std::string KLEE_PATH_FLAG = "kleePathFlag";
    const std::string KLEE_PATH_FLAG_SYMBOLIC = "kleePathFlagSymbolic";
    const std::string EQ_OPERATOR = " == ";
    const std::string ASSIGN_OPERATOR = " = ";
    const std::string TAB = "    ";

    const std::string EXPECTED = "expected";
    const std::string ACTUAL = "actual";
    const std::string EXPECT_ = "EXPECT_";
    const std::string EXPECT_FLOAT_EQ = "EXPECT_FLOAT_EQ";
    const std::string EXPECT_DOUBLE_EQ = "EXPECT_DOUBLE_EQ";
    const std::string EQ = "EQ";

    std::string convertToBytesFunctionName(std::string const &typeName);

    std::string convertBytesToStruct(const std::string &typeName, const std::string &bytes);

    std::string wrapperName(const std::string &declName,
                            utbot::ProjectContext const &projectContext,
                            const fs::path &sourceFilePath);

    std::string getterName(const std::string &wrapperName);

    std::string getterDecl(const std::string &returnTypeName,
                           const std::string &wrapperName);

    /**
     * How a C test names a global the source declared static.
     *
     * Such a variable has no name to link against, and a macro that gave it one
     * would rewrite every other use of that identifier in the test -- including
     * the locals the printer names after the function's own parameters, which
     * on real code collide often. So the test dereferences the getter instead,
     * and nothing but the global itself is spelled this way.
     */
    std::string fileStaticAccess(const std::string &declName,
                                 utbot::ProjectContext const &projectContext,
                                 const fs::path &sourceFilePath);

    /**
     * The #include lines \p declarations needs to be readable on their own.
     *
     * Matched by name against what was actually printed, so a header carries
     * only what its own declarations use, and a header guard makes a repeat of
     * one the runner already includes cost nothing.
     */
    std::string standardIncludesFor(const std::string &declarations);

    std::string getFieldAccess(const std::string &objectName, const types::Field &field);

    std::string getConstQualifier(bool constQualifiedValue);

    std::string fillVarName(std::string const &temp, std::string const &varName);

    void appendIndicesToVarName(std::string &varName, const std::vector<size_t> &sizes, size_t offset);

    void appendConstCast(std::string &varName);

    std::string getKleePrefix(bool forKlee);

    std::string wrapUserValue(const testsgen::ValidationType &type, const std::string &value);

    std::string getPointerMangledName(const std::string &name);
    std::string getParamMangledName(const std::string &paramName, const std::string &methodName);
    std::string getReturnMangledName(const std::string &methodName);
    std::string getReturnMangledTypeName(const std::string& methodName);
    std::string getEnumReturnMangledTypeName(const std::string& methodName);

    std::string getEqualString(const std::string &lhs, const std::string &rhs);

    /**
     * An equality test written over the representations rather than the values,
     * for types the target KLEE cannot compare symbolically.
     *
     * Both operands have to be lvalues, since their addresses are taken.
     */
    std::string getBitsEqualString(const std::string &lhs, const std::string &rhs);

    /** Name of the helper getBitsEqualString calls; defined by BITS_EQUAL_DECLARATION. */
    extern const std::string BITS_EQUAL;
    std::string getDereferencePointer(const std::string &name, const size_t depth);
    std::string getExpectedVarName(const std::string &varName);

    std::string initializePointer(const std::string &type,
                                  const std::string &value,
                                  size_t additionalPointersCount,
                                  bool pointerToConstQualifiedValue);

    std::string initializePointerToVar(const std::string &type,
                                       const std::string &varName,
                                       size_t additionalPointersCount,
                                       bool pointerToConstQualifiedValue);

    std::string generateNewVar(int cnt);

    std::string getFileParamKTestJSON(char fileName);
    std::string getFileReadBytesParamKTestJSON(char fileName);
    std::string getFileWriteBytesParamKTestJSON(char fileName);

    void removeThreadLocalQualifiers(std::string &decl);

    const std::string LAZYRENAME = "utbotInnerVar";
    const std::string UTBOT_ARGC = "utbot_argc";
    const std::string UTBOT_ARGV = "utbot_argv";
    const std::string UTBOT_ENVP = "utbot_envp";
    const std::string POSIX_INIT = "klee_init_env";
    const std::string WRAPPED_SUFFIX = "__wrapped";
    const std::string POSIX_CHECK_STDIN_READ = "check_stdin_read";
    const std::string MANGLED_PREFIX = "_Z";
    const std::string MANGLED_SUFFIX = "iPPcS0_";

    const std::string TEST_NAMESPACE = "UTBot";
    const std::string DEFINES_FOR_C_KEYWORDS =
        /* Currently Clang tool transforms RecordDecl for
         * @code
         * struct data {
         * char x;
         * _Alignas(64) char cacheline[64];
         * };
         * to
         * @code
         * struct data {
         * char x;
         * char cacheline[64] _Alignas(64);
         * };
         * which is not valid code even for C, I suppose
         * */
        "#define _Alignas(x)\n"
        // can't be a part of function declaration, only typedef
        "#define _Atomic(x) x\n"
        "#define _Bool bool\n"
        // ignore for function declaration
        "#define _Noreturn\n"
        // can't be a part of function declaration, only typedef
        "#define _Thread_local thread_local\n"
        "";

    /**
     * Undoes DEFINES_FOR_C_KEYWORDS.
     *
     * Without this the definitions leak past the declarations they were meant
     * for and into every header the test includes afterwards. "_Atomic(x) x"
     * is the one that bites: libc++ declares the storage inside std::atomic as
     * _Atomic(_Tp), so with the macro still in force it becomes a plain _Tp
     * and every __c11_atomic_* builtin on it fails to compile -- which takes
     * out <atomic>, and with it <memory> and all of gtest.
     *
     * libstdc++ implements the same headers over __atomic_* builtins on plain
     * types and never spells _Atomic, which is why this went unnoticed for as
     * long as the tests were only ever compiled on Linux.
     */
    const std::string UNDEFS_FOR_C_KEYWORDS =
        "#undef _Alignas\n"
        "#undef _Atomic\n"
        "#undef _Bool\n"
        "#undef _Noreturn\n"
        "#undef _Thread_local\n"
        "";

    // TODO This the list of known implicit records by now (i.e implicitly generated by the
    // implementation and not explicitly written in the source code). This particular is created in
    // ASTContext::buildImplicitRecord call in clang/lib/AST/ASTContext.cpp file. However, the
    // correct way would be to collect them while traversing types and write at the beginning of
    // header file.
    static const std::vector<std::string> KNOWN_IMPLICIT_RECORD_DECLS = { "struct __va_list_tag;" };

    /**
     * The standard header each standard type comes from.
     *
     * A type the project defines can have a member whose own type belongs to
     * the standard library -- a jmp_buf, a va_list -- and the generated header
     * re-declares the former having never declared the latter. The source
     * reached that type through an include, and so must the header: the type is
     * left where it is, rather than being reproduced, precisely so that it stays
     * the one the compiler building the test agrees with.
     *
     * Only types that turn up inside a struct or a signature are worth listing.
     * Anything else a declaration mentions is one the printer wrote itself.
     */
    static const std::vector<std::pair<std::string, std::string>> STANDARD_TYPE_HEADERS = {
        { "jmp_buf", "setjmp.h" },
        { "va_list", "stdarg.h" },
        { "FILE", "stdio.h" },
        { "sig_atomic_t", "signal.h" },
    };
    const std::string KNOWN_IMPLICIT_RECORD_DECLS_CODE =
        StringUtils::joinWith(KNOWN_IMPLICIT_RECORD_DECLS, "\n");

    const std::string C_NULL = "NULL";
    const std::unordered_map<int, std::string> escapeSequences = {
        { 10, "\\n" }, { 9, "\\t" },   { 11, "\\v" }, { 8, "\\b" },   { 13, "\\r" },  { 12, "\\f" },
        { 7, "\\a" },  { 92, "\\\\" }, { 63, "\\?" }, { 39, "\\\'" }, { 34, "\\\"" }, { 0, "\\0" }
    };

    const std::string KLEE_MODE = "KLEE_MODE";
    const std::string KLEE_SYMBOLIC_SUFFIX = "_symbolic";
};

#endif // UNITTESTBOT_PRINTERUTILS_H
