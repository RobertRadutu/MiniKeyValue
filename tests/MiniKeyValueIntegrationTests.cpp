#include <gtest/gtest.h>
#include "../include/MiniKeyValue.hpp"
#include "../include/FileStorage.hpp"
#include <memory>
#include <minikvalue/include/LogEntry.hpp>
#include <string>

// ---------------------------------------------------------------------------
// Integration tests — MiniKeyValue backed by real FileStorage on disk.
// These verify persistence, replay, and compaction against the filesystem.
// ---------------------------------------------------------------------------

struct MkvIntegrationTests : public testing::Test {
    void SetUp() override {
        mkv_ = MiniKeyValue::createEmpty(logFile_);
    }

    void TearDown() override {}

protected:
    std::shared_ptr<MiniKeyValue> mkv_;
    std::string logFile_{"data.in"};
};

TEST_F(MkvIntegrationTests, persistencyAcrossReopen) {
    mkv_->Set("Robert", {1, 2, 3, 4});
    mkv_->Set("Andra", {10, 11, 12, 13});
    mkv_->Set("Alex", {202, 203, 204, 205});
    mkv_->Delete("Robert");

    const auto initial = mkv_->GetAllKeyValues();

    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), initial);
}

TEST_F(MkvIntegrationTests, manyOverwritesSurviveReopen) {
    for (int i = 0; i < 40; ++i) {
        mkv_->Set("k", {static_cast<uint8_t>(i % 256)});
    }
    const auto map = mkv_->GetAllKeyValues();

    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), map);
}

TEST_F(MkvIntegrationTests, deleteAllKeysLeavesEmptyLog) {
    mkv_->Set("x", {1});
    mkv_->Delete("x");
    EXPECT_EQ(FileStorage(logFile_).sizeBytes(), 0u);
}

TEST_F(MkvIntegrationTests, manyKeysThenDeletesRemainConsistentAfterReopen) {
    for (int i = 0; i < 20; ++i) {
        mkv_->Set("k" + std::to_string(i), {static_cast<uint8_t>(i)});
    }
    for (int i = 0; i < 19; ++i) {
        mkv_->Delete("k" + std::to_string(i));
    }
    const auto map = mkv_->GetAllKeyValues();

    auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues(), map);
    ASSERT_TRUE(mkv2->Get("k19").has_value());
    EXPECT_EQ(mkv2->Get("k19")->front(), 19);
}

TEST_F(MkvIntegrationTests, batchWriteInOneGo) {
    std::vector<LogEntry> batch{{"Robert", {1, 2, 3, 4}, 0},
                                {"Andra", {5, 6, 7, 8}, 0},
                                {"Robert", {}, true}};
    mkv_->BatchWrite(batch);
    const auto mkv2 = MiniKeyValue::createMiniKeyValue(logFile_);
    EXPECT_EQ(mkv2->GetAllKeyValues().begin()->first, "Andra");
    EXPECT_EQ(mkv2->GetAllKeyValues(), mkv_->GetAllKeyValues());
}
