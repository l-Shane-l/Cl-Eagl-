#include <cctype> // For std::isspace, std::isdigit, std::isalpha, std::isalnum
#include <cmath>  // For std::modf
#include <cstdio> // For snprintf
#include <filesystem>
#include <fstream>
#include <functional> // For std::function
#include <iostream>
#include <memory> // For std::unique_ptr
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility> // For std::pair
#include <vector>

// --- Forward Declarations, Structs, Helper functions ---

// read_file_contents (remains the same)
[[nodiscard]] std::optional<std::string>
read_file_contents(std::string_view filename_sv) {
  std::filesystem::path file_path = filename_sv;
  std::ifstream file(file_path, std::ios::in);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file: " << filename_sv << std::endl;
    return std::nullopt;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// TrieNode and TokenTrie (for OPERATORS primarily now)
struct TrieNode {
  std::unordered_map<char, std::unique_ptr<TrieNode>> children;
  std::optional<std::string_view> token_type;
  TrieNode() = default;
  bool isEndOfToken() const { return token_type.has_value(); }
};

class OperatorTrie { // Renamed for clarity
public:
  OperatorTrie() : root(std::make_unique<TrieNode>()) {}

  void insert(std::string_view lexeme, std::string_view tokenType) {
    TrieNode *current_node = root.get();
    for (char ch : lexeme) {
      if (current_node->children.find(ch) == current_node->children.end()) {
        current_node->children[ch] = std::make_unique<TrieNode>();
      }
      current_node = current_node->children[ch].get();
    }
    current_node->token_type = tokenType;
  }

  std::pair<size_t, std::optional<std::string_view>>
  searchLongestMatch(std::string_view text_view) const {
    TrieNode *current_node = root.get();
    size_t longest_match_length = 0;
    std::optional<std::string_view> type_of_longest_match;
    for (size_t i = 0; i < text_view.length(); ++i) {
      char ch = text_view[i];
      auto child_iter = current_node->children.find(ch);
      if (child_iter == current_node->children.end()) {
        break;
      }
      current_node = child_iter->second.get();
      if (current_node->isEndOfToken()) {
        longest_match_length = i + 1;
        type_of_longest_match = current_node->token_type;
      }
    }
    return {longest_match_length, type_of_longest_match};
  }

private:
  std::unique_ptr<TrieNode> root;
};

// Scanning Context
struct ScanContext {
  const std::string_view &source_view;
  size_t &current_pos;
  int &current_line;
  bool &in_error_flag;
  const OperatorTrie &op_trie; // Changed to OperatorTrie
  const std::unordered_map<std::string_view, std::string_view>
      &keywords; // Added keywords map

  ScanContext(const std::string_view &src_view, size_t &pos, int &line,
              bool &err_flag, const OperatorTrie &operator_trie,
              const std::unordered_map<std::string_view, std::string_view> &kws)
      : source_view(src_view), current_pos(pos), current_line(line),
        in_error_flag(err_flag), op_trie(operator_trie), keywords(kws) {}

  std::string_view remaining() const { /* ... same ... */
    if (current_pos >= source_view.length())
      return {};
    return source_view.substr(current_pos);
  }
  bool isAtEnd() const { /* ... same ... */
    return current_pos >= source_view.length();
  }
  char currentChar() const { /* ... same ... */
    return source_view[current_pos];
  }
};

using MatcherFunction = std::function<bool(ScanContext &)>;

// --- Character Type Helper Functions ---
bool isIdentifierStartChar(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool isIdentifierPartChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// --- Matcher Function Implementations ---
// scanNewline, scanWhitespace, scanComment, scanStringLiteral (remain the same)
// scanNumberLiteral and formatDoubleForLoxLiteral (remain the same)
bool scanNewline(ScanContext &ctx) { /* ... same ... */
  if (!ctx.isAtEnd() && ctx.currentChar() == '\n') {
    ctx.current_line++;
    ctx.current_pos++;
    return true;
  }
  return false;
}
bool scanWhitespace(ScanContext &ctx) { /* ... same ... */
  if (!ctx.isAtEnd() && ctx.currentChar() != '\n' &&
      std::isspace(static_cast<unsigned char>(ctx.currentChar()))) {
    ctx.current_pos++;
    return true;
  }
  return false;
}
bool scanComment(ScanContext &ctx) { /* ... same ... */
  std::string_view remaining = ctx.remaining();
  if (remaining.length() >= 2 && remaining[0] == '/' && remaining[1] == '/') {
    while (!ctx.isAtEnd() && ctx.currentChar() != '\n') {
      ctx.current_pos++;
    }
    return true;
  }
  return false;
}
bool scanStringLiteral(ScanContext &ctx) { /* ... same ... */
  if (!ctx.isAtEnd() && ctx.currentChar() == '"') {
    size_t string_start_original_pos = ctx.current_pos;
    int string_start_line = ctx.current_line;
    ctx.current_pos++;
    std::string literal_value;
    bool string_terminated = false;
    while (!ctx.isAtEnd()) {
      char char_in_string = ctx.currentChar();
      if (char_in_string == '"') {
        string_terminated = true;
        ctx.current_pos++;
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
    return true;
  }
  return false;
}
std::string formatDoubleForLoxLiteral(double val) { /* ... same ... */
  char buffer[128];
  double integer_part;
  if (std::modf(val, &integer_part) == 0.0) {
    snprintf(buffer, sizeof(buffer), "%.1f", val);
  } else {
    snprintf(buffer, sizeof(buffer), "%.15g", val);
  }
  return std::string(buffer);
}
bool scanNumberLiteral(ScanContext &ctx) { /* ... same ... */
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
    ctx.current_pos++;
    while (!ctx.isAtEnd() &&
           std::isdigit(static_cast<unsigned char>(ctx.currentChar()))) {
      ctx.current_pos++;
    }
  }
  std::string_view lexeme =
      ctx.source_view.substr(start_pos, ctx.current_pos - start_pos);
  double literal_double_val;
  try {
    literal_double_val = std::stod(std::string(lexeme));
  } catch (const std::out_of_range &) {
    std::cerr << "[line " << ctx.current_line
              << "] Error: Number literal out of range: " << lexeme
              << std::endl;
    ctx.in_error_flag = true;
    return true;
  } catch (const std::invalid_argument &) {
    std::cerr << "[line " << ctx.current_line
              << "] Error: Invalid number format (stod failed): " << lexeme
              << std::endl;
    ctx.in_error_flag = true;
    return true;
  }
  std::string literal_str_output =
      formatDoubleForLoxLiteral(literal_double_val);
  std::cout << "NUMBER " << lexeme << " " << literal_str_output << std::endl;
  return true;
}

// New/Revised Matcher for Identifiers and Keywords
bool scanIdentifierOrKeyword(ScanContext &ctx) {
  if (ctx.isAtEnd() || !isIdentifierStartChar(ctx.currentChar())) {
    return false; // Not starting with a character valid for an identifier
  }

  size_t start_pos = ctx.current_pos;
  // Consume the first character (already checked by isIdentifierStartChar)
  ctx.current_pos++;

  // Consume subsequent characters that are part of the identifier
  while (!ctx.isAtEnd() && isIdentifierPartChar(ctx.currentChar())) {
    ctx.current_pos++;
  }

  std::string_view lexeme =
      ctx.source_view.substr(start_pos, ctx.current_pos - start_pos);

  std::string_view token_type;
  // Check if the lexeme is a reserved keyword
  if (auto it = ctx.keywords.find(lexeme); it != ctx.keywords.end()) {
    token_type = it->second; // It's a keyword
  } else {
    token_type = "IDENTIFIER"; // It's a user-defined identifier
  }

  std::cout << token_type << " " << lexeme << " null" << std::endl;

  return true; // Successfully scanned an identifier or keyword
}

// Renamed scanWithTrie to scanOperator, uses op_trie from context
bool scanOperator(ScanContext &ctx) {
  if (ctx.isAtEnd())
    return false;

  // op_trie is now passed via ScanContext
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

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  if (argc < 3) {
    std::cerr << "Usage: ./your_program tokenize <filename>" << std::endl;
    return 1;
  }

  const std::string_view command = argv[1];
  const std::string_view filename_arg = argv[2];
  bool in_error = false;

  if (command == "tokenize") {
    OperatorTrie operator_trie; // Trie for operators
    // Populate operator_trie ONLY with operators
    operator_trie.insert("==", "EQUAL_EQUAL");
    operator_trie.insert("!=", "BANG_EQUAL");
    operator_trie.insert("<=", "LESS_EQUAL");
    operator_trie.insert(">=", "GREATER_EQUAL");
    operator_trie.insert("(", "LEFT_PAREN");
    operator_trie.insert(")", "RIGHT_PAREN");
    operator_trie.insert("{", "LEFT_BRACE");
    operator_trie.insert("}", "RIGHT_BRACE");
    operator_trie.insert(",", "COMMA");
    operator_trie.insert(".", "DOT");
    operator_trie.insert("-", "MINUS");
    operator_trie.insert("+", "PLUS");
    operator_trie.insert(";", "SEMICOLON");
    operator_trie.insert("*", "STAR");
    operator_trie.insert("=", "EQUAL");
    operator_trie.insert("!", "BANG");
    operator_trie.insert("<", "LESS");
    operator_trie.insert(">", "GREATER");
    operator_trie.insert("/", "SLASH");

    const std::unordered_map<std::string_view, std::string_view> keywords_map =
        {
            {"and", "AND"},     {"class", "CLASS"},   {"else", "ELSE"},
            {"false", "FALSE"}, {"for", "FOR"},       {"fun", "FUN"},
            {"if", "IF"},       {"nil", "NIL"},       {"or", "OR"},
            {"print", "PRINT"}, {"return", "RETURN"}, {"super", "SUPER"},
            {"this", "THIS"},   {"true", "TRUE"},     {"var", "VAR"},
            {"while", "WHILE"} // Add your Irish keywords here, e.g., {"má",
                               // "IF_IRISH"}
        };

    auto file_content_optional = read_file_contents(filename_arg);
    if (!file_content_optional) {
      return 1;
    }
    std::string owned_file_contents = std::move(*file_content_optional);
    std::string_view source_view = owned_file_contents;

    int current_line = 1;
    size_t current_pos = 0;

    ScanContext ctx(source_view, current_pos, current_line, in_error,
                    operator_trie, keywords_map);

    std::vector<MatcherFunction> matchers = {
        scanNewline,       scanWhitespace,    scanComment,
        scanStringLiteral, scanNumberLiteral, scanIdentifierOrKeyword,
        scanOperator};

    while (ctx.current_pos < ctx.source_view.length()) { // Use ctx members
      bool matched_in_iteration = false;
      for (const auto &matcher_func : matchers) {
        if (matcher_func(ctx)) { // Pass the context
          matched_in_iteration = true;
          break;
        }
      }

      if (!matched_in_iteration) {
        if (ctx.current_pos <
            ctx.source_view.length()) { // Check not already at EOF
          std::cerr << "[line " << ctx.current_line
                    << "] Error: Unexpected character: "
                    << ctx.source_view[ctx.current_pos] << std::endl;
          ctx.in_error_flag = true;
          ctx.current_pos++;
        } else {
          break;
        }
      }
    }

    std::cout << "EOF  null" << std::endl;

  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    return 1;
  }
  if (in_error) {
    return 65;
  }
  return 0;
}
