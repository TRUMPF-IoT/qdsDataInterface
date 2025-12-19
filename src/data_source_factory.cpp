// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include <data_source_factory.hpp>

#include "data_source_internal.hpp"

namespace qds_buffer {
    
    namespace core {

        std::shared_ptr<IDataSourceInOut> DataSourceFactory::CreateDataSource(size_t buffer_size, int8_t counter_mode, bool allow_overflow,
                                                                            size_t reset_information_size, size_t deletion_information_size,
                                                                            bool enable_memory_info_logging) {
            return std::make_shared<DataSourceInternal>(buffer_size, counter_mode, allow_overflow,
                                                        reset_information_size, deletion_information_size, enable_memory_info_logging);
        }
    } //namespace core
} // namespace qds_buffer
