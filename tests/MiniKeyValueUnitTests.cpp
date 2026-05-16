#include <gtest/gtest.h>
#include "../include/MiniKeyValue.hpp"
#include "TestDoubles.hpp"
#include <memory>

// ---------------------------------------------------------------------------
// Unit tests — MiniKeyValue logic backed by InMemoryStorage (no filesystem)
// ---------------------------------------------------------------------------

struct MkvUnitTests : public testing::Test {
    void SetUp() override {
        mkv_ = MiniKeyValue::createFromStorage(std::make_unique<InMemoryStorage>());
    }

protected:
    std::shared_ptr<MiniKeyValue> mkv_;
};

TEST_F(MkvUnitTests, setAndGet) {
    mkv_->Set("alpha", {10, 20, 30});
    auto result = mkv_->Get("alpha");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (std::vector<uint8_t>{10, 20, 30}));
}

TEST_F(MkvUnitTests, getNonExistentKeyReturnsNullopt) {
    EXPECT_FALSE(mkv_->Get("ghost").has_value());
}

TEST_F(MkvUnitTests, deleteRemovesKey) {
    mkv_->Set("temp", {1, 2, 3});
    mkv_->Delete("temp");
    EXPECT_FALSE(mkv_->Get("temp").has_value());
}

TEST_F(MkvUnitTests, setOverwritesExistingKey) {
    mkv_->Set("k", {1});
    mkv_->Set("k", {2, 3});
    auto result = mkv_->Get("k");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (std::vector<uint8_t>{2, 3}));
}

TEST_F(MkvUnitTests, emptyStoreReturnsEmptyMap) {
    EXPECT_TRUE(mkv_->GetAllKeyValues().empty());
}

TEST_F(MkvUnitTests, deleteNonExistentKeyDoesNotThrow) {
    EXPECT_NO_THROW(mkv_->Delete("nonexistent"));
}

TEST_F(MkvUnitTests, getAllKeyValuesReturnsFullSnapshot) {
    mkv_->Set("a", {1});
    mkv_->Set("b", {2});
    mkv_->Set("c", {3});
    mkv_->Delete("b");

    auto map = mkv_->GetAllKeyValues();
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(map.at("a"), (std::vector<uint8_t>{1}));
    EXPECT_EQ(map.at("c"), (std::vector<uint8_t>{3}));
    EXPECT_EQ(map.count("b"), 0u);
}

TEST_F(MkvUnitTests, manyOverwritesRetainLastValue) {
    for (int i = 0; i < 50; ++i) {
        mkv_->Set("k", {static_cast<uint8_t>(i)});
    }
    auto result = mkv_->Get("k");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1u);
    EXPECT_EQ(result->front(), 49);
}

TEST_F(MkvUnitTests, manyKeysThenDeletesLeaveSingleKey) {
    for (int i = 0; i < 20; ++i) {
        mkv_->Set("k" + std::to_string(i), {static_cast<uint8_t>(i)});
    }
    for (int i = 0; i < 19; ++i) {
        mkv_->Delete("k" + std::to_string(i));
    }
    auto map = mkv_->GetAllKeyValues();
    EXPECT_EQ(map.size(), 1u);
    ASSERT_TRUE(mkv_->Get("k19").has_value());
    EXPECT_EQ(mkv_->Get("k19")->front(), 19);
}

TEST_F(MkvUnitTests, batchWriteUnit) {
    std::vector<LogEntry> batch{{"Robert", {1, 2, 3, 4}, 0},
                                {"Andra", {5, 6, 7, 8}, 0},
                                {"Robert", {}, true}};
    mkv_->BatchWrite(batch);

}

// ---------------------------------------------------------------------------
// Error-handling tests — FailingStorage that throws on write
// ---------------------------------------------------------------------------

struct FailingStorageTests : public testing::Test {
    void SetUp() override {
        mkv_ = MiniKeyValue::createFromStorage(std::make_unique<FailingStorage>());
    }

protected:
    std::shared_ptr<MiniKeyValue> mkv_;
};

TEST_F(FailingStorageTests, setThrowsOnStorageFailure) {
    EXPECT_THROW(mkv_->Set("k", {1}), std::runtime_error);
}

TEST_F(FailingStorageTests, deleteThrowsOnStorageFailure) {
    EXPECT_THROW(mkv_->Delete("k"), std::runtime_error);
}

TEST_F(FailingStorageTests, storeRemainsReadableAfterSetFailure) {
    EXPECT_THROW(mkv_->Set("k", {1}), std::runtime_error);
    EXPECT_TRUE(mkv_->GetAllKeyValues().empty() || mkv_->Get("k").has_value());
}
