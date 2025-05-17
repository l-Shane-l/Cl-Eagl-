#ifndef SCAN_CONTEXT_H
#define SCAN_CONTEXT_H

#include <cctype> // For std::isspace, std::isdigit, std::isalpha, std::isalnum
#include <functional> // For std::function
#include <string_view>
#include <unordered_map> // For keywords map in ScanContext
#include <vector> // For std::vector (though MatcherFunction is std::function)

// Forward declaration of OperatorTrie to avoid circular include if OperatorTrie
// needed ScanContext
class OperatorTrie;

// Scanning Context structure
struct ScanContext {
  const std::string_view &source_view;
  size_t &current_pos;
  int &current_line;
  bool &in_error_flag;
  const OperatorTrie &op_trie; // Changed to reference
  const std::unordered_map<std::string_view, std::string_view>
      &keywords; // Changed to reference

  // Constructor
  ScanContext(
      const std::string_view &src_view, size_t &pos, int &line, bool &err_flag,
      const OperatorTrie &operator_trie,
      const std::unordered_map<std::string_view, std::string_view> &kws);

  // Helper methods
  std::string_view remaining() const;
  bool isAtEnd() const;
  char currentChar() const;
};

// Type alias for matcher functions
using MatcherFunction = std::function<bool(ScanContext &)>;

// --- Character Type Helper Functions (inline as they are small and used by
// scanner logic) ---
inline bool isIdentifierStartChar(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

inline bool isIdentifierPartChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// --- Inline implementations for ScanContext methods ---
inline ScanContext::ScanContext(
    const std::string_view &src_view, size_t &pos, int &line, bool &err_flag,
    const OperatorTrie &operator_trie,
    const std::unordered_map<std::string_view, std::string_view> &kws)
    : source_view(src_view), current_pos(pos), current_line(line),
      in_error_flag(err_flag), op_trie(operator_trie), keywords(kws) {}

inline std::string_view ScanContext::remaining() const {
  if (current_pos >= source_view.length()) {
    return {};
  }
  return source_view.substr(current_pos);
}

inline bool ScanContext::isAtEnd() const {
  return current_pos >= source_view.length();
}

inline char ScanContext::currentChar() const {
  // Assumes !isAtEnd() has been checked by caller, like in original code.
  return source_view[current_pos];
}

#endif // SCAN_CONTEXT_H
