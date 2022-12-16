// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include "ring_buffer.hpp"
#include <exception.hpp>

using namespace qds_buffer::core;

#define DUMMY std::make_shared<std::vector<Measurement>>()

TEST(RingBufferTest, SimplePushRead) {
    RingBuffer buffer{100, 0};
    Measurement data;
    data.name_ = "SimplePushRead 0123";
    auto measurement = DUMMY;
    measurement->emplace_back(data);

    EXPECT_NO_THROW(buffer.Push(111, measurement));
    EXPECT_EQ(111, buffer.begin()->id_);
    EXPECT_EQ("SimplePushRead 0123", buffer.begin()->measurements_->front().name_);
}

TEST(RingBufferTest, ValidatePush) {
    RingBuffer buffer{100, 0};

    EXPECT_NO_THROW(buffer.Push(1, DUMMY));
    EXPECT_NO_THROW(buffer.Push(2, DUMMY));
    EXPECT_NO_THROW(buffer.Push(3, DUMMY));
    EXPECT_THROW(buffer.Push(2, DUMMY), RingBufferException); // Bad Id
    EXPECT_NO_THROW(buffer.Push(99, DUMMY));
    EXPECT_THROW(buffer.Push(50, DUMMY), RingBufferException); // Bad Id
}

TEST(RingBufferTest, RingbufferOverflow) {
    RingBuffer buffer{3, 0};

    EXPECT_NO_THROW(buffer.Push(1, DUMMY));
    EXPECT_EQ(1, buffer.GetSize());
    EXPECT_EQ(1, buffer.GetLastId());
    EXPECT_NO_THROW(buffer.Push(10, DUMMY));
    EXPECT_EQ(2, buffer.GetSize());
    EXPECT_EQ(10, buffer.GetLastId());
    EXPECT_NO_THROW(buffer.Push(50, DUMMY));
    EXPECT_EQ(3, buffer.GetSize());
    EXPECT_EQ(50, buffer.GetLastId());
    EXPECT_NO_THROW(buffer.Push(100, DUMMY));
    EXPECT_EQ(3, buffer.GetSize());
    EXPECT_EQ(100, buffer.GetLastId());
    EXPECT_NO_THROW(buffer.Push(500, DUMMY));
    EXPECT_EQ(3, buffer.GetSize());
    EXPECT_EQ(500, buffer.GetLastId());
}

TEST(RingBufferTest, Iterate) {
    RingBuffer buffer{100, 0};

    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY); buffer.Push(100, DUMMY); buffer.Push(500, DUMMY);

    auto it = buffer.begin();
    EXPECT_EQ(1, it->id_);
    ++it;
    EXPECT_EQ(10, it->id_);
    ++it;
    EXPECT_EQ(50, it->id_);
    ++it;
    EXPECT_EQ(100, it->id_);
    ++it;
    EXPECT_EQ(500, it->id_);
    ++it;
    EXPECT_EQ(buffer.end(), it);
}

TEST(RingBufferTest, Locked) {
    RingBuffer buffer{3, 0};

    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY);

    auto it = buffer.begin() + 1;
    it->locked_ = true; // lock entry '10'

    buffer.Push(100, DUMMY); buffer.Push(500, DUMMY); buffer.Push(1000, DUMMY);

    it = buffer.begin();
    EXPECT_EQ(10, it->id_);
    ++it;
    EXPECT_EQ(500, it->id_);
    it->locked_ = true; // lock entry '500'
    ++it;
    EXPECT_EQ(1000, it->id_);
    it->locked_ = true; // lock entry '1000'
    ++it;
    EXPECT_EQ(buffer.end(), it);

    buffer.Push(5000, DUMMY); buffer.Push(10000, DUMMY); buffer.Push(50000, DUMMY);

    it = buffer.begin();
    EXPECT_EQ(10, it->id_);
    ++it;
    EXPECT_EQ(500, it->id_);
    ++it;
    EXPECT_EQ(1000, it->id_);
    ++it;
    EXPECT_EQ(buffer.end(), it);
}

TEST(RingBufferTest, Delete) {
    RingBuffer buffer{100, 0};

    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY); buffer.Push(100, DUMMY); buffer.Push(500, DUMMY);
    EXPECT_EQ(5, buffer.GetSize());

    EXPECT_NO_THROW(buffer.Delete(1));
    EXPECT_EQ(4, buffer.GetSize());

    EXPECT_NO_THROW(buffer.Delete(100));
    EXPECT_NO_THROW(buffer.Delete(50));
    EXPECT_EQ(2, buffer.GetSize());

    auto it = buffer.begin();
    EXPECT_EQ(10, it->id_);
    ++it;
    EXPECT_EQ(500, it->id_);
    ++it;
    EXPECT_EQ(buffer.end(), it);
}

