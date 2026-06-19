#include "Application.hpp"

#include <cstdio>
#include <print>
#include <string_view>

int main(int argc, char **argv) {
  if (argc > 1) {
    const std::string_view arg(argv[1]);
    if (arg == "--help" || arg == "-h") {
      std::println("vox-desktop [--help] [--version]");
      return 0;
    }
    if (arg == "--version") {
      std::println("vox-desktop 0.1.0");
      return 0;
    }
  }
  return vox::app::Application::run(argc, argv);
}
