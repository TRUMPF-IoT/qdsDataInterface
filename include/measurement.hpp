// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/json.hpp>
#include <boost/variant.hpp>

namespace qds_buffer { 
    
    namespace core {

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
            boost::variant<boost::blank, std::string, std::int64_t, double, bool> value_;   // value of the measurement

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
                return boost::apply_visitor(VariantValueAsString{}, value_);// @suppress("Invalid arguments")
            }

            /*
            * Serializes a set of measurements to a JSON string
            * this function was reviewed because json control characters in value string were not escaped
            */
            static std::string ToJson(const std::vector<Measurement>& list) {
                boost::json::array measurementArray;
                for (auto& data : list) {
                    boost::json::object measurementObj;
                    measurementObj["NAME"] = data.name_;
                    measurementObj["TYPE"] = data.TypeToString();
                    if (!data.unit_.empty()) {
                        measurementObj["UNIT"] = data.unit_;
                    }
                    measurementObj["VALUE"] = data.ValueToString();
                    measurementArray.push_back(measurementObj);
                }
                return boost::json::serialize(measurementArray);
            }

        private:
            /*
            * Helper struct for variant conversion
            */

            struct VariantValueAsString : public boost::static_visitor<std::string> {
                std::string operator()(boost::blank) const { return ""; }
                std::string operator()(const std::string& value) const { return value; }
                std::string operator()(std::int64_t value) const { return std::to_string(value); }
                std::string operator()(double value) const { return std::to_string(value); }
                std::string operator()(bool value) const { return value ? "true" : "false"; }
            };
        };

    } // namespace
}
