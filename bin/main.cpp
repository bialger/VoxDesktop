#include "Application.hpp"

#include <cstdio>
#include <string_view>

int main(int argc, char **argv) {
  if (argc > 1) {
    const std::string_view arg(argv[1]);
    if (arg == "--help" || arg == "-h") {
      printf("vox-desktop [--help] [--version]\n");
      return 0;
    }
    if (arg == "--version") {
      printf("vox-desktop 0.1.0\n");
      return 0;
    }
  }
  return vox::app::Application::run(argc, argv);
}
