#ifndef VOX_TESTS_PROJECTINTEGRATIONTESTSUITE_HPP
#define VOX_TESTS_PROJECTINTEGRATIONTESTSUITE_HPP

#include <filesystem>

#include <gtest/gtest.h>

class ProjectIntegrationTestSuite : public testing::Test {
protected:
    std::filesystem::path temporaryDirectory;

    void SetUp() override;
    void TearDown() override;
};

#endif // VOX_TESTS_PROJECTINTEGRATIONTESTSUITE_HPP
