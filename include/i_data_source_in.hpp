// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string_view>

namespace qds_buffer::core {

/*
 * Interface exposes the input methods of a DataSource object
 */
class IDataSourceIn {
 public:
    virtual ~IDataSourceIn() = default;

    /*
     * Adds new QDS data to the buffer.
     *
     * @param id: ID (counter) of the QDS data set
     * @param json: QDS data (set of measurements) in JSON representation, e.g.
     *              [
     *                {
     *                  "NAME":"ProgramName",
     *                  "TYPE":"STRING",
     *                  "VALUE":"test"
     *                },
     *                {
     *                  "NAME":"ProgramNumber",
     *                  "TYPE":"INT",
     *                  "VALUE":1
     *                }
     *              ]
     *
     * @returns true: data added
     *          false: data not added
     *
     * @throws ParsingException, RefException, RingBufferException
     */
    virtual bool Add(int64_t id, std::string_view json) = 0;

    /*
     * Stores a new reference (REF data type)
     *
     * @param ref: reference name
     * @param data: binary data belonging to the reference
     * @param data_format: data format (e.g. bmp, jpg, xml)
     *
     * @throws RefException
     */
    virtual void SetReference(const std::string& ref, const std::string& data, const std::string& data_format) = 0;


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
