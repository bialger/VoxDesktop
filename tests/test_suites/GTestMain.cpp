#include <cstdlib>

#include <QCoreApplication>
#include <gtest/gtest.h>

namespace {

[[noreturn]] void ExitWithoutTeardown(int code) {
#if defined(_WIN32)
  ::_exit(code);
#else
  std::quick_exit(code);
#endif
}

} // namespace

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  const int result = RUN_ALL_TESTS();
  ExitWithoutTeardown(result);
}
