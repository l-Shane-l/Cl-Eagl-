#include "Scanner.h"

#include <cmath>     // For std::modf
#include <cstdio>    // For snprintf
#include <iostream>  // For std::cout, std::cerr
#include <stdexcept> // For std::out_of_range, std::invalid_argument

Scanner::Scanner(std::string_view source_code)
    : source_view_(source_code), current_pos_(0), current_line_(1),
      in_error_flag_(false) {

  // Populate operator_trie_
  operator_trie_.insert("==", "EQUAL_EQUAL");
  operator_trie_.insert("!=", "BANG_EQUAL");
  operator_trie_.insert("<=", "LESS_EQUAL");
  operator_trie_.insert(">=", "GREATER_EQUAL");
  operator_trie_.insert("(", "LEFT_PAREN");
  operator_trie_.insert(")", "RIGHT_PAREN");
  operator_trie_.insert("{", "LEFT_BRACE");
  operator_trie_.insert("}", "RIGHT_BRACE");
  operator_trie_.insert(",", "COMMA");
  operator_trie_.insert(".", "DOT");
  operator_trie_.insert("-", "MINUS");
  operator_trie_.insert("+", "PLUS");
  operator_trie_.insert(";", "SEMICOLON");
  operator_trie_.insert("*", "STAR");
  operator_trie_.insert("=", "EQUAL");
  operator_trie_.insert("!", "BANG");
  operator_trie_.insert("<", "LESS");
  operator_trie_.insert(">", "GREATER");
  operator_trie_.insert("/", "SLASH");

  // Populate keywords_map_
  keywords_map_ = {{"and", "AND"},     {"class", "CLASS"},   {"else", "ELSE"},
                   {"false", "FALSE"}, {"for", "FOR"},       {"fun", "FUN"},
                   {"if", "IF"},       {"nil", "NIL"},       {"or", "OR"},
                   {"print", "PRINT"}, {"return", "RETURN"}, {"super", "SUPER"},
                   {"this", "THIS"},   {"true", "TRUE"},     {"var", "VAR"},
                   {"while", "WHILE"}};

  // Populate matchers_ vector using lambdas to call member functions
  matchers_ = {
      [this](ScanContext &ctx) { return this->scanNewline(ctx); },
      [this](ScanContext &ctx) { return this->scanWhitespace(ctx); },
      [this](ScanContext &ctx) { return this->scanComment(ctx); },
      [this](ScanContext &ctx) { return this->scanStringLiteral(ctx); },
      [this](ScanContext &ctx) { return this->scanNumberLiteral(ctx); },
      [this](ScanContext &ctx) { return this->scanIdentifierOrKeyword(ctx); },
      [this](ScanContext &ctx) { return this->scanOperator(ctx); }};
}

bool Scanner::scanAndPrintTokens() {
  // The ScanContext will use references to the Scanner's internal state
  ScanContext ctx(source_view_, current_pos_, current_line_, in_error_flag_,
                  operator_trie_, keywords_map_);

  while (current_pos_ <
         source_view_
             .length()) { // Use Scanner's members directly for loop condition
    bool matched_in_iteration = false;
    for (const auto &matcher_func : matchers_) {
      if (matcher_func(ctx)) { // Pass the context
        matched_in_iteration = true;
        break;
      }
    }

    if (!matched_in_iteration) {
      // Check not already at EOF, which might have been reached by a matcher
      if (current_pos_ < source_view_.length()) {
        std::cerr << "[line " << current_line_ // Use Scanner's member
                  << "] Error: Unexpected character: "
                  << source_view_[current_pos_]
                  << std::endl; // Use Scanner's members
        in_error_flag_ = true;  // Update Scanner's member
        current_pos_++;         // Advance Scanner's member
      } else {
        break; // Already at EOF, possibly set by the last successful matcher
      }
    }
  }
  // The main function will print "EOF  null"
  return in_error_flag_; // Return the final error state
}

// --- Matcher Function Implementations (Copied verbatim from original main.cpp,
// now class methods) ---

bool Scanner::scanNewline(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() == '\n') {
    ctx.current_line++;
    ctx.current_pos++;
    return true;
  }
  return false;
}

bool Scanner::scanWhitespace(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() != '\n' &&
      std::isspace(static_cast<unsigned char>(ctx.currentChar()))) {
    ctx.current_pos++;
    return true;
  }
  return false;
}

bool Scanner::scanComment(ScanContext &ctx) {
  std::string_view remaining = ctx.remaining();
  if (remaining.length() >= 2 && remaining[0] == '/' && remaining[1] == '/') {
    while (!ctx.isAtEnd() && ctx.currentChar() != '\n') {
      ctx.current_pos++;
    }
    return true;
  }
  return false;
}

