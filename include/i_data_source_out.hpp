// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <shared_mutex>
#include <string_view>

#include "types.hpp"

namespace qds_buffer::core {

/*
 * Stores a reference data (REF) object
 */
struct ReferenceData {
    int64_t id_;            // data ID, to which the reference belongs (0, if not yet determined)
    std::string ref_;       // REF value

    bool is_path_;          // is this reference a path to a file?
    std::string format_;    // data format (e.g. bmp, jpg, xml)
    std::string content_;   // content, either a file path or the binary data itself
};

/*
 * Interface exposes the output methods of a DataSource object
 */
class IDataSourceOut {
 public:
    virtual ~IDataSourceOut() = default;

    /*
     * Deletes QDS data
     *
     * @param id: ID (counter) of the QDS data set to delete
     *
     * @throws RingBufferException
     */
    virtual void Delete(int64_t id) = 0;

    /*
     * Completely resets the buffer (deletes all data)
     */
    virtual void Reset() = 0;

    /*
     * @returns buffer mutex; use in combination with begin() and end() when iterating through the buffer
     */
    virtual std::shared_mutex& GetBufferSharedMutex() const = 0;
    /*
     * @returns iterator to the beginning of the buffer; must lock mutex via GetBufferSharedMutex() before iterating
     */
    virtual BufferQueueType::iterator begin() = 0;
    /*
     * @returns iterator to the end of the buffer; must lock mutex via GetBufferSharedMutex() before iterating
     */
    virtual BufferQueueType::iterator end() = 0;

    /*
     * @param ref: reference name
     *
     * @returns a reference data object (REF data type)
     *
     * @throws RefException
     */
    virtual const ReferenceData& GetReference(const std::string& ref) const = 0;


    ////////////////////////////////// shared methods //////////////////////////////////
    /*
     * @returns number of elements in the buffer
     */
    virtual size_t GetSize() const = 0;
    /*
     * @returns size of the buffer (number of storable entries)
     */
    virtual size_t GetMaxSize() const = 0;
    /*
     * @returns ID of the last stored QDS data
     */
    virtual int64_t GetLastId() const = 0;
    /*
     * @returns active counter mode
     */
    virtual int8_t GetCounterMode() const = 0;
};

} // namespace
