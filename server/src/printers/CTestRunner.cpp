#include "CTestRunner.h"

#include <sstream>

namespace printer {
namespace CTestRunner {

    const std::string &runtime() {
        static const std::string source = R"CSRC(/*
 * UnitTestBot's C test runner.
 *
 * The tests in the file that includes this are ordinary functions; the main()
 * at the end of that file hands them to utbot_run_tests. A generated test
 * binary is therefore two generated files and a C compiler -- no gtest, no
 * C++, nothing that has to be cross-compiled for a board first.
 *
 * It answers the same command line gtest does, because the same server reads
 * the results: --gtest_list_tests, --gtest_filter, --gtest_output=json:PATH,
 * and the GTEST_FILTER environment variable.
 *
 * Define UTBOT_TEST_NO_STDIO to build without <stdio.h>: no messages, no
 * command line, no report file. main() runs every test and returns how many
 * failed, which is all a board without a console can report anyway.
 *
 * Define UTBOT_TEST_BRIEF to print only failures and the closing count. A
 * board's console is the debugger, one round trip per character, and a line
 * per test either way is the difference between a run that finishes and one
 * that is still talking when the session times out.
 *
 * Define UTBOT_TEST_SKIP_ERROR_SUITE to leave the error suite unrun. Those
 * tests replay a path a sanitizer objected to, and they assert that reaching
 * the end of the function is a failure -- which is what happens wherever a
 * sanitizer is what stops it. On a target there is none, so the call goes
 * through, and where what it does is index far past the end of an array it
 * overwrites the stack it returned through. The test does not fail there; it
 * does not come back at all, and the rest of the suite never runs. A target
 * needs the filter that a hosted run can pass on the command line, and has
 * neither a command line nor an environment to pass it in.
 */
#ifndef UTBOT_C_TEST_RUNNER
#define UTBOT_C_TEST_RUNNER

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef UTBOT_TEST_NO_STDIO
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif

typedef void (*utbot_test_body)(void);

typedef struct {
    const char *suite;
    const char *name;
    utbot_test_body body;
} utbot_test_case;

/* The suite a generated error test is filed under, and the one
   UTBOT_TEST_SKIP_ERROR_SUITE leaves out. Compared rather than matched: it is
   one exact name, and a target should not carry a pattern matcher to find it. */
#define UTBOT_ERROR_SUITE_NAME "error"

#ifdef UTBOT_TEST_SKIP_ERROR_SUITE
#define UTBOT_TEST_IS_SKIPPED(testCase) \
    (strcmp((testCase).suite, UTBOT_ERROR_SUITE_NAME) == 0)
#else
#define UTBOT_TEST_IS_SKIPPED(testCase) 0
#endif

/* gtest's TEST(suite, name) { ... } declares a class and registers it. Here it
   opens a plain function, and the table at the end of the file is the
   registration -- the generator knows every test it wrote, so nothing has to
   happen before main() to find them. */
#define TEST(suite, name) static void suite##_##name(void)

/* Set by a failing assertion, read by the runner once the body returns, so a
   test that fails twice is still one failed test. */
static int utbot_current_test_failed = 0;

#ifdef UTBOT_TEST_NO_STDIO
#define UTBOT_SAY(...) ((void) 0)
#else
#define UTBOT_SAY(...) ((void) printf(__VA_ARGS__))
#endif

/* How many failures one test reports before it stops describing them. A test
   that compares a large struct field by field can fail on most of them, and on
   a target the console is the debugger -- one round trip per character, so the
   report costs far more than the run. A unit whose first test failed wide spent
   twelve minutes printing and never reached its second.
   The count is still exact: what is bounded is the description, not the
   detection, and the test is marked failed either way. */
#ifndef UTBOT_TEST_MAX_REPORTED_FAILURES
#define UTBOT_TEST_MAX_REPORTED_FAILURES 8
#endif

static int utbot_current_test_reported = 0;

/* Only the basename: the directory is the same for every line and is most of
   what each one costs to print. */
static const char *utbot_basename(const char *path) {
    const char *last = path;
    const char *scan;
    for (scan = path; *scan != '\0'; ++scan) {
        if (*scan == '/' || *scan == '\\') {
            last = scan + 1;
        }
    }
    return last;
}

/* Returns whether it described this one, so the caller can skip printing the
   values behind it too. */
static int utbot_report_failure(const char *file, int line, const char *what) {
    utbot_current_test_failed = 1;
    if (utbot_current_test_reported >= UTBOT_TEST_MAX_REPORTED_FAILURES) {
        if (utbot_current_test_reported == UTBOT_TEST_MAX_REPORTED_FAILURES) {
            ++utbot_current_test_reported;
            UTBOT_SAY("  ... further failures in this test not shown\n");
        }
        return 0;
    }
    ++utbot_current_test_reported;
    UTBOT_SAY("%s:%d: Failure\n  %s\n", utbot_basename(file), line, what);
    return 1;
}

/* Showing the values behind a failed comparison needs a way to ask what type
   an expression has, which is _Generic and nothing earlier. Without it the
   message still names the expressions, which is what says where to look. */
#if !defined(UTBOT_TEST_NO_STDIO) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static inline void utbot_show_signed(const char *label, long long value) {
    printf("    %s: %lld\n", label, value);
}
static inline void utbot_show_unsigned(const char *label, unsigned long long value) {
    printf("    %s: %llu\n", label, value);
}
static inline void utbot_show_real(const char *label, long double value) {
    printf("    %s: %.17Lg\n", label, value);
}
static inline void utbot_show_address(const char *label, const void *value) {
    printf("    %s: %p\n", label, value);
}
#define UTBOT_SHOW(label, value)                                              \
    _Generic((value),                                                         \
             _Bool: utbot_show_unsigned,                                      \
             char: utbot_show_signed,                                         \
             signed char: utbot_show_signed,                                  \
             short: utbot_show_signed,                                        \
             int: utbot_show_signed,                                          \
             long: utbot_show_signed,                                         \
             long long: utbot_show_signed,                                    \
             unsigned char: utbot_show_unsigned,                              \
             unsigned short: utbot_show_unsigned,                             \
             unsigned int: utbot_show_unsigned,                               \
             unsigned long: utbot_show_unsigned,                              \
             unsigned long long: utbot_show_unsigned,                         \
             float: utbot_show_real,                                          \
             double: utbot_show_real,                                         \
             long double: utbot_show_real,                                    \
             default: utbot_show_address)((label), (value))
#else
#define UTBOT_SHOW(label, value) ((void) 0)
#endif

/* The comparisons below read a float through an integer of the same width,
   which is what their ULP distance is a distance in. */
typedef char utbot_float_is_four_bytes[sizeof(float) == sizeof(uint32_t) ? 1 : -1];
typedef char utbot_double_is_eight_bytes[sizeof(double) == sizeof(uint64_t) ? 1 : -1];

static inline int utbot_float_almost_eq(float lhs, float rhs) {
    union { float value; uint32_t bits; } left, right;
    uint32_t x, y;
    left.value = lhs;
    right.value = rhs;
    /* Identical representations are equal, NaN included. gtest says a NaN
       differs from everything, which would fail a test on the very value the
       run it was written from produced. */
    if (left.bits == right.bits) {
        return 1;
    }
    if (lhs != lhs || rhs != rhs) {
        return 0;
    }
    x = (left.bits & UINT32_C(0x80000000)) ? (~left.bits + 1u)
                                           : (left.bits | UINT32_C(0x80000000));
    y = (right.bits & UINT32_C(0x80000000)) ? (~right.bits + 1u)
                                            : (right.bits | UINT32_C(0x80000000));
    return (x > y ? x - y : y - x) <= 4u;
}

static inline int utbot_double_almost_eq(double lhs, double rhs) {
    union { double value; uint64_t bits; } left, right;
    uint64_t x, y;
    left.value = lhs;
    right.value = rhs;
    if (left.bits == right.bits) {
        return 1;
    }
    if (lhs != lhs || rhs != rhs) {
        return 0;
    }
    x = (left.bits & UINT64_C(0x8000000000000000)) ? (~left.bits + 1u)
                                                   : (left.bits | UINT64_C(0x8000000000000000));
    y = (right.bits & UINT64_C(0x8000000000000000)) ? (~right.bits + 1u)
                                                    : (right.bits | UINT64_C(0x8000000000000000));
    return (x > y ? x - y : y - x) <= UINT64_C(4);
}

#define UTBOT_EXPECT_RELATION(lhs, rhs, op)                                   \
    do {                                                                      \
        if (!((lhs) op (rhs))) {                                              \
            if (utbot_report_failure(__FILE__, __LINE__,                      \
                                 "expected (" #lhs ") " #op " (" #rhs ")")) { \
                UTBOT_SHOW("left", (lhs));                                    \
                UTBOT_SHOW("right", (rhs));                                   \
            }                                                                 \
        }                                                                     \
    } while (0)

#define EXPECT_EQ(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, ==)
#define EXPECT_NE(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, !=)
#define EXPECT_LT(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, <)
#define EXPECT_GT(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, >)
#define EXPECT_LE(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, <=)
#define EXPECT_GE(lhs, rhs) UTBOT_EXPECT_RELATION(lhs, rhs, >=)

#define EXPECT_TRUE(condition)                                                \
    do {                                                                      \
        if (!(condition)) {                                                   \
            utbot_report_failure(__FILE__, __LINE__, "expected " #condition); \
        }                                                                     \
    } while (0)

#define EXPECT_FLOAT_EQ(lhs, rhs)                                             \
    do {                                                                      \
        if (!utbot_float_almost_eq((float) (lhs), (float) (rhs))) {           \
            if (utbot_report_failure(__FILE__, __LINE__,                      \
                                 "expected (" #lhs ") equals (" #rhs ")")) {  \
                UTBOT_SHOW("left", (float) (lhs));                            \
                UTBOT_SHOW("right", (float) (rhs));                           \
            }                                                                 \
        }                                                                     \
    } while (0)

#define EXPECT_DOUBLE_EQ(lhs, rhs)                                            \
    do {                                                                      \
        if (!utbot_double_almost_eq((double) (lhs), (double) (rhs))) {        \
            if (utbot_report_failure(__FILE__, __LINE__,                      \
                                 "expected (" #lhs ") equals (" #rhs ")")) {  \
                UTBOT_SHOW("left", (double) (lhs));                           \
                UTBOT_SHOW("right", (double) (rhs));                          \
            }                                                                 \
        }                                                                     \
    } while (0)

/* What gtest spells FAIL() << "text". A C macro cannot take a stream, so the
   generated tests hand it the text they would have written. */
#define UTBOT_FAIL(message) utbot_report_failure(__FILE__, __LINE__, (message))

/* A C test runs in the process that started it: there is no child to watch
   die and no exception to catch. The call is made, and if it does end the
   process, the runner reports the death that was expected. */
#define ASSERT_DEATH(statement, regex) ((void) (statement))
#define EXPECT_ANY_THROW(statement) ((void) (statement))

/* Reading a value back out of the bytes a run recorded. The C++ header does
   this with a function template; a union of the two views does the same
   without one, and a compound literal keeps it an expression, which is where
   the generated code puts it. The bytes arrive as a string literal, hence the
   variadic tail: a brace list of them would otherwise be several arguments. */
#define UTBOT_FROM_BYTES(type, ...)                                           \
    (((union { char as_bytes[sizeof(type)]; type as_value; }) {               \
         .as_bytes = __VA_ARGS__                                              \
     }).as_value)

/* Writing through a pointer whose pointee is const: C++ says const_cast, C
   says take the address and cast the qualifier off. Asking for "that type,
   without its qualifiers" is spelled typeof_unqual in C23 and
   __typeof_unqual__ before it; where neither exists the name is left alone,
   so a const violation is reported where it is rather than written through. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ > 201710L
#define constCast(lvalue) (*(typeof_unqual(lvalue) *) (void *) &(lvalue))
#elif defined(__clang__) || defined(__GNUC__)
#define constCast(lvalue) (*(__typeof_unqual__(lvalue) *) (void *) &(lvalue))
#else
#define constCast(lvalue) (lvalue)
#endif

/* Matches one gtest pattern: a name in which '*' stands for any run of
   characters and '?' for exactly one. The pattern is bounded rather than
   terminated, because it is a slice of a longer ':'-separated filter. */
static inline int utbot_glob_matches(const char *pattern, const char *patternEnd,
                                     const char *name) {
    const char *star = NULL;
    const char *retry = name;
    while (*name != '\0') {
        if (pattern != patternEnd && (*pattern == '?' || *pattern == *name)) {
            ++pattern;
            ++name;
        } else if (pattern != patternEnd && *pattern == '*') {
            star = pattern++;
            retry = name;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
        }
    }
    while (pattern != patternEnd && *pattern == '*') {
        ++pattern;
    }
    return pattern == patternEnd;
}

/* gtest's filter grammar: positive patterns separated by ':', then '-' and the
   patterns to subtract. No positive pattern means every name. */
static inline int utbot_filter_matches(const char *filter, const char *fullName) {
    const char *cursor = filter;
    int inNegatives = 0;
    int positiveCount = 0;
    int positiveMatched = 0;
    int negativeMatched = 0;

    if (filter == NULL || *filter == '\0') {
        return 1;
    }
    if (*cursor == '-') {
        ++cursor;
        inNegatives = 1;
    }
    while (*cursor != '\0') {
        const char *end = cursor;
        while (*end != '\0' && *end != ':' && *end != '-') {
            ++end;
        }
        if (end != cursor) {
            if (inNegatives) {
                negativeMatched |= utbot_glob_matches(cursor, end, fullName);
            } else {
                ++positiveCount;
                positiveMatched |= utbot_glob_matches(cursor, end, fullName);
            }
        }
        if (*end == '-') {
            inNegatives = 1;
        }
        cursor = (*end == '\0') ? end : end + 1;
    }
    return (positiveCount == 0 || positiveMatched) && !negativeMatched;
}

#ifdef UTBOT_TEST_NO_STDIO

static int utbot_run_tests(int argc, char **argv, const utbot_test_case *cases, size_t count) {
    size_t index;
    int failed = 0;
    (void) argc;
    (void) argv;
    (void) utbot_filter_matches;
    for (index = 0; index < count; ++index) {
        if (UTBOT_TEST_IS_SKIPPED(cases[index])) {
            continue;
        }
        utbot_current_test_failed = 0;
        utbot_current_test_reported = 0;
        cases[index].body();
        failed += utbot_current_test_failed;
    }
    return failed;
}

#else

/* Long enough for "suite.name" built out of C identifiers. A name that did not
   fit would silently stop matching its own filter, so one that does not fit is
   run rather than skipped. */
#define UTBOT_FULL_NAME_CAPACITY 512

static void utbot_write_report(const char *path, size_t total, size_t failed, double seconds) {
    FILE *out = fopen(path, "w");
    if (out == NULL) {
        return;
    }
    fprintf(out,
            "{\n"
            "  \"tests\": %lu,\n"
            "  \"failures\": %lu,\n"
            "  \"disabled\": 0,\n"
            "  \"errors\": 0,\n"
            "  \"time\": \"%.3fs\",\n"
            "  \"name\": \"AllTests\"\n"
            "}\n",
            (unsigned long) total, (unsigned long) failed, seconds);
    fclose(out);
}

static int utbot_run_tests(int argc, char **argv, const utbot_test_case *cases, size_t count) {
    const char *filter = getenv("GTEST_FILTER");
    const char *reportPath = NULL;
    int listOnly = 0;
    int argIndex;
    size_t index;
    size_t selected = 0;
    size_t failed = 0;
    const char *printedSuite = NULL;
    clock_t startedAt;

    /* A test that ends the process -- a sanitizer report, a real crash -- takes
       whatever is still in the buffer with it, and what is still in the buffer
       is the record of every test that passed before it. Redirected into a
       pipe, which is how the makefile runs this, that is all of them. */
    setvbuf(stdout, NULL, _IONBF, 0);

    for (argIndex = 1; argIndex < argc; ++argIndex) {
        const char *argument = argv[argIndex];
        if (strcmp(argument, "--gtest_list_tests") == 0) {
            listOnly = 1;
        } else if (strncmp(argument, "--gtest_filter=", 15) == 0) {
            filter = argument + 15;
        } else if (strncmp(argument, "--gtest_output=json:", 20) == 0) {
            reportPath = argument + 20;
        }
    }

    if (listOnly) {
        /* The first line is the one the server skips before reading the list;
           gtest puts the location of its own main() there. */
        printf("Running main() from the generated C test runner\n");
        for (index = 0; index < count; ++index) {
            if (printedSuite == NULL || strcmp(printedSuite, cases[index].suite) != 0) {
                printedSuite = cases[index].suite;
                printf("%s.\n", printedSuite);
            }
            printf("  %s\n", cases[index].name);
        }
        fflush(stdout);
        return 0;
    }

    startedAt = clock();
    for (index = 0; index < count; ++index) {
        char fullName[UTBOT_FULL_NAME_CAPACITY];
        int written = snprintf(fullName, sizeof(fullName), "%s.%s", cases[index].suite,
                               cases[index].name);
        int fits = written > 0 && (size_t) written < sizeof(fullName);
        const char *shown = fits ? fullName : cases[index].name;
        if (UTBOT_TEST_IS_SKIPPED(cases[index])) {
            continue;
        }
        if (fits && !utbot_filter_matches(filter, fullName)) {
            continue;
        }
        ++selected;
#ifndef UTBOT_TEST_BRIEF
        printf("[ RUN      ] %s\n", shown);
#endif
        utbot_current_test_failed = 0;
        utbot_current_test_reported = 0;
        cases[index].body();
        if (utbot_current_test_failed) {
            ++failed;
            printf("[  FAILED  ] %s\n", shown);
        } else {
#ifndef UTBOT_TEST_BRIEF
            printf("[       OK ] %s\n", shown);
#endif
        }
    }

    printf("[==========] %lu test(s) ran, %lu failed.\n", (unsigned long) selected,
           (unsigned long) failed);
    fflush(stdout);

    if (reportPath != NULL) {
        utbot_write_report(reportPath, selected, failed,
                           (double) (clock() - startedAt) / (double) CLOCKS_PER_SEC);
    }
    return failed == 0 ? 0 : 1;
}

#endif /* UTBOT_TEST_NO_STDIO */

#endif /* UTBOT_C_TEST_RUNNER */
)CSRC";
        return source;
    }

    std::string entryPoint(const std::vector<TestEntry> &tests) {
        std::stringstream ss;
        ss << "\n"
              "/* Every test above, in the order it was printed. The trailing entry keeps\n"
              "   the array from being empty when a file produced no test at all. */\n"
              "static const utbot_test_case utbot_all_tests[] = {\n";
        for (const TestEntry &test: tests) {
            ss << "    { \"" << test.suite << "\", \"" << test.name << "\", " << test.suite << "_"
               << test.name << " },\n";
        }
        ss << "    { 0, 0, 0 }\n"
              "};\n"
              "\n"
              "int main(int argc, char **argv) {\n"
              "    return utbot_run_tests(argc, argv, utbot_all_tests, "
           << tests.size()
           << ");\n"
              "}\n";
        return ss.str();
    }
}
}
