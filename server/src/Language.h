#ifndef UNITTESTBOT_LANGUAGE_H
#define UNITTESTBOT_LANGUAGE_H

namespace utbot {
  enum class Language{
    C,
    CXX,
    ANY,
    UNKNOWN
  };

  /**
   * The language the generated tests are written in.
   *
   * Independent of the language of the code under test: a C source has always
   * been tested from C++, because gtest is C++. Asking for C instead gives
   * tests that carry their own runner and need nothing but a C compiler --
   * which is the only kind that can be cross-compiled for a target board.
   *
   * It is a per-request choice, so it lives beside the request rather than in
   * the printers: Paths decides a test file's extension long before any
   * printer exists, and the header rewriter runs in a clang tool that has no
   * settings of its own. Both read it from here.
   *
   * Request-scoped means thread-local: the server answers each request on its
   * own thread, and the CLI is one request on one thread.
   */
  namespace TestLanguage {
      /// CXX unless the current request asked otherwise.
      Language get();

      void set(Language language);

      inline bool isC() {
          return get() == Language::C;
      }
  }
}


#endif // UNITTESTBOT_LANGUAGE_H
