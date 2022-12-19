// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include "ring_buffer.hpp"

#include <exception.hpp>

namespace qds_buffer::core {

RingBuffer::RingBuffer(size_t size, int8_t counter_mode,  bool allow_overflow, OnDeleteCallbackType on_delete_callback)
    : kMaxSize_(size),
      kCounterMode_(counter_mode),
      kAllowOverflow_(allow_overflow),
      on_delete_callback_(on_delete_callback) {}

bool RingBuffer::Push(int64_t id, std::shared_ptr<std::vector<Measurement>> measurement) {
    std::unique_lock lock(mutex_);

    // discard old unlocked data
    if (buffer_.size() >= kMaxSize_) {
        if (!kAllowOverflow_) {
            throw RingBufferException("Data overflow", "RingBuffer::Push");
        }

        auto it = buffer_.begin();
        while (it < buffer_.end() && buffer_.size() >= kMaxSize_) {
            if (!it->locked_) {
                if (on_delete_callback_) {
                    on_delete_callback_(&buffer_.front(), false, GetCurrentTimeMs());
                }

                it = buffer_.erase(it);
            } else {
                ++it;
            }
        }

        if (buffer_.size() >= kMaxSize_) {
            // all data is locked, can't add new data
            return false;
        }
    }

    if (kCounterMode_ == 0) {
        // in CounterMode 0, only allow ids that are newer than the last element in the buffer
        if (!buffer_.empty() && buffer_.back().id_ >= id) {
            throw RingBufferException("Bad Id", "RingBuffer::Push");
        }
    } else {
        // in CounterMode 1, check if we already have an entry
        for (auto it = buffer_.begin(); it < buffer_.end(); ++it) {
            if (it->id_ == id) {
                // delete old entry if not yet locked
                if (!it->locked_) {
                    if (on_delete_callback_) {
                        on_delete_callback_(&*it, false, 0);
                    }

                    buffer_.erase(it);
                    break;
                }
                // no action if entry was locked
                return false;
            }
        }
    }

    buffer_.push_back(BufferEntry{id, measurement, GetCurrentTimeMs(), false});
    return true;
}

void RingBuffer::Delete(int64_t id) {
    std::unique_lock lock(mutex_);

    for (auto it = buffer_.begin(); it < buffer_.end(); ++it) {
        if (it->id_ == id) {
            if (on_delete_callback_) {
                on_delete_callback_(&*it, false, 0);
            }

            buffer_.erase(it);
            return;
        } else if (kCounterMode_ == 0 && it->id_ > id) {
            // in CounterMode 0, buffer entries are sorted, so we can stop iteration here
            break;
        }
    }

    // not found, but treat as success
}

ResetInformation RingBuffer::Reset(ResetReason reason) {
    std::unique_lock lock(mutex_);

    if (buffer_.empty()) return {0, ResetReason::UNKNOWN, 0, 0, 0};

    if (on_delete_callback_) {
        on_delete_callback_(0, true, 0);
    }

    uint64_t reset_time_ms = GetCurrentTimeMs();
    uint64_t oldest_dataset_time_ms = buffer_.front().timestamp_ms_;
    uint64_t newest_dataset_time_ms = buffer_.back().timestamp_ms_;
    uint32_t deleted_datasets_count = buffer_.size();

    buffer_.clear();

    return {reset_time_ms, reason, oldest_dataset_time_ms, newest_dataset_time_ms, deleted_datasets_count};
}

std::shared_mutex& RingBuffer::GetSharedMutex() const {
    return mutex_;
}

BufferQueueType::iterator RingBuffer::begin() {
    // must lock mutex via GetSharedMutex() before calling begin() and end()
    return buffer_.begin();
}

BufferQueueType::iterator RingBuffer::end() {
    // must lock mutex via GetSharedMutex() before calling begin() and end()
    return buffer_.end();
}

size_t RingBuffer::GetSize() const {
    std::shared_lock lock(mutex_);

    return buffer_.size();
}

size_t RingBuffer::GetMaxSize() const {
    return kMaxSize_;
}

int64_t RingBuffer::GetLastId() const {
    std::shared_lock lock(mutex_);

    return !buffer_.empty() ? buffer_.back().id_ : -1;
}

int8_t RingBuffer::GetCounterMode() const {
    return kCounterMode_;
}

uint64_t RingBuffer::GetCurrentTimeMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace
