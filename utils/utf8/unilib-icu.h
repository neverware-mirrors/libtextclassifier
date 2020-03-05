// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// UniLib implementation with the help of ICU. UniLib is basically a wrapper
// around the ICU functionality.

#ifndef LIBTEXTCLASSIFIER_UTILS_UTF8_UNILIB_ICU_H_
#define LIBTEXTCLASSIFIER_UTILS_UTF8_UNILIB_ICU_H_

#include <memory>
#include <mutex>  // NOLINT(build/c++11)

#include "utils/base/integral_types.h"
#include "utils/utf8/unicodetext.h"
#include "unicode/brkiter.h"
#include "unicode/errorcode.h"
#include "unicode/regex.h"
#include "unicode/uchar.h"
#include "unicode/unum.h"

namespace libtextclassifier3 {

class UniLibBase {
 public:
  bool ParseInt32(const UnicodeText& text, int* result) const;
  bool IsOpeningBracket(char32 codepoint) const;
  bool IsClosingBracket(char32 codepoint) const;
  bool IsWhitespace(char32 codepoint) const;
  bool IsDigit(char32 codepoint) const;
  bool IsLower(char32 codepoint) const;
  bool IsUpper(char32 codepoint) const;
  bool IsPunctuation(char32 codepoint) const;

  char32 ToLower(char32 codepoint) const;
  char32 ToUpper(char32 codepoint) const;
  char32 GetPairedBracket(char32 codepoint) const;

  // Forward declaration for friend.
  class RegexPattern;

  class RegexMatcher {
   public:
    static constexpr int kError = -1;
    static constexpr int kNoError = 0;

    // Checks whether the input text matches the pattern exactly.
    bool Matches(int* status) const;

    // Approximate Matches() implementation implemented using Find(). It uses
    // the first Find() result and then checks that it spans the whole input.
    // NOTE: Unlike Matches() it can result in false negatives.
    // NOTE: Resets the matcher, so the current Find() state will be lost.
    bool ApproximatelyMatches(int* status);

    // Finds occurrences of the pattern in the input text.
    // Can be called repeatedly to find all occurences. A call will update
    // internal state, so that 'Start', 'End' and 'Group' can be called to get
    // information about the match.
    // NOTE: Any call to ApproximatelyMatches() in between Find() calls will
    // modify the state.
    bool Find(int* status);

    // Gets the start offset of the last match (from  'Find').
    // Sets status to 'kError' if 'Find'
    // was not called previously.
    int Start(int* status) const;

    // Gets the start offset of the specified group of the last match.
    // (from  'Find').
    // Sets status to 'kError' if an invalid group was specified or if 'Find'
    // was not called previously.
    int Start(int group_idx, int* status) const;

    // Gets the end offset of the last match (from  'Find').
    // Sets status to 'kError' if 'Find'
    // was not called previously.
    int End(int* status) const;

    // Gets the end offset of the specified group of the last match.
    // (from  'Find').
    // Sets status to 'kError' if an invalid group was specified or if 'Find'
    // was not called previously.
    int End(int group_idx, int* status) const;

    // Gets the text of the last match (from 'Find').
    // Sets status to 'kError' if 'Find' was not called previously.
    UnicodeText Group(int* status) const;

    // Gets the text of the specified group of the last match (from 'Find').
    // Sets status to 'kError' if an invalid group was specified or if 'Find'
    // was not called previously.
    UnicodeText Group(int group_idx, int* status) const;

    // Returns the matched text (the 0th capturing group).
    std::string Text() const {
      std::string result;
      return text_.toUTF8String(result);
    }

   private:
    friend class RegexPattern;
    explicit RegexMatcher(icu::RegexPattern* pattern, icu::UnicodeString text);
    bool UpdateLastFindOffset() const;

    std::unique_ptr<icu::RegexMatcher> matcher_;
    icu::UnicodeString text_;
    mutable int last_find_offset_;
    mutable int last_find_offset_codepoints_;
    mutable bool last_find_offset_dirty_;
  };

  class RegexPattern {
   public:
    std::unique_ptr<RegexMatcher> Matcher(const UnicodeText& input) const;

   private:
    friend class UniLibBase;
    explicit RegexPattern(const UnicodeText& pattern, bool lazy = false);
    void LockedInitializeIfNotAlready() const;

    // These members need to be mutable because of the lazy initialization.
    // NOTE: The Matcher method first ensures (using a lock) that the
    // initialization was attempted (by using LockedInitializeIfNotAlready) and
    // then can access them without locking.
    mutable std::mutex mutex_;
    mutable bool initialized_;
    mutable bool initialization_failure_;
    mutable UnicodeText pattern_text_;
    mutable std::unique_ptr<icu::RegexPattern> pattern_;
  };

  class BreakIterator {
   public:
    int Next();

    static constexpr int kDone = -1;

   private:
    friend class UniLibBase;
    explicit BreakIterator(const UnicodeText& text);

   private:
    std::unique_ptr<icu::BreakIterator> break_iterator_;
    icu::UnicodeString text_;
    int last_break_index_;
    int last_unicode_index_;
  };

  std::unique_ptr<RegexPattern> CreateRegexPattern(
      const UnicodeText& regex) const;
  std::unique_ptr<RegexPattern> CreateLazyRegexPattern(
      const UnicodeText& regex) const;
  std::unique_ptr<BreakIterator> CreateBreakIterator(
      const UnicodeText& text) const;
};

}  // namespace libtextclassifier3

#endif  // LIBTEXTCLASSIFIER_UTILS_UTF8_UNILIB_ICU_H_
