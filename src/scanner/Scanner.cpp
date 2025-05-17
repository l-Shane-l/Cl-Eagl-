#include "Scanner.h"

#include <algorithm> // For std::ranges::find_if, std::ranges::distance
#include <cmath>     // For std::modf
#include <format>    // C++20/23 for std::format
#include <print>     // C++23 for std::print, std::println
#include <stdexcept> // For std::out_of_range, std::invalid_argument

// Assuming isIdentifierStartChar and isIdentifierPartChar are accessible from
// ScanContext.h If they are in an anonymous namespace there, this might require
// adjustment or moving them. From the provided files, they appear to be global
// inline functions in ScanContext.h

Scanner::Scanner(std::string_view source_code)
    : source_view_(source_code), current_pos_(0), current_line_(1),
      in_error_flag_(false) {

  // Operator trie and keywords map initialization remains the same
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

  keywords_map_ = {{"and", "AND"},     {"class", "CLASS"},   {"else", "ELSE"},
                   {"false", "FALSE"}, {"for", "FOR"},       {"fun", "FUN"},
                   {"if", "IF"},       {"nil", "NIL"},       {"or", "OR"},
                   {"print", "PRINT"}, {"return", "RETURN"}, {"super", "SUPER"},
                   {"this", "THIS"},   {"true", "TRUE"},     {"var", "VAR"},
                   {"while", "WHILE"}};

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
  ScanContext ctx(source_view_, current_pos_, current_line_, in_error_flag_,
                  operator_trie_, keywords_map_);

  while (!ctx.isAtEnd()) { // Loop while not at the end of the source
    bool matched_in_iteration = false;

    // Use std::ranges::find_if to find and execute the first successful matcher
    // The predicate itself executes the matcher_func.
    // This ensures matcher_func(ctx) is called, and its side effects occur.
    auto it = std::ranges::find_if(
        matchers_, [&](const auto &matcher_func) { return matcher_func(ctx); });

    matched_in_iteration = (it != matchers_.end());

    if (!matched_in_iteration) {
      // If no matcher succeeded, but we are not at the end, it's an error.
      // This check is important because a matcher might have consumed all
      // remaining input.
      if (!ctx.isAtEnd()) {
        std::println(stderr, "[line {}] Error: Unexpected character: {}",
                     ctx.current_line, ctx.currentChar());
        ctx.in_error_flag = true;
        ctx.current_pos++; // Advance past the unexpected character
      } else {
        // All input consumed, possibly by the last successful matcher, or we
        // started empty.
        break;
      }
    }
    // If a matcher succeeded and consumed all input, ctx.isAtEnd() will handle
    // the loop exit.
  }
  return ctx.in_error_flag;
}

bool Scanner::scanNewline(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() == '\n') {
    ctx.current_line++;
    ctx.current_pos++;
    return true;
  }
  return false;
}

bool Scanner::scanWhitespace(ScanContext &ctx) {
  // Using std::views::take_while to find contiguous whitespace (excluding
  // newline) This is slightly more involved if we only want to consume one char
  // as original. The original consumes only one whitespace character at a time.
  // To maintain that behavior:
  if (!ctx.isAtEnd() && ctx.currentChar() != '\n' &&
      std::isspace(static_cast<unsigned char>(ctx.currentChar()))) {
    ctx.current_pos++;
    return true;
  }
  return false;
  // If we wanted to consume all contiguous whitespace (not newlines):
  //   auto view = ctx.remaining() | std::views::take_while([](char c){
  //       return c != '\n' && std::isspace(static_cast<unsigned char>(c));
  //   });
  //   size_t count = std::ranges::distance(view);
  //   if (count > 0) {
  //       ctx.current_pos += count;
  //       return true;
  //   }
  //   return false;
  // Sticking to original single-char consumption for minimal behavioral change.
}

