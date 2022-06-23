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
    bool locked_;                                            // indicates whether this entry is locked or not;
                                                             // a locked entry is not deleted if the buffer overflows
                                                             // or overridden with counter mode 1
};

/*
 * type of the buffer
 */
using BufferQueueType = std::deque<BufferEntry>;

}
