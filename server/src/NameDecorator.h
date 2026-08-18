#ifndef UNITTESTBOT_NAMEDECORATOR_H
#define UNITTESTBOT_NAMEDECORATOR_H

#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

class NameDecorator {
public:
    static std::string decorate(std::string_view name);

    /**
     * decorate, unless the generated tests are C.
     *
     * Renaming a name that is a keyword in C++ but not in C only works because
     * the C++ generated header defines the original spelling back; a C test
     * carries no such macros and needs none, since the name was legal C where
     * it came from.
     *
     * Kept separate from decorate so that the DEFINES below, which are built
     * from it before any request exists, stay what they always were.
     */
    static std::string decorateForTests(std::string_view name);

    static std::string defineWcharT(std::string_view canonicalName);

    static const std::string UNDEF_WCHAR_T;

    static const std::unordered_set<std::string> C_KEYWORDS;

    static const std::unordered_set<std::string> CPP_KEYWORDS;

    static const std::unordered_set<std::string> CPP_ONLY_KEYWORDS;

    static const std::regex CPP_ONLY_KEYWORDS_REGEX;

    static const std::unordered_set<std::string> CPP_OPERATORS;

    static const std::vector<std::string> DEFINES;

    static const std::string DEFINES_CODE;

    static const std::vector<std::string> UNDEFS;

    static const std::string UNDEFS_CODE;

private:
    static const std::unordered_set<std::string> TO_DEFINE;
};


#endif // UNITTESTBOT_NAMEDECORATOR_H
