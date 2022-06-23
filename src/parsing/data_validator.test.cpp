// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <exception.hpp>

#include "data_validator.hpp"

using namespace qds_buffer;
using namespace qds_buffer::core;
using namespace qds_buffer::core::parsing;

#define TEST_STRING(str) str, strlen(str)

TEST(DataValidatorTest, ParserCallbackStateNull) {
    EXPECT_THROW(DataValidator::ParserCallback(nullptr, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr), ParsingException); // state is null
}

TEST(DataValidatorTest, ParserCallbackStateNotNull) {
    ParsingState state;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr));
}

TEST(DataValidatorTest, OnObjectBegin) {
    ParsingState state;
    EXPECT_EQ(0, state.data_->size());
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    EXPECT_EQ(1, state.data_->size());
}

TEST(DataValidatorTest, OnObjectEndBasic) {
    ParsingState state;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid JSON

    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    state.data_->back().name_ = "my-name";
    state.data_->back().type_ = MeasurementType::kString;
    state.data_->back().value_ = std::string("my-value");

    EXPECT_FALSE(state.current_element_completed_);
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    EXPECT_TRUE(state.current_element_completed_);
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid JSON
    state.current_element_completed_ = false;

    state.data_->back().name_ = "";
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Measurement missing NAME
    state.data_->back().name_ = "my-name";
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Measurement missing TYPE
    state.data_->back().type_ = MeasurementType::kString;
    state.data_->back().value_ = std::monostate();
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Measurement missing VALUE
    state.data_->back().value_ = std::string("my-value");
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
}

TEST(DataValidatorTest, OnObjectEndPrimitiveTypes) {
    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);

    state.data_->back().name_ = "my-name";
    state.data_->back().type_ = MeasurementType::kString;
    state.data_->back().value_ = std::string("my-value");
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;

    state.data_->back().value_ = std::int64_t(123);
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().type_ = MeasurementType::kInteger;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = std::int64_t(std::numeric_limits<std::int64_t>::max()) - 1;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid INTEGER value
    state.data_->back().type_ = MeasurementType::kLong;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = 123.123;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().type_ = MeasurementType::kFloat;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = double(std::numeric_limits<double>::max()) - 1;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid FLOAT value
    state.data_->back().type_ = MeasurementType::kDouble;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().type_ = MeasurementType::kBool;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = std::string("A5E9");
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().type_ = MeasurementType::kWord;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().value_ = std::string("A5E91");
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid WORD value
    state.data_->back().value_ = std::string("A5G9");
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid WORD value
    state.data_->back().value_ = std::string("2019-02-18T13:29:43+02:00");
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // Invalid WORD value
    state.data_->back().type_ = MeasurementType::kTimestamp;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().type_ = MeasurementType::kRef;
    state.data_->back().value_ = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().value_ = std::string("my-ref");
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
    state.current_element_completed_ = false;
    state.data_->back().type_ = MeasurementType::kForeignKey;
    state.data_->back().value_ = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException); // VALUE of 'my-name' does not match its TYPE
    state.data_->back().value_ = std::string("my-foreign-key");
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr));
}

TEST(DataValidatorTest, OnObjectEndTimestamp) {
    const std::vector<std::string> invalid_timestamps = {
        "2019-02-18T13:29:43",
        "800-02-18T13:29:43+02:00",
        "2019-02-18T13:29:43Z+02:00",
        "2019-02-18T13:29:43+20:00",
        "2019-02-18Z13:29:43+02:00",
        "2019-02-18T13:29:43Z+02:00",
        "2019-02-18T13:29:43",
        "2019-02-18-13:29:43+02:00",
        "2019-2-18T13:29:43+02:00",
        "2019-02-18T24:29:43+02:00",
        "2019-13-18T13:29:43+02:00",
        "2019-02-30T13:29:43+02:00",
        "2019-02-18T13:60:43+02:00",
        "2019-02-18T13:29:60+02:00",
        "2019-02-18T13:29:43+02:60",
        "2019-02-18T13-29-43+02:00",
        "2019:02:18T13:29:43+02:00",
    };
    const std::vector<std::string> valid_timestamps = {
        "2019-02-18T13:29:43+02:00",
        "2019-02-18T13:29:43-02:00",
        "2019-02-18T13:29:43Z",
        "20190218T132943-0200",
        "20190218T132943Z",
    };

    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);

    state.data_->back().name_ = "my-timestamp";
    state.data_->back().type_ = MeasurementType::kTimestamp;

    for (auto& ts : invalid_timestamps) {
        state.data_->back().value_ = ts;
        state.current_element_completed_ = false;
        EXPECT_THROW(DataValidator::ParserCallback(
                &state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr), ParsingException) << "Timestamp: " << ts; // Invalid TIMESTAMP value
    }

    for (auto& ts : valid_timestamps) {
        state.data_->back().value_ = ts;
        state.current_element_completed_ = false;
        EXPECT_NO_THROW(DataValidator::ParserCallback(
                &state, ParserEvent::kOnObjectEnd, TEST_STRING(""), nullptr)) << "Timestamp: " << ts;
    }
}

