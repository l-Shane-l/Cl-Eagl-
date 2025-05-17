#include "OperatorTrie.h"

// TrieNode method implementations
TrieNode::TrieNode() = default;

bool TrieNode::isEndOfToken() const { return token_type.has_value(); }

// OperatorTrie method implementations
OperatorTrie::OperatorTrie() : root(std::make_unique<TrieNode>()) {}

void OperatorTrie::insert(std::string_view lexeme, std::string_view tokenType) {
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
OperatorTrie::searchLongestMatch(std::string_view text_view) const {
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
