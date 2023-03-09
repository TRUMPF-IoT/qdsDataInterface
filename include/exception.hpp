// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <exception>
#include <string>

namespace qds_buffer {
    
    namespace core {

        /*
        * Generic Exception class with basic strings for 'message', 'scope', and a concatenated string 'what'
        */
        class Exception : public std::exception {
        public:
            Exception(const std::string& msg, const std::string& scope) : msg_(msg), scope_(scope), what_(msg + " [" + scope + "]") {}

            /*
            * Returns a string describing the cause of the error
            */
            const char* msg() const {
                return msg_.c_str();
            }

            /*
            * Returns a string describing the scope of the error
            */
            const char* scope() const {
                return scope_.c_str();
            }

            /*
            * Returns a formatted string combining message and scope
            */
            virtual const char* what() const noexcept override {
                return what_.c_str();
            }

        protected:
            std::string msg_;
            std::string scope_;
            std::string what_;
        };

        /*
        * Thrown if parsing of the JSON string results in an error.
        *
        * Parsing exceptions include (not a complete list):
        * - Invalid JSON
        * - Measurement missing NAME/TYPE/VALUE/UNIT
        * - Invalid INTEGER/FLOAT/WORD/TIMESTAMP value
        * - Invalid key
        * - Entry is not an object
        * - Missing key for value
        * - Duplicate NAME/TYPE/VALUE/UNIT key
        */
        class ParsingException : public Exception {
        public:
            ParsingException(const std::string& msg, const std::string& scope) : Exception(msg, scope) {}
        };

        /*
        * Thrown if manipulating the ring buffer (Push/Delete) results in an error.
        */
        class RingBufferException : public Exception {
        public:
            RingBufferException(const std::string& msg, const std::string& scope) : Exception(msg, scope) {}
        };

        /*
        * Thrown if processing a reference (REF data type) results in an error.
        */
        class RefException : public Exception {
        public:
            RefException(const std::string& msg, const std::string& scope) : Exception(msg, scope) {}
        };

        /*
        * Thrown if a file operation results in an error.
        */
        class FileIoException : public Exception {
        public:
            FileIoException(const std::string& msg, const std::string& scope) : Exception(msg, scope) {}
        };
    } // namespace core
} // namespace qds_buffer