bool Scanner::scanComment(ScanContext &ctx) {
  std::string_view remaining_view = ctx.remaining();
  if (remaining_view.starts_with("//")) {
    // Find the newline character or end of view
    auto newline_pos = remaining_view.find('\n');

    if (newline_pos == std::string_view::npos) {
      // Comment goes to the end of the file
      ctx.current_pos += remaining_view.length();
    } else {
      // Comment goes up to (but not including) the newline
      ctx.current_pos += newline_pos;
      // The newline itself will be handled by scanNewline in a subsequent
      // iteration
    }
    return true;
  }
  return false;
}

bool Scanner::scanStringLiteral(ScanContext &ctx) {
  if (ctx.isAtEnd() || ctx.currentChar() != '"') {
    return false;
  }

  size_t string_start_original_pos_in_source = ctx.current_pos; // For lexeme
  int string_start_line = ctx.current_line;
  ctx.current_pos++; // Consume the opening quote

  std::string literal_value;
  bool string_terminated = false;

  std::string_view remaining_after_quote = ctx.remaining();
  size_t current_search_offset = 0;

  while (current_search_offset < remaining_after_quote.length()) {
    // Find the next occurrence of a quote or newline
    size_t next_quote_pos =
        remaining_after_quote.find('"', current_search_offset);
    size_t next_newline_pos =
        remaining_after_quote.find('\n', current_search_offset);

    if (next_quote_pos == std::string_view::npos &&
        next_newline_pos == std::string_view::npos) {
      // Neither quote nor newline found, consume rest of the view
      literal_value += remaining_after_quote.substr(current_search_offset);
      ctx.current_pos +=
          (remaining_after_quote.length() - current_search_offset);
      current_search_offset =
          remaining_after_quote.length(); // Mark as consumed
      break; // Unterminated string, will be handled below
    }

    // Determine which comes first, or if only one is found
    bool quote_found_first = (next_quote_pos != std::string_view::npos &&
                              (next_newline_pos == std::string_view::npos ||
                               next_quote_pos < next_newline_pos));

    if (quote_found_first) {
      // Closing quote found
      literal_value += remaining_after_quote.substr(
          current_search_offset, next_quote_pos - current_search_offset);
      ctx.current_pos += (next_quote_pos - current_search_offset +
                          1); // +1 for the quote itself
      string_terminated = true;
      break; // End of string
    } else {
      // Newline found (either before a quote or quote not found at all)
      // next_newline_pos must be valid here if quote_found_first is false and
      // loop continues
      literal_value += remaining_after_quote.substr(
          current_search_offset, next_newline_pos - current_search_offset +
                                     1); // include newline in literal
      ctx.current_pos += (next_newline_pos - current_search_offset + 1);
      ctx.current_line++;
      current_search_offset =
          next_newline_pos + 1; // Continue search after newline
    }
  }

  if (string_terminated) {
    std::string_view lexeme = ctx.source_view.substr(
        string_start_original_pos_in_source,
        ctx.current_pos - string_start_original_pos_in_source);
    std::println("STRING {} {}", lexeme, literal_value);
  } else {
    std::println(stderr, "[line {}] Error: Unterminated string.",
                 string_start_line);
    ctx.in_error_flag = true;
  }
  return true; // Processed a string opening (or attempt)
}

std::string Scanner::formatDoubleForLoxLiteral(double val) {
  double integer_part;
  if (std::modf(val, &integer_part) == 0.0) {
    return std::format("{:.1f}", val);
  } else {
    return std::format("{:.15g}", val);
  }
}

