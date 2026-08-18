#include "Language.h"

namespace utbot {
    namespace {
        // CXX is what every caller got before the setting existed, and what a
        // request that does not mention it still gets.
        thread_local Language currentTestLanguage = Language::CXX;
    }

    Language TestLanguage::get() {
        return currentTestLanguage;
    }

    void TestLanguage::set(Language language) {
        currentTestLanguage = language == Language::C ? Language::C : Language::CXX;
    }
}
