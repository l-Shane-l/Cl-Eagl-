#ifndef OPERATOR_TRIE_H
#define OPERATOR_TRIE_H

#include <memory> // For std::unique_ptr
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility> // For std::pair

// TrieNode structure
struct TrieNode {
  std::unordered_map<char, std::unique_ptr<TrieNode>> children;
  std::optional<std::string_view> token_type;

  TrieNode();                // Constructor declaration
  bool isEndOfToken() const; // Method declaration
};

// OperatorTrie class
class OperatorTrie {
public:
  OperatorTrie(); // Constructor declaration

  void insert(std::string_view lexeme,
              std::string_view tokenType); // Method declaration

  std::pair<size_t, std::optional<std::string_view>>
  searchLongestMatch(std::string_view text_view) const; // Method declaration

private:
  std::unique_ptr<TrieNode> root;
};

#endif // OPERATOR_TRIE_H
