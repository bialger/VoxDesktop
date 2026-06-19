#include "ProjectUnitTestSuite.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include <QCoreApplication>

namespace {

constexpr std::size_t kUnitTestAppNameLength = 15;

void EnsureQtCoreApplication() {
  if (QCoreApplication::instance() != nullptr) {
    return;
  }

  static int argc = 1;
  static std::array<char, kUnitTestAppNameLength> app_name = {
      'v', 'o', 'x', '-', 'u', 'n', 'i', 't', '-', 't', 'e', 's', 't', 's', '\0'};
  static std::array<char *, 2> argv = {app_name.data(), nullptr};
  static const std::unique_ptr<QCoreApplication> kApp = std::make_unique<QCoreApplication>(argc, argv.data());
  (void) kApp;
}

} // namespace

void ProjectUnitTestSuite::SetUp() {
  EnsureQtCoreApplication();

  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  temporaryDirectory =
      std::filesystem::temp_directory_path() / std::filesystem::path("vox_unit_" + std::to_string(stamp));
  std::filesystem::create_directories(temporaryDirectory);
}

void ProjectUnitTestSuite::TearDown() {
  std::error_code ec;
  std::filesystem::remove_all(temporaryDirectory, ec);
}
