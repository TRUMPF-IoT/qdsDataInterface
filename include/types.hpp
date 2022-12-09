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
 * Stores information of a buffer reset
 */
struct ResetInformation {
    uint64_t reset_time_ms_;
    uint64_t oldest_dataset_time_ms_;
    uint64_t newest_dataset_time_ms_;
    uint32_t deleted_datasets_count_;
};

/*
 * Stores a list of reset information and a flag of whether or not the list has overflown
 */
struct ResetInformationList {
    std::deque<ResetInformation> list_;
    bool exceeded_max_entries_;
};

}
