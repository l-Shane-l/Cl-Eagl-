#include <filesystem>
#include <fstream>
#include <optional>
#include <print> // C++23
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "scanner/Scanner.h"

namespace {
[[nodiscard]] std::optional<std::string>
read_file_contents(std::string_view filename_sv) {
  std::filesystem::path file_path = filename_sv;
  std::ifstream file(file_path, std::ios::in);
  if (!file.is_open()) {
    std::println(stderr, "Error: Could not open file: {}", filename_sv);
    return std::nullopt;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
} // namespace

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::println(stderr, "Usage: ./your_program tokenize <filename>");
    return 1;
  }

  const std::string_view command = argv[1];
  const std::string_view filename_arg = argv[2];
  bool scan_had_error = false;

  if (command == "tokenize") {
    auto file_content_optional = read_file_contents(filename_arg);
    if (!file_content_optional) {
      return 1;
    }

    std::string owned_file_contents = std::move(*file_content_optional);
    std::string_view source_view = owned_file_contents;

    Scanner scanner(source_view);
    scan_had_error = scanner.scanAndPrintTokens();

    std::println("EOF  null");

  } else {
    std::println(stderr, "Unknown command: {}", command);
    return 1;
  }

  if (scan_had_error) {
    return 65;
  }
  return 0;
}
