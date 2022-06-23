// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <measurement.hpp>

#include "json_parser.hpp"

namespace qds_buffer::core::parsing {

using ValidationFunction = std::function<void(ParserEvent, Measurement&, const char*, size_t, const void*)>;
using ValidationStructure = std::vector<std::pair<std::string, ValidationFunction>>;

struct ParsingState {
    std::shared_ptr<std::vector<Measurement>> data_;

    ValidationFunction validator_;
    bool has_key_;
    bool current_element_completed_;

    ParsingState() : data_(std::make_shared<std::vector<Measurement>>()), has_key_(false), current_element_completed_(false) {}
};

class DataValidator {
 public:
    static void ParserCallback(void* state, ParserEvent event, const char* value_as_string, size_t len, const void* value);

 private:
    DataValidator();
    ~DataValidator();

    static void OnObjectBegin(ParsingState& state);
    static void OnObjectEnd(ParsingState& state);
    static void OnKey(ParsingState& state, const char* value, size_t len);
    static void OnValue(ParsingState& state, ParserEvent event, const char* value_as_string, size_t len, const void* value);

    static ValidationStructure BuildParseValidation();

    static void ThrowWrongTypeError(const std::string& key_name, const char* value, size_t len,
                                    ParserEvent event_actual, ParserEvent event_expected);
};

} // namespace
