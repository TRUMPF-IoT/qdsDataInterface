// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once


#include <boost/json.hpp>
#include <boost/tuple/tuple.hpp>

// This file must be manually included when
// using basic_parser to implement a parser.
#include <boost/json/basic_parser_impl.hpp>
#include <boost/thread.hpp>


namespace qds_buffer { 
    
    namespace core { 
        
        namespace parsing {

            enum class ParserEvent {
                kOnObjectBegin,
                kOnObjectEnd,
                kOnKey,
                kOnString,
                kOnInt64,
                kOnUint64,
                kOnDouble,
                kOnBool
            };

            using ParserCallback = std::function<void(void*, ParserEvent, const char*, size_t, const void*)>;

            class JsonParser {
            public:
                JsonParser(ParserCallback parser_callback)
                    : parser_(SetOptions()),
                    parser_callback_(parser_callback) {

                    parser_.handler().setup(parser_callback);
                }

                std::tuple<bool, std::string> Parse(boost::json::string_view string, void* state) {
                    boost::unique_lock<boost::shared_mutex> lock(mutex_);

                    parser_.handler().set_state(state);

                    boost::json::error_code ec;
                    parser_.reset();
                    auto const n = parser_.write_some(false, string.data(), string.size(), ec);
                    parser_.reset();

                    parser_.handler().set_state(nullptr);

                    if(!ec && n < string.size()) {
                        ec = boost::json::error::extra_data;
                    }
                    if(ec) {
                        std::string error_msg = "Parsing error: " + ec.message();
                        return {false, error_msg};
                    }
                    return {true, ""};
                }

                struct handler {
                    constexpr static std::size_t max_object_size = std::size_t(-1);
                    constexpr static std::size_t max_array_size = std::size_t(-1);
                    constexpr static std::size_t max_key_size = std::size_t(-1);
                    constexpr static std::size_t max_string_size = std::size_t(-1);

                    void setup(ParserCallback parser_callback) {
                        parser_callback_ = parser_callback;
                    }

                    void set_state(void* state) {
                        state_ = state;
                    }

                    bool on_key(boost::json::string_view key, std::size_t, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnKey, key.data(), key.size(), nullptr);
                        return true;
                    }

                    bool on_string(boost::json::string_view value, std::size_t, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnString, value.data(), value.size(), value.data());
                        return true;
                    }

                    bool on_int64(std::int64_t val, boost::json::string_view value, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnInt64, value.data(), value.size(), &val);
                        return true;
                    }

                    bool on_uint64(std::uint64_t val, boost::json::string_view value, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnUint64, value.data(), value.size(), &val);
                        return true;
                    }

                    bool on_double(double val, boost::json::string_view value, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnDouble, value.data(), value.size(), &val);
                        return true;
                    }

                    bool on_bool(bool value, boost::json::error_code&) {
                        boost::json::string_view val = value ? "true" : "false";
                        parser_callback_(state_, ParserEvent::kOnBool, val.data(), val.size(), &value);
                        return true;
                    }

                    bool on_object_begin(boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnObjectBegin, nullptr, 0, nullptr);
                        return true;
                    }

                    bool on_object_end(std::size_t, boost::json::error_code&) {
                        parser_callback_(state_, ParserEvent::kOnObjectEnd, nullptr, 0, nullptr);
                        return true;
                    }

                    bool on_document_begin(boost::json::error_code&) { return true; }
                    bool on_document_end(boost::json::error_code&) { return true; }
                    bool on_array_begin(boost::json::error_code&) { return true; }
                    bool on_array_end(std::size_t, boost::json::error_code&) { return true; }
                    bool on_key_part(boost::json::string_view, std::size_t, boost::json::error_code&) { return true; }
                    bool on_string_part(boost::json::string_view, std::size_t, boost::json::error_code&) { return true; }
                    bool on_number_part(boost::json::string_view, boost::json::error_code&) { return true; }
                    bool on_comment_part(boost::json::string_view, boost::json::error_code&) { return true; }
                    bool on_comment(boost::json::string_view, boost::json::error_code&) { return true; }
                    bool on_null(boost::json::error_code&) { return false; }

                private:
                    void* state_;
                    ParserCallback parser_callback_;
                };

            private:
                static boost::json::parse_options SetOptions() {
                    boost::json::parse_options parse_options;
                    parse_options.max_depth = 2;
                    return parse_options;
                }

                mutable boost::shared_mutex mutex_;
                boost::json::basic_parser<handler> parser_;
                ParserCallback parser_callback_;
            };
        } // namespace parsing
    } // namespace core
} // namespace qds_buffer
