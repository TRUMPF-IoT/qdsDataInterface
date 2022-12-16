// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "measurement.hpp"

namespace qds_buffer::core {

/*
 * Stores a buffer entry
 */
struct BufferEntry {
    int64_t id_;                                             // ID (counter) of the set
    std::shared_ptr<std::vector<Measurement>> measurements_; // set of measurements (QDS data)
    uint64_t timestamp_ms_;                                  // timestamp of when this entry got added
    bool locked_;                                            // indicates whether this entry is locked or not;
                                                             // a locked entry is not deleted if the buffer overflows
                                                             // or overridden with counter mode 1
};

/*
 * type of the buffer
 */
using BufferQueueType = std::deque<BufferEntry>;

/**
 * Reset Reason
 */
enum class ResetReason {
    UNKNOWN,    // the reason for the reset is unknown
                // (e.g. as default value on the server side if an older client calls the reset method)
    SYSTEM,     // the reset is caused by the system/product (e.g. due to a restart)
    USER        // the reset is caused by a user action (e.g. due to a manual buffer reset action)
};

/**
 * Stores information about a buffer reset
 */
struct ResetInformation {
    uint64_t reset_time_ms_;            // ISO 8601 timestamp of the buffer reset
    ResetReason reset_reason;           // reason for the reset
    uint64_t oldest_dataset_time_ms_;   // ISO 8601 timestamp of the oldest deleted data set due to a buffer reset
    uint64_t newest_dataset_time_ms_;   // ISO 8601 timestamp of the newest deleted data set due to a buffer reset
    uint32_t deleted_datasets_count_;   // number of deleted data sets due to a buffer reset
};

/*
 * Wrapper struct
 */
struct ResetInformationList {
    std::deque<ResetInformation> list_; // list of reset information
    bool exceeded_max_entries_;         // flag of whether or not the list has overflown
};

/**
 * Stores information about an dataset deletion
 */
struct DeletionInformation {
    uint64_t deletion_time_ms_;    // ISO 8601 timestamp of the deletion
    uint64_t data_set_time_ms_;    // ISO 8601 timestamp of the deleted dataset
};

/*
 * Wrapper struct
 */
struct DeletionInformationList {
    std::deque<DeletionInformation> list_;  // list of deletion information
    bool exceeded_max_entries_;             // flag of whether or not the list has overflown
};

}
