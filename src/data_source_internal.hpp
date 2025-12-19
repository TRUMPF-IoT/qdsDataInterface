// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <boost/json/string_view.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/thread.hpp>
#include <i_data_source_in_out.hpp>

#include "parsing/json_parser.hpp"
#include "ring_buffer.hpp"

namespace qds_buffer {

namespace core {

namespace multi_index_tag {
struct id {};
struct ref {};
}  // namespace multi_index_tag

using ReferenceContainer = boost::multi_index_container<
    ReferenceData, boost::multi_index::indexed_by<
                       boost::multi_index::hashed_non_unique<boost::multi_index::tag<multi_index_tag::id>,
                                                             boost::multi_index::member<ReferenceData, int64_t, &ReferenceData::id_> >,
                       boost::multi_index::hashed_unique<boost::multi_index::tag<multi_index_tag::ref>,
                                                         boost::multi_index::member<ReferenceData, std::string, &ReferenceData::ref_> > > >;

/**
 * Thread-Safe
 */
class DataSourceInternal : public IDataSourceInOut {
   public:
    DataSourceInternal(size_t buffer_size = 100, int8_t counter_mode = 0, bool allow_overflow = true, size_t reset_information_size = 100,
                       size_t deletion_information_size = 100,bool enable_memory_info_logging = false);
    virtual ~DataSourceInternal();

    // IDataSourceIn methods
    virtual int Add(int64_t id, boost::json::string_view json) override;
    virtual void SetReference(const std::string& ref, const std::string& data, const std::string& data_format) override;
    virtual void Reset(ResetReason reason) override;
    // /IDataSourceIn methods

    // IDataSourceOut methods
    virtual void Delete(int64_t id) override;
    virtual bool IsReset() const override;
    virtual ResetInformationList AcknowledgeReset() override;

    virtual bool IsOverflown() const override;
    virtual DeletionInformationList AcknowledgeOverflow() override;
    virtual bool GetAllowOverflow() const override;
    virtual boost::shared_mutex& GetBufferSharedMutex() const override;
    virtual BufferQueueType::iterator begin() override;
    virtual BufferQueueType::iterator end() override;

    virtual const ReferenceData& GetReference(const std::string& ref) const override;
    // /IDataSourceOut methods

    // shared methods
    virtual size_t GetSize() const override;
    virtual size_t GetMaxSize() const override;
    virtual int64_t GetLastId() const override;
    virtual int8_t GetCounterMode() const override;
    virtual size_t GetDeletionInformationSize() const override;
    virtual size_t GetResetInformationSize() const override;
    virtual bool GetEnableMemoryInfoLogging() const override;
    // /shared methods

   private:
    void OnDeleteCallback(const BufferEntry* entry, bool clear, uint64_t timestamp_ms);
    void ProcessRefMapping(int64_t id, std::vector<Measurement>& data);
    void DeleteRefMapping(int64_t id, bool clear);

    parsing::JsonParser parser_;
    RingBuffer buffer_;
    mutable boost::shared_mutex ref_mapping_mutex_;
    ReferenceContainer ref_mapping_;
    uint64_t ref_counter_;

    const size_t kResetInformationSize_;
    ResetInformationList reset_information_list_;
    mutable boost::shared_mutex reset_information_list_mutex_;

    const size_t kDeletionInformationSize_;
    DeletionInformationList deletion_information_list_;
    mutable boost::shared_mutex deletion_information_list_mutex_;
    bool enable_memory_info_logging_ = false;
};
}  // namespace core
}  // namespace qds_buffer