bool Scanner::scanStringLiteral(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() == '"') {
    size_t string_start_original_pos = ctx.current_pos;
    int string_start_line = ctx.current_line;
    ctx.current_pos++; // Consume the opening quote
    std::string literal_value;
    bool string_terminated = false;
    while (!ctx.isAtEnd()) {
      char char_in_string = ctx.currentChar();
      if (char_in_string == '"') {
        string_terminated = true;
        ctx.current_pos++; // Consume the closing quote
        break;
      }
      if (char_in_string == '\n')
        ctx.current_line++;
      literal_value += char_in_string;
      ctx.current_pos++;
    }
    if (string_terminated) {
      std::string_view lexeme =
          ctx.source_view.substr(string_start_original_pos,
                                 ctx.current_pos - string_start_original_pos);
      std::cout << "STRING " << lexeme << " " << literal_value << std::endl;
    } else {
      std::cerr << "[line " << string_start_line
                << "] Error: Unterminated string." << std::endl;
      ctx.in_error_flag = true;
    }
    return true; // Processed a string opening, regardless of termination
  }
  return false;
}

std::string Scanner::formatDoubleForLoxLiteral(double val) {
  char buffer[128];
  double integer_part;
  if (std::modf(val, &integer_part) == 0.0) {
    snprintf(buffer, sizeof(buffer), "%.1f", val);
  } else {
    snprintf(buffer, sizeof(buffer), "%.15g", val);
  }
  return std::string(buffer);
}

bool Scanner::scanNumberLiteral(ScanContext &ctx) {
  if (ctx.isAtEnd() ||
      !std::isdigit(static_cast<unsigned char>(ctx.currentChar()))) {
    return false;
  }
  size_t start_pos = ctx.current_pos;
  while (!ctx.isAtEnd() &&
         std::isdigit(static_cast<unsigned char>(ctx.currentChar()))) {
    ctx.current_pos++;
  }
  if (!ctx.isAtEnd() && ctx.currentChar() == '.' &&
      (ctx.current_pos + 1 < ctx.source_view.length() &&
       std::isdigit(
           static_cast<unsigned char>(ctx.source_view[ctx.current_pos + 1])))) {
    ctx.current_pos++; // Consume the dot
    while (!ctx.isAtEnd() &&
           std::isdigit(static_cast<unsigned char>(ctx.currentChar()))) {
      ctx.current_pos++;
    }
  }
  std::string_view lexeme =
      ctx.source_view.substr(start_pos, ctx.current_pos - start_pos);
  double literal_double_val;
  try {
    // std::stod requires a null-terminated string or std::string.
    // Create a temporary std::string for conversion.
    literal_double_val = std::stod(std::string(lexeme));
  } catch (const std::out_of_range &) {
    std::cerr << "[line " << ctx.current_line
              << "] Error: Number literal out of range: " << lexeme
              << std::endl;
    ctx.in_error_flag = true;
    return true; // Consumed a number, even if invalid
  } catch (const std::invalid_argument &) {
    std::cerr << "[line " << ctx.current_line
              << "] Error: Invalid number format (stod failed): " << lexeme
              << std::endl;
    ctx.in_error_flag = true;
    return true; // Consumed something that started like a number
  }
  std::string literal_str_output =
      formatDoubleForLoxLiteral(literal_double_val); // Call as member
  std::cout << "NUMBER " << lexeme << " " << literal_str_output << std::endl;
  return true;
}

bool Scanner::scanIdentifierOrKeyword(ScanContext &ctx) {
  // Using isIdentifierStartChar and isIdentifierPartChar from ScanContext.h
  if (ctx.isAtEnd() || !isIdentifierStartChar(ctx.currentChar())) {
    return false;
  }

  size_t start_pos = ctx.current_pos;
  ctx.current_pos++; // Consume the first character

  while (!ctx.isAtEnd() && isIdentifierPartChar(ctx.currentChar())) {
    ctx.current_pos++;
  }

  std::string_view lexeme =
      ctx.source_view.substr(start_pos, ctx.current_pos - start_pos);

  std::string_view token_type;
  // Check if the lexeme is a reserved keyword using keywords map from context
  if (auto it = ctx.keywords.find(lexeme); it != ctx.keywords.end()) {
    token_type = it->second; // It's a keyword
  } else {
    token_type = "IDENTIFIER"; // It's a user-defined identifier
  }

  std::cout << token_type << " " << lexeme << " null" << std::endl;
  return true;
}

bool Scanner::scanOperator(ScanContext &ctx) {
  if (ctx.isAtEnd())
    return false;

  // op_trie is accessed via the context
  auto match_result = ctx.op_trie.searchLongestMatch(ctx.remaining());
  size_t matched_length = match_result.first;
  std::optional<std::string_view> matched_type_opt = match_result.second;

  if (matched_length > 0 && matched_type_opt) {
    std::string_view lexeme = ctx.remaining().substr(0, matched_length);
    std::cout << *matched_type_opt << " " << lexeme << " null" << std::endl;
    ctx.current_pos += matched_length;
    return true;
  }
  return false;
}
