// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "i_data_source_in.hpp"
#include "i_data_source_out.hpp"

namespace qds_buffer {
    
    namespace core {

        /*
        * Combines the interfaces IDataSourceIn and IDataSourceOut
        */
        class IDataSourceInOut : public virtual IDataSourceIn, public virtual IDataSourceOut {
        public:
            virtual ~IDataSourceInOut() = default;
            // remove ambiguity of shared methods
            using IDataSourceIn::GetSize;
            using IDataSourceIn::GetMaxSize;
            using IDataSourceIn::GetLastId;
            using IDataSourceIn::GetCounterMode;
            using IDataSourceIn::GetAllowOverflow;

            virtual size_t GetDeletionInformationSize() const = 0;
            virtual size_t GetResetInformationSize() const = 0;
            virtual bool GetEnableMemoryInfoLogging() const = 0;
        };
    }
} // namespace
