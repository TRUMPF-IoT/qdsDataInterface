// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include <data_source_factory.hpp>

#include "data_source_internal.hpp"

namespace qds_buffer::core {

std::shared_ptr<IDataSourceInOut> DataSourceFactory::CreateDataSource(size_t buffer_size, int8_t counter_mode, size_t reset_information_size) {
    return std::make_shared<DataSourceInternal>(buffer_size, counter_mode, reset_information_size);
}

} // namespace
