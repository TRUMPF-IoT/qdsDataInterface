// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include "data_source_internal.hpp"

#include <boost/thread.hpp>
#include <exception.hpp>
#include <fstream>
#include <unordered_set>

#include "parsing/data_validator.hpp"

namespace qds_buffer {

namespace core {

using namespace std::placeholders;

DataSourceInternal::DataSourceInternal(size_t buffer_size, int8_t counter_mode, bool allow_overflow, size_t reset_information_size,
                                       size_t deletion_information_size)
    : parser_(std::bind(&parsing::DataValidator::ParserCallback, _1, _2, _3, _4, _5)),  // @suppress("Symbol is not resolved")
      buffer_(buffer_size, counter_mode, allow_overflow,                                // @suppress("Symbol is not resolved")
              std::bind(&DataSourceInternal::OnDeleteCallback, this, _1, _2, _3)),
      ref_counter_(0),
      kResetInformationSize_(reset_information_size),
      kDeletionInformationSize_(deletion_information_size) {
    reset_information_list_.exceeded_max_entries_ = false;
    deletion_information_list_.exceeded_max_entries_ = false;
}

/**
 * IDataSourceIn methods
 */

int DataSourceInternal::Add(int64_t id, boost::json::string_view json) {
    parsing::ParsingState state;

    auto jsonTuple = parser_.Parse(json, &state);
    bool ok = std::get<0>(jsonTuple);
    std::string error_msg = std::get<1>(jsonTuple);

    if (!ok) {
        throw ParsingException(error_msg, "DataSourceInternal::Add");
    }
       

    auto measurement = state.data_;
    ProcessRefMapping(id, *measurement);
    int deletion_count = 0;

    try {
        deletion_count = buffer_.Push(id, measurement);
        if (deletion_count < 0) {
            DeleteRefMapping(id, false);
        }
    } catch (...) {
        DeleteRefMapping(id, false);
        throw;
    }

    return deletion_count;
}

void DataSourceInternal::SetReference(const std::string& ref, const std::string& data, const std::string& data_format) {
    boost::unique_lock<boost::shared_mutex> lock(ref_mapping_mutex_);

    auto&& view = ref_mapping_.get<multi_index_tag::ref>();
    if (view.find(ref) != view.end()) {
        throw RefException("Reference " + ref + " exists already", "DataSourceInternal::SetRef");
    }

    // id = 0, it will get updated once the measurement arrives
    // ref_mapping_.emplace(ReferenceData{0, ref, data_format, data}); // @suppress("Symbol is not resolved")

    ref_mapping_.emplace(ReferenceData{0, ref, data_format, data});  // @suppress("Symbol is not resolved")
}

void DataSourceInternal::Reset(ResetReason reason) {
    boost::unique_lock<boost::shared_mutex> lock(reset_information_list_mutex_);
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

/**
 * IDataSourceOut methods
 */

void DataSourceInternal::Delete(int64_t id) { buffer_.Delete(id); }

bool DataSourceInternal::IsReset() const {
    boost::shared_lock<boost::shared_mutex> lock(reset_information_list_mutex_);

    return !reset_information_list_.list_.empty();
}

ResetInformationList DataSourceInternal::AcknowledgeReset() {
    boost::unique_lock<boost::shared_mutex> lock(reset_information_list_mutex_);

    // create a copy to return
    ResetInformationList list = reset_information_list_;

    // clear member
    reset_information_list_ = {{}, false};

    return list;
}

bool DataSourceInternal::IsOverflown() const {
    boost::shared_lock<boost::shared_mutex> lock(deletion_information_list_mutex_);

    return !deletion_information_list_.list_.empty();
}

DeletionInformationList DataSourceInternal::AcknowledgeOverflow() {
    boost::unique_lock<boost::shared_mutex> lock(deletion_information_list_mutex_);

    // create a copy to return
    DeletionInformationList list = deletion_information_list_;

    // clear member
    deletion_information_list_ = {{}, false};

    return list;
}
bool DataSourceInternal::GetAllowOverflow() const { return buffer_.GetAllowOverflow(); }
boost::shared_mutex& DataSourceInternal::GetBufferSharedMutex() const { return buffer_.GetSharedMutex(); }

BufferQueueType::iterator DataSourceInternal::begin() {
    // must lock mutex via GetBufferSharedMutex() before calling begin() and end()
    return buffer_.begin();
}

BufferQueueType::iterator DataSourceInternal::end() {
    // must lock mutex via GetBufferSharedMutex() before calling begin() and end()
    return buffer_.end();
}

const ReferenceData& DataSourceInternal::GetReference(const std::string& ref) const {
    boost::shared_lock<boost::shared_mutex> lock(ref_mapping_mutex_);

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

size_t DataSourceInternal::GetSize() const { return buffer_.GetSize(); }

size_t DataSourceInternal::GetMaxSize() const { return buffer_.GetMaxSize(); }

int64_t DataSourceInternal::GetLastId() const { return buffer_.GetLastId(); }

int8_t DataSourceInternal::GetCounterMode() const { return buffer_.GetCounterMode(); }
size_t DataSourceInternal::GetDeletionInformationSize() const { return kDeletionInformationSize_; } 
size_t DataSourceInternal::GetResetInformationSize() const { return kResetInformationSize_; }

/**
 * private methods
 */

void DataSourceInternal::OnDeleteCallback(const BufferEntry* entry, bool clear, uint64_t timestamp_ms) {
    int64_t id = 0;
    if (entry) {
        boost::unique_lock<boost::shared_mutex> lock(deletion_information_list_mutex_);
        id = entry->id_;

        auto& list = deletion_information_list_.list_;
        list.emplace_back(DeletionInformation{timestamp_ms, entry->timestamp_ms_});

        if (list.size() > kDeletionInformationSize_) {
            // list has overflown, delete oldest information
            list.pop_front();
            deletion_information_list_.exceeded_max_entries_ = true;
        }
    }

    DeleteRefMapping(id, clear);
}

void DataSourceInternal::ProcessRefMapping(int64_t id, std::vector<Measurement>& data) {
    for (auto& d : data) {
        if (d.type_ == MeasurementType::kRef) {
            boost::unique_lock<boost::shared_mutex> lock(ref_mapping_mutex_);

            auto&& view = ref_mapping_.get<multi_index_tag::ref>();
            const std::string& value = boost::get<std::string>(d.value_);

            auto it = view.find(value);
            if (it == view.end()) {
#ifdef _MSC_VER
                FILE* file;
                if (fopen_s(&file, value.c_str(), "r") == 0) {
#else
                if (FILE* file = fopen(value.c_str(), "r")) {
#endif
                    fclose(file);
                } else {
                    throw RefException("The reference of '" + d.name_ + "' is neither an existing file, nor an existing reference",
                                       "DataSourceInternal::ProcessRefMapping");
                }
            } else {
                // A valid reference exists, update id and return
                if (it->id_ == 0) {
                    view.modify(it, [id](ReferenceData& data) { data.id_ = id; });
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
            std::ifstream ifs(path, std::ios::binary | std::ios::ate);
            if (!ifs) {
                throw FileIoException("Could not open file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            auto end = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            auto size = std::size_t(end - ifs.tellg());
            if (size == 0) {
                throw FileIoException("File size is 0 bytes", "DataSourceInternal::ProcessRefMapping");
            }

            std::string buffer;
            buffer.resize(size);
            if (!ifs.read((char*)buffer.data(), size)) {
                throw FileIoException("Could not read from file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            ifs.close();

            // delete file
            if (std::remove(path.c_str()) != 0) {
                throw FileIoException("Could not delete file " + path, "DataSourceInternal::ProcessRefMapping");
            }

            // ref_mapping_.emplace(ReferenceData{id, ref, format, buffer}); // @suppress("Symbol is not resolved")
            ref_mapping_.emplace(ReferenceData{id, ref, format, buffer});  // @suppress("Symbol is not resolved")

            // replace original ref value
            d.value_ = ref;
        }
    }
}

void DataSourceInternal::DeleteRefMapping(int64_t id, bool clear) {
    boost::unique_lock<boost::shared_mutex> lock(ref_mapping_mutex_);

    if (clear) {
        ref_mapping_.clear();
    } else {
        auto&& id_view = ref_mapping_.get<multi_index_tag::id>();
        auto range = id_view.equal_range(id);
        id_view.erase(range.first, range.second);
    }
}
}  // namespace core
}  // namespace qds_buffer