TEST(DataValidatorTest, OnKey) {
    ParsingState state;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("abcd"), nullptr), ParsingException); // Entry 'abcd' is not an object
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("NAME"), nullptr), ParsingException); // Entry 'NAME' is not an object

    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, "", 0, nullptr);
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("abcd"), nullptr), ParsingException); // Invalid key 'abcd'

    EXPECT_FALSE(state.has_key_);
    EXPECT_EQ(nullptr, state.validator_);
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("NAME"), nullptr));
    EXPECT_TRUE(state.has_key_);
    EXPECT_NE(nullptr, state.validator_);

    state.has_key_ = false;
    state.validator_ = nullptr;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("TYPE"), nullptr));
    EXPECT_TRUE(state.has_key_);
    EXPECT_NE(nullptr, state.validator_);

    state.has_key_ = false;
    state.validator_ = nullptr;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("UNIT"), nullptr));
    EXPECT_TRUE(state.has_key_);
    EXPECT_NE(nullptr, state.validator_);

    state.has_key_ = false;
    state.validator_ = nullptr;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("VALUE"), nullptr));
    EXPECT_TRUE(state.has_key_);
    EXPECT_NE(nullptr, state.validator_);
}

TEST(DataValidatorTest, OnValueInvalid) {
    ParsingState state;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr), ParsingException); // Entry 'abcd' is not an object

    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr), ParsingException); // Missing key for value 'abcd'
}

TEST(DataValidatorTest, OnValueName) {
    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("NAME"), nullptr);
    auto validator = state.validator_;

    int64_t test_int64 = 123;
    uint64_t test_uint64 = 123;
    double test_double = 123.123;
    bool test_bool = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnInt64, TEST_STRING("abcd"), &test_int64), ParsingException); // NAME value 'abcd' has wrong type (int64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnUint64, TEST_STRING("abcd"), &test_uint64), ParsingException); // NAME value 'abcd' has wrong type (uint64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnDouble, TEST_STRING("abcd"), &test_double), ParsingException); // NAME value 'abcd' has wrong type (double), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnBool, TEST_STRING("abcd"), &test_bool), ParsingException); // NAME value 'abcd' has wrong type (bool), should be string

    EXPECT_EQ("", state.data_->back().name_);
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr));
    EXPECT_EQ("abcd", state.data_->back().name_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("zzzz"), nullptr), ParsingException); // Duplicate NAME key
}

