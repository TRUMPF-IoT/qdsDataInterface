// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include "data_validator.hpp"

#include <regex>

#include <exception.hpp>

namespace qds_buffer::core::parsing {

void DataValidator::ParserCallback(void* state, ParserEvent event, const char* value_as_string, size_t len, const void* value) {
    if (state == nullptr) {
        throw ParsingException("state is null", "DataValidator::ParserCallback");
    }

    auto& parsing_state = *static_cast<ParsingState*>(state);

    switch (event) {
        case ParserEvent::kOnObjectBegin: {
            return OnObjectBegin(parsing_state);
        }
        case ParserEvent::kOnObjectEnd: {
            return OnObjectEnd(parsing_state);
        }
        case ParserEvent::kOnKey: {
            return OnKey(parsing_state, value_as_string, len);
        }
        default: {
            return OnValue(parsing_state, event, value_as_string, len, value);
        }
    }
}

void DataValidator::OnObjectBegin(ParsingState& state) {
    state.data_->emplace_back(Measurement{});
    state.current_element_completed_ = false;
}

void DataValidator::OnObjectEnd(ParsingState& state) {
    if (state.data_->empty() || state.current_element_completed_) {
        throw ParsingException("Invalid JSON", "DataValidator::OnObjectEnd");
    }

    auto& data = state.data_->back();
    if (data.name_.empty()) {
        throw ParsingException("Measurement missing NAME", "DataValidator::OnObjectEnd");
    }
    if (data.type_ == MeasurementType::kNotSet) {
        throw ParsingException("Measurement missing TYPE", "DataValidator::OnObjectEnd");
    }
    if (std::holds_alternative<std::monostate>(data.value_)) {
        throw ParsingException("Measurement missing VALUE", "DataValidator::OnObjectEnd");
    }

    try {
        switch (data.type_) {
            case MeasurementType::kString: {
                (void)std::get<std::string>(data.value_);
                break;
            }
            case MeasurementType::kInteger: {
                std::int64_t value = std::get<std::int64_t>(data.value_);
                if (value > std::numeric_limits<std::int32_t>::max()) {
                    throw ParsingException("Invalid INTEGER value '" + std::to_string(value) + "'", "DataValidator::OnObjectEnd");
                }
                break;
            }
            case MeasurementType::kLong: {
                (void)std::get<std::int64_t>(data.value_);
                break;
            }
            case MeasurementType::kFloat: {
                double value = std::get<double>(data.value_);
                if (value > std::numeric_limits<float>::max()) {
                    throw ParsingException("Invalid FLOAT value '" + std::to_string(value) + "'", "DataValidator::OnObjectEnd");
                }
                break;
            }
            case MeasurementType::kDouble: {
                (void)std::get<double>(data.value_);
                break;
            }
            case MeasurementType::kBool: {
                (void)std::get<bool>(data.value_);
                break;
            }
            case MeasurementType::kWord: {
                std::string value = std::get<std::string>(data.value_);
                if (value.size() != 4 || value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
                    throw ParsingException("Invalid WORD value '" + value + "'", "DataValidator::OnObjectEnd");
                }
                break;
            }
            case MeasurementType::kTimestamp: {
                std::string value = std::get<std::string>(data.value_);

                // see https://stackoverflow.com/a/28022901
                static const std::regex iso_8601_regex(
                        "^(?:[1-9]\\d{3}(-?)(?:(?:0[1-9]|1[0-2])\\1(?:0[1-9]|1\\d|2[0-8])|(?:0[13-9]|1[0-2])\\1(?:29|30)"
                        "|(?:0[13578]|1[02])(?:\\1)31|00[1-9]|0[1-9]\\d|[12]\\d{2}|3(?:[0-5]\\d|6[0-5]))|(?:[1-9]\\d(?:0"
                        "[48]|[2468][048]|[13579][26])|(?:[2468][048]|[13579][26])00)(?:(-?)02(?:\\2)29|-?366))T(?:[01]"
                        "\\d|2[0-3])(:?)[0-5]\\d(?:\\3[0-5]\\d)?"
                        "(\\.?\\d{1,6})?"
                        "(?:Z|[+-][01]\\d(?:\\3[0-5]\\d)?)$");

                if (!std::regex_match(value, iso_8601_regex)) {
                    throw ParsingException("Invalid TIMESTAMP value '" + value + "'", "DataValidator::OnObjectEnd");
                }

                // follow API recommendation of moving timestamp entry to the front of the list;
                // note that 'data' will not be pointing to the correct entry anymore after this operation,
                // however, 'data' will not be accessed anymore, so it's not critical to update
                std::rotate(state.data_->begin(), state.data_->end()-1, state.data_->end());

                break;
            }
            case MeasurementType::kRef: {
                (void)std::get<std::string>(data.value_);
                break;
            }
            case MeasurementType::kForeignKey: {
                (void)std::get<std::string>(data.value_);
                break;
            }
            default: {
                throw ParsingException("Measurement has bad TYPE", "DataValidator::OnObjectEnd");
            }
        }
    } catch (const std::bad_variant_access&) {
        throw ParsingException("VALUE of '" + data.name_ + "' does not match its TYPE", "DataValidator::OnObjectEnd");
    }

    state.current_element_completed_ = true;
}

void DataValidator::OnKey(ParsingState& state, const char* value, size_t len) {
    static const ValidationStructure validation = BuildParseValidation();

    if (state.data_->empty() || state.current_element_completed_) {
        throw ParsingException("Entry '" + std::string(value, len) + "' is not an object", "DataValidator::OnKey");
    }

    for (auto& entry : validation) {
        if (strncmp(entry.first.c_str(), value, len) == 0) {
            state.has_key_ = true;
            state.validator_ = entry.second;
            return;
        }
    }

    throw ParsingException("Invalid key '" + std::string(value, len) + "'", "DataValidator::OnKey");
}

void DataValidator::OnValue(ParsingState& state, ParserEvent event,
                               const char* value_as_string, size_t len, const void* value) {
    if (state.data_->empty() || state.current_element_completed_) {
        throw ParsingException("Entry '" + std::string(value_as_string, len) + "' is not an object", "DataValidator::OnValue");
    }

    auto& data = state.data_->back();

    if (!state.has_key_) {
        throw ParsingException("Missing key for value '" + std::string(value_as_string, len) + "'", "DataValidator::OnValue");
    }

    if (state.validator_) {
        state.validator_(event, data, value_as_string, len, value);
    }

    state.has_key_ = false;
    state.validator_ = nullptr;
}

ValidationStructure DataValidator::BuildParseValidation() {
    return {
        ////////////////////////// NAME /////////////////////////////////
        {"NAME", [](ParserEvent event, Measurement& data, const char* value_as_string, size_t len, const void*) {
            if (event != ParserEvent::kOnString) {
                ThrowWrongTypeError("NAME", value_as_string, len, event, ParserEvent::kOnString);
            }
            if (!data.name_.empty()) {
                throw ParsingException("Duplicate NAME key", "DataValidator::BuildParseValidation");
            }
            data.name_ = std::string(value_as_string, len);
            return true;
        }},
        ////////////////////////// TYPE /////////////////////////////////
        {"TYPE", [](ParserEvent event, Measurement& data, const char* value_as_string, size_t len, const void*) {
            if (event != ParserEvent::kOnString) {
                ThrowWrongTypeError("TYPE", value_as_string, len, event, ParserEvent::kOnString);
            }
            if (data.type_ != MeasurementType::kNotSet) {
                throw ParsingException("Duplicate TYPE key", "DataValidator::BuildParseValidation");
            }

            data.SetTypeFromString(std::string(value_as_string, len));
            if (data.type_ == MeasurementType::kNotSet) {
                throw ParsingException("Invalid TYPE value '" + std::string(value_as_string, len) + "'",
                                       "DataValidator::BuildParseValidation");
            }
        }},
        ////////////////////////// UNIT /////////////////////////////////
        {"UNIT", [](ParserEvent event, Measurement& data, const char* value_as_string, size_t len, const void*) {
            if (event != ParserEvent::kOnString) {
                ThrowWrongTypeError("UNIT", value_as_string, len, event, ParserEvent::kOnString);
            }
            if (!data.unit_.empty()) {
                throw ParsingException("Duplicate UNIT key", "DataValidator::BuildParseValidation");
            }
            data.unit_ = std::string(value_as_string, len);
        }},
        ////////////////////////// VALUE /////////////////////////////////
        {"VALUE", [](ParserEvent event, Measurement& data, const char*, size_t len, const void* value) {
            if (value == nullptr) {
                throw ParsingException("value is null", "DataValidator::BuildParseValidation");
            }
            if (!std::holds_alternative<std::monostate>(data.value_)) {
                throw ParsingException("Duplicate VALUE key", "DataValidator::BuildParseValidation");
            }

            switch (event) {
                case ParserEvent::kOnString: {
                    data.value_ = std::string(static_cast<const char*>(value), len);
                    break;
                }
                case ParserEvent::kOnInt64:
                case ParserEvent::kOnUint64: {
                    data.value_ = *static_cast<const std::int64_t*>(value); // note: currently no unsigned values are supported
                    break;
                }
                case ParserEvent::kOnDouble: {
                    data.value_ = *static_cast<const double*>(value);
                    break;
                }
                case ParserEvent::kOnBool: {
                    data.value_ = *static_cast<const bool*>(value);
                    break;
                }
                default: {}
            }
        }},
        ////////////////////////// DECIMALS /////////////////////////////////
        {"DECIMALS", [](ParserEvent, Measurement&, const char*, size_t, const void*) {
            // legacy key set by VisionLine, ignore
            return true;
        }}
    };
}

void DataValidator::ThrowWrongTypeError(const std::string& key_name, const char* value, size_t len,
                             ParserEvent event_actual, ParserEvent event_expected) {
    static std::map<ParserEvent, std::string> event_map = {
            {ParserEvent::kOnString, "string"},
            {ParserEvent::kOnInt64, "int64_t"},
            {ParserEvent::kOnUint64, "uint64_t"},
            {ParserEvent::kOnDouble, "double"},
            {ParserEvent::kOnBool, "bool"}
    };

    std::string msg = key_name + " value '" + std::string(value, len) + "' has wrong type (" +
            event_map[event_actual] + "), should be " + event_map[event_expected];
    throw ParsingException(msg, "DataValidator::ThrowWrongTypeError");
}

} // namespace
