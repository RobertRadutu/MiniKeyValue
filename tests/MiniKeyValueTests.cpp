#include <gtest/gtest.h>
#include "../include/MiniKeyValue.hpp"
#include "../include/FileStorage.hpp"
#include <memory>
#include <string>

struct MiniKeyValueTests : public testing::Test {
    void SetUp() override {
        mkv_ = MiniKeyValue::createEmpty(logFile_);
    }

    void TearDown() override {}

protected:
    std::shared_ptr<MiniKeyValue> mkv_;
    std::string logFile_{"data.in"};
};

TEST_F(MiniKeyValueTests, addEntryInStore) {
    mkv_->Set("Iulian", {200, 220, 61});
    const std::vector<uint8_t> expected = {200, 220, 61};
    auto result = mkv_->Get("Iulian");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), expected);
}

TEST_F(MiniKeyValueTests, getNonExistentKeyReturnsNullopt) {
    auto result = mkv_->Get("ghost");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MiniKeyValueTests, deleteRemovesKey) {
    mkv_->Set("temp", {1, 2, 3});
    mkv_->Delete("temp");
    EXPECT_FALSE(mkv_->Get("temp").has_value());
}

TEST_F(MiniKeyValueTests, setOverwritesExistingKey) {
    mkv_->Set("key", {1});
    mkv_->Set("key", {2, 3});
    auto result = mkv_->Get("key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), (std::vector<uint8_t>{2, 3}));
}

TEST_F(MiniKeyValueTests, checkStoragePersistency) {
    mkv_->Set("Robert", {1, 2, 3, 4});
    mkv_->Set("Andra", {10, 11, 12, 13});
    mkv_->Set("Alex", {202, 203, 204, 205});
    mkv_->Delete("Robert");

    std::unordered_map<std::string, std::vector<uint8_t>> initial = mkv_->GetAllKeyValues();
    TearDown();

    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), initial);
}

TEST_F(MiniKeyValueTests, emptyStoreReturnsEmptyMap) {
    EXPECT_TRUE(mkv_->GetAllKeyValues().empty());
}

TEST_F(MiniKeyValueTests, deleteNonExistentKeyDoesNotThrow) {
    EXPECT_NO_THROW(mkv_->Delete("nonexistent"));
}

TEST_F(MiniKeyValueTests, manyOverwritesSurviveReopen) {
    for (int i = 0; i < 40; ++i) {
        mkv_->Set("k", {static_cast<uint8_t>(i % 256)});
    }
    const auto map = mkv_->GetAllKeyValues();
    TearDown();
    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), map);
}

TEST_F(MiniKeyValueTests, deleteAllKeysLeavesEmptyLog) {
    mkv_->Set("x", {1});
    mkv_->Delete("x");
    EXPECT_EQ(FileStorage(logFile_).sizeBytes(), 0u);
}

TEST_F(MiniKeyValueTests, manyKeysThenDeletesRemainConsistentAfterReopen) {
    for (int i = 0; i < 20; ++i) {
        mkv_->Set("k" + std::to_string(i), {static_cast<uint8_t>(i)});
    }
    for (int i = 0; i < 19; ++i) {
        mkv_->Delete("k" + std::to_string(i));
    }
    const auto map = mkv_->GetAllKeyValues();
    TearDown();
    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), map);
    ASSERT_TRUE(mkv2->Get("k19").has_value());
    EXPECT_EQ(mkv2->Get("k19")->front(), 19);
}
