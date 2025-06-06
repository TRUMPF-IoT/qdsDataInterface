// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <functional>

#include <measurement.hpp>
#include <types.hpp>
#include <boost/thread.hpp>

namespace qds_buffer {
   
   namespace core {

      using OnDeleteCallbackType = std::function<void(const BufferEntry*, bool, uint64_t)>;

      /**
       * Thread-Safe
       */
      class RingBuffer {
      public:
         RingBuffer(size_t size, int8_t counter_mode, bool allow_overflow = true, OnDeleteCallbackType on_delete_callback = nullptr);

         bool Push(int64_t id, std::shared_ptr<std::vector<Measurement>> measurement);
         void Delete(int64_t id);
         ResetInformation Reset(ResetReason reason);

         boost::shared_mutex& GetSharedMutex() const;
         BufferQueueType::iterator begin();
         BufferQueueType::iterator end();

         size_t GetSize() const;
         size_t GetMaxSize() const;
         int64_t GetLastId() const;
         int8_t GetCounterMode() const;
         bool GetAllowOverflow() const;

      private:
         static uint64_t GetCurrentTimeMs();

         const size_t kMaxSize_;
         const int8_t kCounterMode_;
         const bool kAllowOverflow_;

         mutable boost::shared_mutex mutex_;
         BufferQueueType buffer_;

         OnDeleteCallbackType on_delete_callback_;
      };

   } // namespace
}