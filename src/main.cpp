#include <cctype> // For std::isspace
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory> // For std::unique_ptr
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map> // For TrieNode's children
#include <utility>       // For std::pair

// [[nodiscard]] std::optional<std::string> read_file_contents(...)
// This function remains unchanged from the C++17 version you provided.
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

// --- Trie Implementation ---

struct TrieNode {
  // Using a map for children is flexible for various character sets (including
  // UTF-8 bytes)
  std::unordered_map<char, std::unique_ptr<TrieNode>> children;
  // Stores the token type if the path ending at this node forms a complete
  // token.
  std::optional<std::string_view> token_type;

  TrieNode() = default; // Use default constructor

  bool isEndOfToken() const { return token_type.has_value(); }
};

class TokenTrie {
public:
  TokenTrie() : root(std::make_unique<TrieNode>()) {}

  void insert(std::string_view lexeme, std::string_view tokenType) {
    TrieNode *current_node = root.get();
    for (char ch : lexeme) {
      // If path does not exist, create it
      if (current_node->children.find(ch) == current_node->children.end()) {
        current_node->children[ch] = std::make_unique<TrieNode>();
      }
      current_node = current_node->children[ch].get();
    }
    current_node->token_type =
        tokenType; // Mark end of token and store its type
  }

  // Searches for the longest token in 'text_view' starting at its beginning.
  // Returns a pair: {length_of_match, token_type_if_matched}
  // If no token is matched starting at text_view[0], length_of_match will be 0.
  std::pair<size_t, std::optional<std::string_view>>
  searchLongestMatch(std::string_view text_view) const {
    TrieNode *current_node = root.get();
    size_t longest_match_length = 0;
    std::optional<std::string_view>
        type_of_longest_match; // Default to no match

    for (size_t i = 0; i < text_view.length(); ++i) {
      char ch = text_view[i];
      auto child_iter = current_node->children.find(ch);
      if (child_iter == current_node->children.end()) {
        // No further path in Trie for this character sequence
        break;
      }
      current_node = child_iter->second.get(); // Move to child node

      if (current_node->isEndOfToken()) { // Check if this node marks the end of
                                          // a valid token
        longest_match_length = i + 1;     // Length of the token found so far
        type_of_longest_match = current_node->token_type;
      }
    }
    return {longest_match_length, type_of_longest_match};
  }

private:
  std::unique_ptr<TrieNode> root;
};

// --- Main Application Logic ---

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

    // Populate the Trie with fixed tokens (operators and keywords)
    // Operators
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
    token_matcher_trie.insert(
        "/", "SLASH"); // Will be matched if not part of a comment

    // Add English keywords (replace/add your Irish keywords here later)
    // For your Irish language, you'd insert keywords like:
    // token_matcher_trie.insert("má", "KEYWORD_IF_IRISH");
    // token_matcher_trie.insert("nó", "KEYWORD_OR_IRISH");
    // token_matcher_trie.insert("déan", "KEYWORD_DO_IRISH");
    token_matcher_trie.insert("and", "AND");
    token_matcher_trie.insert("class", "CLASS");
    token_matcher_trie.insert("else", "ELSE");
    token_matcher_trie.insert(
        "false", "FALSE_KW"); // Using _KW to distinguish from actual boolean
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

    auto file_content_optional = read_file_contents(filename_arg);
    if (!file_content_optional) {
      return 1;
    }
    std::string owned_file_contents = std::move(*file_content_optional);
    std::string_view source_view = owned_file_contents;

    int current_line = 1;
    size_t current_pos = 0;

    while (current_pos < source_view.length()) {
      std::string_view remaining_source = source_view.substr(current_pos);
      if (remaining_source.empty())
        break;

      char first_char_of_remaining = remaining_source.front();

      // 1. Handle Newlines
      if (first_char_of_remaining == '\n') {
        current_line++;
        current_pos++;
        continue;
      }

      // 2. Skip Other Whitespace
      if (std::isspace(static_cast<unsigned char>(first_char_of_remaining))) {
        current_pos++;
        continue;
      }

      // 3. Handle Comments (//)
      if (remaining_source.length() >= 2 && remaining_source[0] == '/' &&
          remaining_source[1] == '/') {
        while (current_pos < source_view.length() &&
               source_view[current_pos] != '\n') {
          current_pos++;
        }
        continue;
      }

      // 4. Try to match fixed tokens (operators & keywords) using the Trie
      auto match_result =
          token_matcher_trie.searchLongestMatch(remaining_source);
      size_t matched_length = match_result.first;
      std::optional<std::string_view> matched_type_opt = match_result.second;

      if (matched_length > 0 && matched_type_opt) {
        std::string_view lexeme = remaining_source.substr(0, matched_length);
        std::cout << *matched_type_opt << " " << lexeme << " null" << std::endl;
        current_pos += matched_length;
      } else {
        // No fixed token matched by the Trie.
        // This is where you would add logic for:
        // 1. Numbers (e.g., check if first_char_of_remaining is a digit)
        // 2. Identifiers (that are not keywords)
        // 3. String literals
        // For now, if nothing matched, it's an unexpected character.
        if (current_pos >= source_view.length())
          break; // Should be caught by outer while
        std::cerr << "[line " << current_line
                  << "] Error: Unexpected character: "
                  << source_view[current_pos] << std::endl;
        in_error = true;
        current_pos++;
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