TEST(DataValidatorTest, OnValueType) {
    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("TYPE"), nullptr);
    auto validator = state.validator_;

    int64_t test_int64 = 123;
    uint64_t test_uint64 = 123;
    double test_double = 123.123;
    bool test_bool = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnInt64, TEST_STRING("abcd"), &test_int64), ParsingException); // TYPE value 'abcd' has wrong type (int64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnUint64, TEST_STRING("abcd"), &test_uint64), ParsingException); // TYPE value 'abcd' has wrong type (uint64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnDouble, TEST_STRING("abcd"), &test_double), ParsingException); // TYPE value 'abcd' has wrong type (double), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnBool, TEST_STRING("abcd"), &test_bool), ParsingException); // TYPE value 'abcd' has wrong type (bool), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr), ParsingException); // Invalid TYPE value 'abcd'

    EXPECT_EQ(MeasurementType::kNotSet, state.data_->back().type_);
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr));
    EXPECT_EQ(MeasurementType::kString, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("INTEGER"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("INTEGER"), nullptr));
    EXPECT_EQ(MeasurementType::kInteger, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("INT"), nullptr));
    EXPECT_EQ(MeasurementType::kInteger, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("FLOAT"), nullptr));
    EXPECT_EQ(MeasurementType::kFloat, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("LONG"), nullptr));
    EXPECT_EQ(MeasurementType::kLong, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("DOUBLE"), nullptr));
    EXPECT_EQ(MeasurementType::kDouble, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("BOOL"), nullptr));
    EXPECT_EQ(MeasurementType::kBool, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("WORD"), nullptr));
    EXPECT_EQ(MeasurementType::kWord, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("TIMESTAMP"), nullptr));
    EXPECT_EQ(MeasurementType::kTimestamp, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("REF"), nullptr));
    EXPECT_EQ(MeasurementType::kRef, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
    state.data_->back().type_ = MeasurementType::kNotSet;
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("FOREIGN_KEY"), nullptr));
    EXPECT_EQ(MeasurementType::kForeignKey, state.data_->back().type_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("STRING"), nullptr), ParsingException); // Duplicate TYPE key
}

TEST(DataValidatorTest, OnValueUnit) {
    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("UNIT"), nullptr);
    auto validator = state.validator_;

    int64_t test_int64 = 123;
    uint64_t test_uint64 = 123;
    double test_double = 123.123;
    bool test_bool = true;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnInt64, TEST_STRING("abcd"), &test_int64), ParsingException); // UNIT value 'abcd' has wrong type (int64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnUint64, TEST_STRING("abcd"), &test_uint64), ParsingException); // UNIT value 'abcd' has wrong type (uint64_t), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnDouble, TEST_STRING("abcd"), &test_double), ParsingException); // UNIT value 'abcd' has wrong type (double), should be string
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnBool, TEST_STRING("abcd"), &test_bool), ParsingException); // UNIT value 'abcd' has wrong type (bool), should be string

    EXPECT_EQ("", state.data_->back().unit_);
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr));
    EXPECT_EQ("abcd", state.data_->back().unit_);

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("zzzz"), nullptr), ParsingException); // Duplicate UNIT key
}

TEST(DataValidatorTest, OnValueValue) {
    ParsingState state;
    DataValidator::ParserCallback(&state, ParserEvent::kOnObjectBegin, TEST_STRING(""), nullptr);
    DataValidator::ParserCallback(&state, ParserEvent::kOnKey, TEST_STRING("VALUE"), nullptr);
    auto validator = state.validator_;

    int64_t test_int64 = 123;
    uint64_t test_uint64 = 456;
    double test_double = 123.123;
    bool test_bool = true;

    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), nullptr), ParsingException); // value is null

    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnString, TEST_STRING("abcd"), "abcd"));
    EXPECT_EQ("abcd", std::get<std::string>(state.data_->back().value_));

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnInt64, TEST_STRING("abcd"), &test_int64), ParsingException); // Duplicate VALUE key
    state.data_->back().value_ = std::monostate();
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnInt64, TEST_STRING("abcd"), &test_int64));
    EXPECT_EQ(test_int64, std::get<std::int64_t>(state.data_->back().value_));

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnUint64, TEST_STRING("abcd"), &test_uint64), ParsingException); // Duplicate VALUE key
    state.data_->back().value_ = std::monostate();
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnUint64, TEST_STRING("abcd"), &test_uint64));
    EXPECT_EQ(test_uint64, std::get<std::int64_t>(state.data_->back().value_));

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnDouble, TEST_STRING("abcd"), &test_double), ParsingException); // Duplicate VALUE key
    state.data_->back().value_ = std::monostate();
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnDouble, TEST_STRING("abcd"), &test_double));
    EXPECT_EQ(test_double, std::get<double>(state.data_->back().value_));

    state.has_key_ = true;
    state.validator_ = validator;
    EXPECT_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnBool, TEST_STRING("abcd"), &test_bool), ParsingException); // Duplicate VALUE key
    state.data_->back().value_ = std::monostate();
    EXPECT_NO_THROW(DataValidator::ParserCallback(&state, ParserEvent::kOnBool, TEST_STRING("abcd"), &test_bool));
    EXPECT_EQ(test_bool, std::get<bool>(state.data_->back().value_));
}
