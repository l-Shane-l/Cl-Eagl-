#include <filesystem> // For std::filesystem::path
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector> // Included in original, kept for consistency if any part needed it

#include "scanner/Scanner.h" // Key include for the refactored scanner

// Anonymous namespace for file-local utility functions
namespace {
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
} // namespace

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf; // Ensure output is not buffered
  std::cerr << std::unitbuf; // Ensure error output is not buffered

  if (argc < 3) {
    std::cerr << "Usage: ./your_program tokenize <filename>" << std::endl;
    return 1;
  }

  const std::string_view command = argv[1];
  const std::string_view filename_arg = argv[2];
  bool scan_had_error = false;

  if (command == "tokenize") {
    auto file_content_optional = read_file_contents(filename_arg);
    if (!file_content_optional) {
      // read_file_contents already printed an error
      return 1;
    }

    // The original main.cpp used std::move here.
    std::string owned_file_contents = std::move(*file_content_optional);
    std::string_view source_view =
        owned_file_contents; // Scanner will view this

    Scanner scanner(source_view);
    scan_had_error = scanner.scanAndPrintTokens(); // Scanner performs the
                                                   // tokenization and printing

    // Print EOF token as per original logic
    std::cout << "EOF  null" << std::endl;

  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    return 1;
  }

  if (scan_had_error) {
    return 65; // Standard Lox error code for a static error
  }

  return 0;
}
