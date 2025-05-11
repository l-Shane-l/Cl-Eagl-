#include <cctype> // For std::isspace, std::isdigit, std::isalpha
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

// --- Forward Declarations & Structs ---

// read_file_contents (remains the same C++17 version)
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

// TrieNode and TokenTrie (remain the same as the last version)
struct TrieNode {
  std::unordered_map<char, std::unique_ptr<TrieNode>> children;
  std::optional<std::string_view> token_type;
  TrieNode() = default;
  bool isEndOfToken() const { return token_type.has_value(); }
};

class TokenTrie {
public:
  TokenTrie() : root(std::make_unique<TrieNode>()) {}

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

// Scanning Context to pass to matcher functions
struct ScanContext {
  const std::string_view &source_view; // Reference to the full source
  size_t &current_pos;   // Reference to the current position index
  int &current_line;     // Reference to the current line number
  bool &in_error_flag;   // Reference to the global error flag
  const TokenTrie &trie; // Reference to the populated Trie

  // Constructor
  ScanContext(const std::string_view &src_view, size_t &pos, int &line,
              bool &err_flag, const TokenTrie &t)
      : source_view(src_view), current_pos(pos), current_line(line),
        in_error_flag(err_flag), trie(t) {}

  // Helper to get the remaining source view from current_pos
  std::string_view remaining() const {
    if (current_pos >= source_view.length()) {
      return {}; // Empty view
    }
    return source_view.substr(current_pos);
  }

  // Helper to check if current_pos is at end
  bool isAtEnd() const { return current_pos >= source_view.length(); }

  // Helper to get current character
  char currentChar() const { return source_view[current_pos]; }
};

// Type alias for our matcher functions
using MatcherFunction = std::function<bool(ScanContext &)>;

// --- Matcher Function Implementations ---

bool scanNewline(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() == '\n') {
    ctx.current_line++;
    ctx.current_pos++;
    return true;
  }
  return false;
}

bool scanWhitespace(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() != '\n' &&
      std::isspace(static_cast<unsigned char>(ctx.currentChar()))) {
    ctx.current_pos++;
    return true;
  }
  return false;
}

bool scanComment(ScanContext &ctx) {
  std::string_view remaining = ctx.remaining();
  if (remaining.length() >= 2 && remaining[0] == '/' && remaining[1] == '/') {
    while (!ctx.isAtEnd() && ctx.currentChar() != '\n') {
      ctx.current_pos++;
    }
    // The newline itself (if any) will be handled by scanNewline in the next
    // iteration
    return true;
  }
  return false;
}

