#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "OperatorTrie.h"
#include "ScanContext.h" // Includes MatcherFunction type alias

class Scanner {
public:
  Scanner(std::string_view source_code);

  bool scanAndPrintTokens();

private:
  std::string_view source_view_;
  size_t current_pos_;
  int current_line_;
  bool in_error_flag_;

  OperatorTrie operator_trie_;
  std::unordered_map<std::string_view, std::string_view> keywords_map_;

  std::vector<MatcherFunction> matchers_;

  // Matcher methods
  bool scanNewline(ScanContext &ctx);
  bool scanWhitespace(ScanContext &ctx);
  bool scanComment(ScanContext &ctx);
  bool scanStringLiteral(ScanContext &ctx);

  std::string formatDoubleForLoxLiteral(double val);

  bool scanNumberLiteral(ScanContext &ctx);
  bool scanIdentifierOrKeyword(ScanContext &ctx);
  bool scanOperator(ScanContext &ctx);
};

#endif // SCANNER_H
