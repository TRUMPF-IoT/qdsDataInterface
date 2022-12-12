// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include "data_source_internal.hpp"

#include <fstream>
#include <unordered_set>

#include <exception.hpp>

#include "parsing/data_validator.hpp"

namespace qds_buffer::core {

using namespace std::placeholders;

DataSourceInternal::DataSourceInternal(size_t buffer_size, int8_t counter_mode, size_t reset_information_size)
    : parser_(std::bind(&parsing::DataValidator::ParserCallback, _1, _2, _3, _4, _5)), // @suppress("Symbol is not resolved")
      buffer_(buffer_size, counter_mode, std::bind(&DataSourceInternal::DeleteRefMapping, this, _1, _2)), // @suppress("Symbol is not resolved")
      ref_counter_(0),
      kResetInformationSize_(reset_information_size) {

    reset_information_list_.exceeded_max_entries_ = false;
}

/**
 * IDataSourceIn methods
 */

bool DataSourceInternal::Add(int64_t id, std::string_view json) {
    parsing::ParsingState state;

    auto [ok, error_msg] = parser_.Parse(json, &state);
    if (!ok) {
        throw ParsingException(error_msg, "DataSourceInternal::Add");
    }

    auto measurement = state.data_;
    ProcessRefMapping(id, *measurement);

    ok = buffer_.Push(id, measurement);
    if (!ok) {
        DeleteRefMapping(id, false);
    }

    return ok;
}

void DataSourceInternal::SetReference(const std::string& ref, const std::string& data, const std::string& data_format) {
    std::unique_lock lock(ref_mapping_mutex_);

    auto&& view = ref_mapping_.get<multi_index_tag::ref>();
    if (view.find(ref) != view.end()) {
        throw RefException("Reference " + ref + " exists already", "DataSourceInternal::SetRef");
    }

    // id = 0, it will get updated once the measurement arrives
    ref_mapping_.emplace(ReferenceData{0, ref, data_format, data}); // @suppress("Symbol is not resolved")
}

/**
 * IDataSourceOut methods
 */

void DataSourceInternal::Delete(int64_t id) {
    buffer_.Delete(id);
}

void DataSourceInternal::Reset(ResetReason reason) {
    std::unique_lock lock(reset_information_list_mutex_);
    auto& list = reset_information_list_.list_;

    const auto& reset_information = list.emplace_back(buffer_.Reset(reason));

    if (reset_information.reset_time_ms_ == 0) {
        // this is an empty structure, remove it from the list
        list.pop_back();
    }

    if (list.size() > kResetInformationSize_) {
        // list has overflown, delete oldest information
        list.pop_front();
        reset_information_list_.exceeded_max_entries_ = true;
    }
}

bool DataSourceInternal::IsReset() const {
    std::shared_lock lock(reset_information_list_mutex_);

    return !reset_information_list_.list_.empty();
}

ResetInformationList DataSourceInternal::AcknowledgeReset() {
    std::unique_lock lock(reset_information_list_mutex_);

    // create a copy to return
    ResetInformationList list = reset_information_list_;

    // clear member
    reset_information_list_ = {{}, false};

    return list;
}

std::shared_mutex& DataSourceInternal::GetBufferSharedMutex() const {
    return buffer_.GetSharedMutex();
}

BufferQueueType::iterator DataSourceInternal::begin() {
    // must lock mutex via GetBufferSharedMutex() before calling begin() and end()
    return buffer_.begin();
}

BufferQueueType::iterator DataSourceInternal::end() {
    // must lock mutex via GetBufferSharedMutex() before calling begin() and end()
    return buffer_.end();
}

const ReferenceData& DataSourceInternal::GetReference(const std::string& ref) const {
    std::shared_lock lock(ref_mapping_mutex_);

    auto&& view = ref_mapping_.get<multi_index_tag::ref>();
    auto it = view.find(ref);
    if (it != view.end()) {
        return *it;
    }

    throw RefException("Reference " + ref + " not found", "DataSourceInternal::GetRef");
}

/**
 * shared methods
 */

size_t DataSourceInternal::GetSize() const {
    return buffer_.GetSize();
}

size_t DataSourceInternal::GetMaxSize() const {
    return buffer_.GetMaxSize();
}

int64_t DataSourceInternal::GetLastId() const {
    return buffer_.GetLastId();
}

int8_t DataSourceInternal::GetCounterMode() const {
    return buffer_.GetCounterMode();
}

/**
 * private methods
 */

void DataSourceInternal::ProcessRefMapping(int64_t id, std::vector<Measurement>& data) {
    for (auto& d : data) {
        if (d.type_ == MeasurementType::kRef) {
            std::unique_lock lock(ref_mapping_mutex_);

            auto&& view = ref_mapping_.get<multi_index_tag::ref>();
            const std::string& value = std::get<std::string>(d.value_);

            auto it = view.find(value);
            if (it == view.end()) {
#ifdef _MSC_VER
                FILE *file;
                if (fopen_s(&file, value.c_str(), "r") == 0) {
#else
                if (FILE *file = fopen(value.c_str(), "r")) {
#endif
                    fclose(file);
                } else {
                    throw RefException("The reference of '" + d.name_ + "' is neither an existing file, nor an existing reference",
                                       "DataSourceInternal::ProcessRefMapping");
                }
            } else {
                // A valid reference exists, update id and return
                if (it->id_ == 0) {
                    view.modify(it, [id](ReferenceData &data){ data.id_ = id; });
                } else {
                    throw RefException("The reference '" + value + "' is already in use", "DataSourceInternal::ProcessRefMapping");
                }
                return;
            }

            // file exists, we will replace it with our own ref-id
            std::string ref = "ref-" + std::to_string(ref_counter_++);

            std::string format = "unknown";
            auto file_extension_position = value.find_last_of(".");
            if (file_extension_position != std::string::npos) {
                format = value.substr(file_extension_position + 1);
            }

            // load file content
            const std::string& path = value;
            std::ifstream ifs(path, std::ios::binary|std::ios::ate);
            if (!ifs) {
                throw FileIoException("Could not open file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            auto end = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            auto size = std::size_t(end - ifs.tellg());
            if(size == 0) {
                throw FileIoException("File size is 0 bytes", "DataSourceInternal::ProcessRefMapping");
            }

            std::string buffer;
            buffer.resize(size);
            if(!ifs.read((char*)buffer.data(), size)) {
                throw FileIoException("Could not read from file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            ifs.close();

            // delete file
            if (std::remove(path.c_str()) != 0) {
                throw FileIoException("Could not delete file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            ref_mapping_.emplace(ReferenceData{id, ref, format, buffer}); // @suppress("Symbol is not resolved")

            // replace original ref value
            d.value_ = ref;
        }
    }
}

void DataSourceInternal::DeleteRefMapping(int64_t id, bool clear) {
    std::unique_lock lock(ref_mapping_mutex_);

    if (clear) {
        ref_mapping_.clear();
    } else {
        auto&& id_view = ref_mapping_.get<multi_index_tag::id>();
        for (auto it = id_view.find(id); it != id_view.end(); it = id_view.find(id)) {
            ref_mapping_.erase(it);
        }
    }
}

} // namespace
