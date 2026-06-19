#include "ProjectIntegrationTestSuite.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include <QCoreApplication>

namespace {

constexpr std::size_t kIntegrationTestAppNameLength = 22;

void EnsureQtCoreApplication() {
  if (QCoreApplication::instance() != nullptr) {
    return;
  }

  static int argc = 1;
  static std::array<char, kIntegrationTestAppNameLength> app_name = {
      'v', 'o', 'x', '-', 'i', 'n', 't', 'e', 'g', 'r', 'a', 't', 'i', 'o', 'n', '-', 't', 'e', 's', 't', 's', '\0'};
  static std::array<char *, 2> argv = {app_name.data(), nullptr};
  static const std::unique_ptr<QCoreApplication> kApp = std::make_unique<QCoreApplication>(argc, argv.data());
  (void) kApp;
}

} // namespace

void ProjectIntegrationTestSuite::SetUp() {
  EnsureQtCoreApplication();

  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  temporaryDirectory =
      std::filesystem::temp_directory_path() / std::filesystem::path("vox_integration_" + std::to_string(stamp));
  std::filesystem::create_directories(temporaryDirectory);
}

void ProjectIntegrationTestSuite::TearDown() {
  std::error_code ec;
  std::filesystem::remove_all(temporaryDirectory, ec);
}
