#include "ProjectIntegrationTestSuite.hpp"

#include <chrono>
#include <string>

void ProjectIntegrationTestSuite::SetUp() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  temporaryDirectory_ =
      std::filesystem::temp_directory_path() / std::filesystem::path("vox_integration_" + std::to_string(stamp));
  std::filesystem::create_directories(temporaryDirectory_);
}

void ProjectIntegrationTestSuite::TearDown() {
  std::error_code ec;
  std::filesystem::remove_all(temporaryDirectory_, ec);
}