bool Scanner::scanNumberLiteral(ScanContext &ctx) {
  if (ctx.isAtEnd() ||
      !std::isdigit(static_cast<unsigned char>(ctx.currentChar()))) {
    return false;
  }

  size_t start_pos_in_source = ctx.current_pos; // For the final lexeme

  // Integer part
  std::string_view view_int = ctx.remaining();
  auto int_end_it = std::ranges::find_if_not(view_int, [](char c) {
    return std::isdigit(static_cast<unsigned char>(c));
  });
  ctx.current_pos += std::ranges::distance(view_int.begin(), int_end_it);

  // Fractional part
  if (!ctx.isAtEnd() && ctx.currentChar() == '.' &&
      (ctx.current_pos + 1 <
           ctx.source_view.length() && // Check there's a char after dot
       std::isdigit(static_cast<unsigned char>(
           ctx.source_view[ctx.current_pos + 1])))) { // And it's a digit

    ctx.current_pos++; // Consume the dot

    std::string_view view_frac = ctx.remaining();
    auto frac_end_it = std::ranges::find_if_not(view_frac, [](char c) {
      return std::isdigit(static_cast<unsigned char>(c));
    });
    ctx.current_pos += std::ranges::distance(view_frac.begin(), frac_end_it);
  }

  std::string_view lexeme = ctx.source_view.substr(
      start_pos_in_source, ctx.current_pos - start_pos_in_source);

  if (lexeme.empty() ||
      (lexeme.length() == 1 &&
       lexeme[0] == '.')) { // Should not happen if initial check is digit
    // This case indicates something went wrong, or initial char wasn't a digit.
    // Revert if no actual number was consumed (e.g. just a '.')
    // However, the logic ensures we start with a digit.
    // If lexeme is just ".", it means it was not followed by a digit.
    // The check for `ctx.source_view[ctx.current_pos + 1]` handles this.
    // So, lexeme should be a valid number structure at this point.
  }

  double literal_double_val;
  try {
    literal_double_val = std::stod(std::string(lexeme));
  } catch (const std::out_of_range &) {
    std::println(stderr, "[line {}] Error: Number literal out of range: {}",
                 ctx.current_line, lexeme);
    ctx.in_error_flag = true;
    return true;
  } catch (const std::invalid_argument &) {
    std::println(stderr,
                 "[line {}] Error: Invalid number format (stod failed): {}",
                 ctx.current_line, lexeme);
    ctx.in_error_flag = true;
    return true;
  }
  std::string literal_str_output =
      formatDoubleForLoxLiteral(literal_double_val);
  std::println("NUMBER {} {}", lexeme, literal_str_output);
  return true;
}

bool Scanner::scanIdentifierOrKeyword(ScanContext &ctx) {
  if (ctx.isAtEnd() || !isIdentifierStartChar(ctx.currentChar())) {
    return false;
  }

  size_t start_pos_in_source = ctx.current_pos;
  ctx.current_pos++; // Consume the first character (already validated as
                     // IdentifierStartChar)

  // Consume subsequent identifier characters
  std::string_view view_ident_suffix = ctx.remaining();
  auto ident_suffix_end_it =
      std::ranges::find_if_not(view_ident_suffix, isIdentifierPartChar);
  ctx.current_pos +=
      std::ranges::distance(view_ident_suffix.begin(), ident_suffix_end_it);

  std::string_view lexeme = ctx.source_view.substr(
      start_pos_in_source, ctx.current_pos - start_pos_in_source);

  std::string_view token_type_str;
  if (auto it = ctx.keywords.find(lexeme); it != ctx.keywords.end()) {
    token_type_str = it->second;
  } else {
    token_type_str = "IDENTIFIER";
  }

  std::println("{} {} null", token_type_str, lexeme);
  return true;
}

bool Scanner::scanOperator(ScanContext &ctx) {
  if (ctx.isAtEnd())
    return false;

  // OperatorTrie handles multi-character operators correctly.
  // This part is already quite optimized for its specific task.
  auto match_result = ctx.op_trie.searchLongestMatch(ctx.remaining());
  size_t matched_length = match_result.first;
  std::optional<std::string_view> matched_type_opt = match_result.second;

  if (matched_length > 0 && matched_type_opt) {
    std::string_view lexeme = ctx.remaining().substr(0, matched_length);
    std::println("{} {} null", *matched_type_opt, lexeme);
    ctx.current_pos += matched_length;
    return true;
  }
  return false;
}
