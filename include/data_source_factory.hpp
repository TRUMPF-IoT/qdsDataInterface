// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

#include "declspec.hpp"
#include "i_data_source_in_out.hpp"

namespace qds_buffer::core {

/*
 * Offers factory methods for creation of a DataSource
 */
class DECLSPEC DataSourceFactory {
 public:
    /*
     * Creates a shared pointer to a DataSource object.
     *
     * The returned object can be accessed through the generic IDataSourceInOut interface (offers both input and output methods)
     * and can be cast to the more restricted IDataSourceIn/IDataSourceOut interfaces to limit access for special use cases such as
     * input-only or output-only.
     *
     * @param buffer_size: Size of the buffer (number of storable entries)
     * @param counter_mode: QDS counter mode (introduced in API 2.1)
     */
    static std::shared_ptr<IDataSourceInOut> CreateDataSource(size_t buffer_size = 100, int8_t counter_mode = 0);
};

} // namespace
