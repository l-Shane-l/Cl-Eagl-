#ifndef SCANNER_H
#define SCANNER_H

#include <functional> // For std::function if MatcherFunction was defined here
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "OperatorTrie.h"
#include "ScanContext.h" // Includes MatcherFunction type alias

class Scanner {
public:
  Scanner(std::string_view source_code);

  // Performs scanning and prints tokens directly.
  // Returns true if an error occurred, false otherwise.
  bool scanAndPrintTokens();

private:
  // Internal state for scanning
  std::string_view source_view_; // View of the source code string
  size_t current_pos_;
  int current_line_;
  bool in_error_flag_; // Flag to track if an error has occurred

  // Scanner owns its Trie and keywords map
  OperatorTrie operator_trie_;
  std::unordered_map<std::string_view, std::string_view> keywords_map_;

  // List of matcher functions
  std::vector<MatcherFunction> matchers_;

  // --- Individual token scanning methods ---
  // These methods match the signatures of the original free functions,
  // operating on the provided ScanContext.
  bool scanNewline(ScanContext &ctx);
  bool scanWhitespace(ScanContext &ctx);
  bool scanComment(ScanContext &ctx);
  bool scanStringLiteral(ScanContext &ctx);
  std::string
  formatDoubleForLoxLiteral(double val); // Helper for scanNumberLiteral
  bool scanNumberLiteral(ScanContext &ctx);
  bool scanIdentifierOrKeyword(ScanContext &ctx);
  bool scanOperator(ScanContext &ctx);
};

#endif // SCANNER_H
