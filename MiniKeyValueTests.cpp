#include <gtest/gtest.h>
#include "minikeyvalue.hpp"
#include <memory>

struct MiniKeyValueTests : public testing::Test{
    void SetUp() override {
        mkv_ = MiniKeyValue::createEmpty(logFile_);
    }
    void TearDown() override {
    }

protected:
    std::shared_ptr<MiniKeyValue> mkv_;
    std::string logFile_{"data.in"};
};

TEST_F(MiniKeyValueTests, addEntryInStore) {
    mkv_->Set("Iulian", {200, 220, 61});
    std::vector<uint8_t> result = {200, 220, 61};
    EXPECT_EQ(mkv_->Get("Iulian"), result);
}

TEST_F(MiniKeyValueTests, checkStoragePersistency) {
    mkv_->Set("Robert", {1, 2, 3, 4});
    mkv_->Set("Andra", {10, 11, 12, 13});
    mkv_->Set("Alex", {202, 203, 204, 205});
    mkv_->Delete("Robert");
    std::unordered_map<std::string, std::vector<uint8_t>> initial{mkv_->GetAllKeyValues()};
    TearDown();
    std::shared_ptr<MiniKeyValue> mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), initial);
}