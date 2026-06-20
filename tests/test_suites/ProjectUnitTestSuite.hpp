#ifndef VOX_TESTS_PROJECTUNITTESTSUITE_HPP
#define VOX_TESTS_PROJECTUNITTESTSUITE_HPP

#include <filesystem>

#include <gtest/gtest.h>

class ProjectUnitTestSuite : public testing::Test {
protected:
  std::filesystem::path temporaryDirectory;

  void SetUp() override;
  void TearDown() override;
};

#endif // VOX_TESTS_PROJECTUNITTESTSUITE_HPP