TEST(RingBufferTest, DeleteNotFound) {
    RingBuffer buffer{100, 0};

    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY); buffer.Push(100, DUMMY); buffer.Push(500, DUMMY);
    EXPECT_EQ(5, buffer.GetSize());

    EXPECT_NO_THROW(buffer.Delete(2));
    EXPECT_EQ(5, buffer.GetSize());

    EXPECT_NO_THROW(buffer.Delete(10));
    EXPECT_NO_THROW(buffer.Delete(11));
    EXPECT_NO_THROW(buffer.Delete(100));
    EXPECT_EQ(3, buffer.GetSize());
}

TEST(RingBufferTest, Reset) {
    RingBuffer buffer{100, 0};

    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY); buffer.Push(100, DUMMY); buffer.Push(500, DUMMY);
    EXPECT_EQ(5, buffer.GetSize());
    buffer.Reset(ResetReason::UNKNOWN);
    EXPECT_EQ(0, buffer.GetSize());
}

TEST(RingBufferTest, GetLastId) {
    RingBuffer buffer{100, 0};

    EXPECT_EQ(-1, buffer.GetLastId());
    buffer.Push(1, DUMMY); buffer.Push(10, DUMMY); buffer.Push(50, DUMMY); buffer.Push(100, DUMMY); buffer.Push(500, DUMMY);
    EXPECT_EQ(500, buffer.GetLastId());
}

TEST(RingBufferTest, OnDeleteCallback) {
    int64_t id_ = 0;
    bool clear_ = false;
    RingBuffer buffer{3, 0, [&](const BufferEntry* entry, bool clear, uint64_t){
        id_ = entry ? entry->id_ : 0;
        clear_ = clear;
    }};

    buffer.Push(1, DUMMY);
    buffer.Push(2, DUMMY);
    buffer.Push(3, DUMMY);

    EXPECT_EQ(0, id_);
    EXPECT_EQ(false, clear_);

    buffer.Push(4, DUMMY);
    EXPECT_EQ(1, id_);
    EXPECT_EQ(false, clear_);

    buffer.Push(5, DUMMY);
    EXPECT_EQ(2, id_);
    EXPECT_EQ(false, clear_);

    buffer.Delete(4);
    EXPECT_EQ(4, id_);
    EXPECT_EQ(false, clear_);

    buffer.Reset(ResetReason::UNKNOWN);
    EXPECT_EQ(true, clear_);
}

TEST(RingBufferTest, CounterMode) {
    {
        // mode 0
        RingBuffer buffer{100, 0};

        EXPECT_NO_THROW(buffer.Push(1, DUMMY));
        EXPECT_NO_THROW(buffer.Push(3, DUMMY));
        EXPECT_NO_THROW(buffer.Push(4, DUMMY));
        EXPECT_THROW(buffer.Push(2, DUMMY), RingBufferException); // Bad Id

        EXPECT_EQ(3, buffer.GetSize());
        EXPECT_NO_THROW(buffer.Delete(3));
        EXPECT_EQ(2, buffer.GetSize());
    }
    {
        // mode 1
        RingBuffer buffer{100, 1};

        Measurement data;

        auto measurement = DUMMY;
        data.name_ = "CounterMode 01";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(1, measurement));

        measurement = DUMMY;
        data.name_ = "CounterMode 03";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(3, measurement));

        measurement = DUMMY;
        data.name_ = "CounterMode 04";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(4, measurement));

        measurement = DUMMY;
        data.name_ = "CounterMode 02";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(2, measurement));

        EXPECT_EQ(4, buffer.GetSize());
        EXPECT_NO_THROW(buffer.Delete(3));
        EXPECT_EQ(3, buffer.GetSize());

        auto it = buffer.begin() + 1;
        EXPECT_EQ(4, it->id_);
        EXPECT_EQ("CounterMode 04", it->measurements_->front().name_);

        // update entry 4 (unlocked)
        measurement = DUMMY;
        data.name_ = "CounterMode 04b";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(4, measurement));

        EXPECT_EQ(3, buffer.GetSize());

        it = buffer.end() - 1;
        EXPECT_EQ(4, it->id_);
        EXPECT_FALSE(it->locked_);
        EXPECT_EQ("CounterMode 04b", it->measurements_->front().name_);

        // update entry 4 (locked)
        it->locked_ = true;
        measurement = DUMMY;
        data.name_ = "CounterMode 04c";
        measurement->emplace_back(data);
        EXPECT_NO_THROW(buffer.Push(4, measurement));

        EXPECT_EQ(3, buffer.GetSize());

        it = buffer.end() - 1;
        EXPECT_EQ(4, it->id_);
        EXPECT_TRUE(it->locked_);
        EXPECT_EQ("CounterMode 04b", it->measurements_->front().name_);
    }
}
