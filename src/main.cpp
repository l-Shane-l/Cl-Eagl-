#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

std::string read_file_contents(const std::string &filename);

int main(int argc, char *argv[]) {
  // Disable output buffering
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  if (argc < 3) {
    std::cerr << "Usage: ./your_program tokenize <filename>" << std::endl;
    return 1;
  }

  const std::string command = argv[1];
  bool in_error = false;

  if (command == "tokenize") {
    // Define the direct mapping from input character to token type string
    const std::unordered_map<char, std::string> token_rules = {
        {'(', "LEFT_PAREN"},  {')', "RIGHT_PAREN"}, {'{', "LEFT_BRACE"},
        {'}', "RIGHT_BRACE"}, {',', "COMMA"},       {'.', "DOT"},
        {'-', "MINUS"},       {'+', "PLUS"},        {';', "SEMICOLON"},
        {'/', "SLASH"},       {'*', "STAR"},        {'(', "LEFT_PAREN"},
        {')', "RIGHT_PAREN"}, {'{', "LEFT_BRACE"},  {'}', "RIGHT_BRACE"}};

    std::string file_contents = read_file_contents(argv[2]);
    int current_line = 0;

    for (char c : file_contents) {
      if (c == '\n') { // Specifically check for newline
        current_line++;
      }
      if (std::isspace(static_cast<unsigned char>(c))) {
        continue;
      }

      auto it = token_rules.find(c);
      if (it != token_rules.end()) {
        std::cout << it->second << " " << it->first << " null" << std::endl;
      } else {
        std::cerr << "[line 1] Error: Unexpected character: " << c << std::endl;
        in_error = true;
      }
    }
    std::cout << "EOF  null" << std::endl;
  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    return 1;
  }
  if (in_error)
    return 65;

  return 0;
}

std::string read_file_contents(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error reading file: " << filename << std::endl;
    std::exit(1);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  return buffer.str();
}
