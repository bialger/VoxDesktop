#include "Application.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char **argv) {
  if (argc > 1) {
    const std::string_view arg(argv[1]);
    if (arg == "--help" || arg == "-h") {
      std::cout << "vox-desktop [--help] [--version]\n";
      return 0;
    }
    if (arg == "--version") {
      std::cout << "vox-desktop 0.1.0\n";
      return 0;
    }
  }
  return vox::app::Application::run(argc, argv);
}