bool scanStringLiteral(ScanContext &ctx) {
  if (!ctx.isAtEnd() && ctx.currentChar() == '"') {
    size_t string_start_original_pos = ctx.current_pos; // For full lexeme
    int string_start_line = ctx.current_line;

    ctx.current_pos++; // Consume opening "
    std::string literal_value;
    bool string_terminated = false;

    while (!ctx.isAtEnd()) {
      char char_in_string = ctx.currentChar();
      if (char_in_string == '"') {
        string_terminated = true;
        ctx.current_pos++; // Consume closing "
        break;
      }
      if (char_in_string == '\n') {
        ctx.current_line++;
      }
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
    return true; // Attempted to scan a string
  }
  return false;
}

// Placeholder for number scanning - to be implemented
bool scanNumberLiteral(ScanContext &ctx) {
  // if (!ctx.isAtEnd() && std::isdigit(static_cast<unsigned
  // char>(ctx.currentChar()))) {
  //     // ... Add number scanning logic here ...
  //     // std::cout << "NUMBER " << lexeme << " " << literal_value <<
  //     std::endl;
  //     // ctx.current_pos += length_of_number;
  //     return true;
  // }
  return false;
}

bool scanWithTrie(ScanContext &ctx) {
  if (ctx.isAtEnd())
    return false;

  auto match_result = ctx.trie.searchLongestMatch(ctx.remaining());
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

// Placeholder for identifier scanning - to be implemented
bool scanIdentifier(ScanContext &ctx) {
  // if (!ctx.isAtEnd() && (std::isalpha(static_cast<unsigned
  // char>(ctx.currentChar())) || ctx.currentChar() == '_')) {
  //     // ... Add identifier scanning logic ...
  //     // After scanning, check if it's a keyword (our Trie already handles
  //     keywords, so this
  //     // matcher would be for non-keyword identifiers if Trie only had
  //     operators, or if
  //     // Trie had keywords, this logic would be simpler or might not be
  //     needed if Trie is exhaustive)
  //     // std::cout << "IDENTIFIER " << lexeme << " null" << std::endl; // or
  //     actual value if storing
  //     // ctx.current_pos += length_of_identifier;
  //     return true;
  // }
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
    TokenTrie token_matcher_trie;
    // Populate Trie with operators and keywords
    token_matcher_trie.insert("==", "EQUAL_EQUAL");
    token_matcher_trie.insert("!=", "BANG_EQUAL");
    token_matcher_trie.insert("<=", "LESS_EQUAL");
    token_matcher_trie.insert(">=", "GREATER_EQUAL");
    token_matcher_trie.insert("(", "LEFT_PAREN");
    token_matcher_trie.insert(")", "RIGHT_PAREN");
    token_matcher_trie.insert("{", "LEFT_BRACE");
    token_matcher_trie.insert("}", "RIGHT_BRACE");
    token_matcher_trie.insert(",", "COMMA");
    token_matcher_trie.insert(".", "DOT");
    token_matcher_trie.insert("-", "MINUS");
    token_matcher_trie.insert("+", "PLUS");
    token_matcher_trie.insert(";", "SEMICOLON");
    token_matcher_trie.insert("*", "STAR");
    token_matcher_trie.insert("=", "EQUAL");
    token_matcher_trie.insert("!", "BANG");
    token_matcher_trie.insert("<", "LESS");
    token_matcher_trie.insert(">", "GREATER");
    token_matcher_trie.insert("/", "SLASH");
    token_matcher_trie.insert("and", "AND");
    token_matcher_trie.insert("class", "CLASS");
    token_matcher_trie.insert("else", "ELSE");
    token_matcher_trie.insert("false", "FALSE_KW");
    token_matcher_trie.insert("for", "FOR");
    token_matcher_trie.insert("fun", "FUN");
    token_matcher_trie.insert("if", "IF");
    token_matcher_trie.insert("nil", "NIL");
    token_matcher_trie.insert("or", "OR");
    token_matcher_trie.insert("print", "PRINT");
    token_matcher_trie.insert("return", "RETURN");
    token_matcher_trie.insert("super", "SUPER");
    token_matcher_trie.insert("this", "THIS");
    token_matcher_trie.insert("true", "TRUE_KW");
    token_matcher_trie.insert("var", "VAR");
    token_matcher_trie.insert("while", "WHILE");
    // Add your Irish keywords here: e.g., token_matcher_trie.insert("má",
    // "IF_IRISH");

    auto file_content_optional = read_file_contents(filename_arg);
    if (!file_content_optional) {
      return 1;
    }
    std::string owned_file_contents = std::move(*file_content_optional);
    std::string_view source_view = owned_file_contents;

    int current_line = 1;
    size_t current_pos = 0;

    // Create the list of matcher functions in order of priority
    std::vector<MatcherFunction> matchers = {
        scanNewline,       scanWhitespace, scanComment, scanStringLiteral,
        scanNumberLiteral, // Placeholder, will always return false for now
        scanWithTrie,      // Handles keywords and operators
        scanIdentifier     // Placeholder, will always return false for now
    };

    ScanContext ctx(source_view, current_pos, current_line, in_error,
                    token_matcher_trie);

    while (current_pos < source_view.length()) {
      bool matched_in_iteration = false;
      for (const auto &matcher_func : matchers) {
        if (matcher_func(ctx)) { // Pass the context by reference
          matched_in_iteration = true;
          break; // Matcher handled this position, restart main while loop
        }
      }

      if (!matched_in_iteration) {
        // If no matcher handled it, and we are not at EOF
        if (current_pos < source_view.length()) {
          std::cerr << "[line " << ctx.current_line
                    << "] Error: Unexpected character: "
                    << ctx.source_view[ctx.current_pos] << std::endl;
          ctx.in_error_flag = true;
          ctx.current_pos++; // Advance past the error character
        } else {
          break; // Should be caught by the outer while loop's condition
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
