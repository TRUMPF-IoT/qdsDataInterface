// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace qds_buffer::core {

/*
 * Possible data types of a measurement
 */
enum class MeasurementType {
    kNotSet,
    kString,
    kInteger,
    kFloat,
    kLong,
    kDouble,
    kBool,
    kWord,
    kTimestamp,
    kRef,
    kForeignKey
};

/*
 * Stores a QDS measurement and offers helper methods for type conversion and serialization
 */
struct Measurement {
    std::string name_;                                                              // name of the measurement
    MeasurementType type_ = MeasurementType::kNotSet;                               // type of the measurement
    std::string unit_;                                                              // unit of the measurement
    std::variant<std::monostate, std::string, std::int64_t, double, bool> value_;   // value of the measurement

    /*
     * Converts measurement type to string
     */
    std::string TypeToString() const {
        static const std::map<MeasurementType, std::string> types_map = {
                {MeasurementType::kString, "STRING"},
                {MeasurementType::kInteger, "INTEGER"},
                {MeasurementType::kFloat, "FLOAT"},
                {MeasurementType::kLong, "LONG"},
                {MeasurementType::kDouble, "DOUBLE"},
                {MeasurementType::kBool, "BOOL"},
                {MeasurementType::kWord, "WORD"},
                {MeasurementType::kTimestamp, "TIMESTAMP"},
                {MeasurementType::kRef, "REF"},
                {MeasurementType::kForeignKey, "FOREIGN_KEY"}
        };
        auto it = types_map.find(type_);
        if (it == types_map.end()) {
            return "";
        }
        return it->second;
    }

    /*
     * Converts string to measurement type
     */
    void SetTypeFromString(const std::string& type) {
        static const std::map<std::string, MeasurementType> types_map = {
            {"STRING", MeasurementType::kString},
            {"INTEGER", MeasurementType::kInteger},
            {"INT", MeasurementType::kInteger},
            {"FLOAT", MeasurementType::kFloat},
            {"LONG", MeasurementType::kLong},
            {"DOUBLE", MeasurementType::kDouble},
            {"BOOL", MeasurementType::kBool},
            {"WORD", MeasurementType::kWord},
            {"TIMESTAMP", MeasurementType::kTimestamp},
            {"REF", MeasurementType::kRef},
            {"FOREIGN_KEY", MeasurementType::kForeignKey}
        };
        auto it = types_map.find(type);
        if (it == types_map.end()) {
            type_ = MeasurementType::kNotSet;
        } else {
            type_ = it->second;
        }
    }

    /*
     * Converts measurement value to string
     */
    std::string ValueToString() const {
        return std::visit(VariantValueAsString{}, value_); // @suppress("Invalid arguments")
    }

    /*
     * Serializes a set of measurements to a JSON string
     */
    static std::string ToJson(const std::vector<Measurement>& list) {
        std::stringstream ss;
        bool first = true;
        ss << "[";
        for (auto& data : list) {
            if (!first) {
                ss << ",";
            }
            ss << "{";
            ss << "\"NAME\":\"" << data.name_ << "\"";
            ss << ",\"TYPE\":\"" << data.TypeToString() << "\"";
            if (!data.unit_.empty()) {
                ss << ",\"UNIT\":\"" << data.unit_ << "\"";
            }
            ss << ",\"VALUE\":" << data.ValueToString();
            ss << "}";

            first = false;
        }
        ss << "]";

        return ss.str();
    }

 private:
    /*
     * Helper struct for variant conversion
     */
    struct VariantValueAsString {
        std::string operator()(std::monostate) { return ""; }
        std::string operator()(const std::string& value) { return "\"" + value + "\""; }
        std::string operator()(std::int64_t value) { return std::to_string(value); }
        std::string operator()(double value) { return std::to_string(value); }
        std::string operator()(bool value) { return value ? "true" : "false"; }
    };
};

} // namespace
