// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "i_data_source_in.hpp"
#include "i_data_source_out.hpp"

namespace qds_buffer::core {

/*
 * Combines the interfaces IDataSourceIn and IDataSourceOut
 */
class IDataSourceInOut : public virtual IDataSourceIn, public virtual IDataSourceOut {
 public:
    // remove ambiguity of shared methods
    using IDataSourceIn::GetSize;
    using IDataSourceIn::GetMaxSize;
    using IDataSourceIn::GetLastId;
    using IDataSourceIn::GetCounterMode;
};

} // namespace
