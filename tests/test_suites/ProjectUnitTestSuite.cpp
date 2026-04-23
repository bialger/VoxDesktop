#include "ProjectUnitTestSuite.hpp"

#include <chrono>
#include <string>

#include <QCoreApplication>

namespace {

void ensureQtCoreApplication() {
  if (QCoreApplication::instance() != nullptr) {
    return;
  }

  static int argc = 1;
  static char appName[] = "vox-unit-tests";
  static char *argv[] = {appName, nullptr};
  static QCoreApplication *app = new QCoreApplication(argc, argv);
  (void) app;
}

} // namespace

void ProjectUnitTestSuite::SetUp() {
  ensureQtCoreApplication();

  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  temporaryDirectory =
      std::filesystem::temp_directory_path() / std::filesystem::path("vox_unit_" + std::to_string(stamp));
  std::filesystem::create_directories(temporaryDirectory);
}

void ProjectUnitTestSuite::TearDown() {
  std::error_code ec;
  std::filesystem::remove_all(temporaryDirectory, ec);
}
