// SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
//
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <fstream>

#include <exception.hpp>

#include "data_source_internal.hpp"

using namespace qds_buffer::core;

#define DUMMY_JSON "{\"NAME\":\"a\",\"TYPE\":\"STRING\",\"VALUE\":\"\"}"

TEST(DataSourceInternalTest, Add) {
    DataSourceInternal ds;

    EXPECT_NO_THROW(ds.Add(0, DUMMY_JSON));
    EXPECT_THROW(ds.Add(0, DUMMY_JSON), RingBufferException); // Bad Id
    EXPECT_THROW(ds.Add(0, "{\"NAME\":a\",\"TYPE\":\"STRING\",\"VALUE\":\"\"}"), ParsingException); // Parsing error: syntax error
    EXPECT_THROW(ds.Add(1, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-123\"}"), // The reference of 'a' is neither an existing
                 RefException);                                                        // file, nor an existing reference

    ds.SetReference("ref-123", "testdata", "abc");

    EXPECT_NO_THROW(ds.Add(1, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-123\"}"));
    EXPECT_THROW(ds.Add(2, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-123\"}"), RefException); // The reference 'ref-123'
                                                                                                      // is already in use

    std::ofstream file;
    file.open("DataSourceInternalTest.data");
    file << "testdata";
    file.close();

    ASSERT_FALSE((std::ifstream("DataSourceInternalTest.data")).fail());
    EXPECT_NO_THROW(ds.Add(2, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"DataSourceInternalTest.data\"}"));
    ASSERT_TRUE((std::ifstream("DataSourceInternalTest.data")).fail());
    auto it = ds.begin()+2;
    EXPECT_NE("DataSourceInternalTest.data", it->measurements_->data()->ValueToString());
}

TEST(DataSourceInternalTest, Delete) {
    DataSourceInternal ds;

    EXPECT_NO_THROW(ds.Delete(123));
    ds.Add(123, DUMMY_JSON);
    EXPECT_NO_THROW(ds.Delete(123));
}

TEST(DataSourceInternalTest, Reset) {
    DataSourceInternal ds;

    ds.Add(111, DUMMY_JSON); ds.Add(222, DUMMY_JSON); ds.Add(333, DUMMY_JSON);
    EXPECT_EQ(3, ds.GetSize());

    ds.Reset(ResetReason::UNKNOWN);
    EXPECT_EQ(0, ds.GetSize());
}

TEST(DataSourceInternalTest, IsReset) {
    DataSourceInternal ds;

    EXPECT_FALSE(ds.IsReset());

    ds.Add(111, DUMMY_JSON); ds.Add(222, DUMMY_JSON); ds.Add(333, DUMMY_JSON);

    EXPECT_FALSE(ds.IsReset());

    ds.Reset(ResetReason::UNKNOWN);

    EXPECT_TRUE(ds.IsReset());
}

TEST(DataSourceInternalTest, AcknowledgeReset) {
    DataSourceInternal ds;

    ds.Add(0, DUMMY_JSON);

    auto info = ds.AcknowledgeReset();
    EXPECT_TRUE(info.list_.empty());
    EXPECT_FALSE(info.exceeded_max_entries_);

    ds.Reset(ResetReason::SYSTEM);

    info = ds.AcknowledgeReset();
    EXPECT_EQ(1, info.list_.size());
    EXPECT_FALSE(info.exceeded_max_entries_);

    for (int i = 1; i < 102; i++) {
        ds.Add(i, DUMMY_JSON);
        ds.Reset(ResetReason::SYSTEM);
    }

    info = ds.AcknowledgeReset();
    EXPECT_EQ(100, info.list_.size());
    EXPECT_TRUE(info.exceeded_max_entries_);
    EXPECT_EQ(ResetReason::SYSTEM, info.list_[0].reset_reason);
}

TEST(DataSourceInternalTest, IsOverflown) {
    DataSourceInternal ds(2);

    EXPECT_FALSE(ds.IsOverflown());

    ds.Add(111, DUMMY_JSON); ds.Add(222, DUMMY_JSON);
    EXPECT_FALSE(ds.IsOverflown());

    ds.Add(333, DUMMY_JSON);
    EXPECT_TRUE(ds.IsOverflown());
}

TEST(DataSourceInternalTest, AcknowledgeOverflow) {
    DataSourceInternal ds;

    ds.Add(0, DUMMY_JSON);

    auto info = ds.AcknowledgeOverflow();
    EXPECT_TRUE(info.list_.empty());
    EXPECT_FALSE(info.exceeded_max_entries_);

    for (int i = 1; i < 100; i++) {
        ds.Add(i, DUMMY_JSON);
    }

    info = ds.AcknowledgeOverflow();
    EXPECT_TRUE(info.list_.empty());
    EXPECT_FALSE(info.exceeded_max_entries_);

    ds.Add(101, DUMMY_JSON);

    info = ds.AcknowledgeOverflow();
    EXPECT_EQ(1, info.list_.size());
    EXPECT_FALSE(info.exceeded_max_entries_);

    for (int i = 102; i < 203; i++) {
        ds.Add(i, DUMMY_JSON);
    }

    info = ds.AcknowledgeOverflow();
    EXPECT_EQ(100, info.list_.size());
    EXPECT_TRUE(info.exceeded_max_entries_);
}

TEST(DataSourceInternalTest, Iterator) {
    DataSourceInternal ds;

    ds.Add(111, "{\"NAME\":\"aaa\",\"TYPE\":\"STRING\",\"VALUE\":\"test-string\"}");
    ds.Add(222, "{\"NAME\":\"bbb\",\"TYPE\":\"INT\",\"VALUE\":123}");
    ds.Add(333, "{\"NAME\":\"ccc\",\"TYPE\":\"BOOL\",\"VALUE\":true}");

    boost::shared_lock<boost::shared_mutex> lock(ds.GetBufferSharedMutex());

    auto it = ds.begin();
    EXPECT_EQ(111, it->id_);
    EXPECT_EQ("aaa", it->measurements_->data()->name_);
    EXPECT_EQ("test-string", boost::get<std::string>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(222, it->id_);
    EXPECT_EQ("bbb", it->measurements_->data()->name_);
    EXPECT_EQ(123, boost::get<std::int64_t>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(333, it->id_);
    EXPECT_EQ("ccc", it->measurements_->data()->name_);
    EXPECT_EQ(true, boost::get<bool>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(it, ds.end());
}

TEST(DataSourceInternalTest, GetSize) {
    DataSourceInternal ds;

    EXPECT_EQ(0, ds.GetSize());
    ds.Add(111, DUMMY_JSON);
    EXPECT_EQ(1, ds.GetSize());
    ds.Add(222, DUMMY_JSON);
    EXPECT_EQ(2, ds.GetSize());
    ds.Add(333, DUMMY_JSON);
    ds.Add(444, DUMMY_JSON);
    EXPECT_EQ(4, ds.GetSize());
    ds.Reset(ResetReason::UNKNOWN);
    EXPECT_EQ(0, ds.GetSize());
}

TEST(DataSourceInternalTest, GetMaxSize) {
    {
        DataSourceInternal ds;
        EXPECT_EQ(100, ds.GetMaxSize());
        ds.Add(111, DUMMY_JSON); ds.Add(222, DUMMY_JSON); ds.Add(333, DUMMY_JSON);
        EXPECT_EQ(100, ds.GetMaxSize());
    }
    {
        DataSourceInternal ds{345, 0};
        EXPECT_EQ(345, ds.GetMaxSize());
    }
}

TEST(DataSourceInternalTest, GetLastId) {
    DataSourceInternal ds;

    EXPECT_EQ(-1, ds.GetLastId());
    ds.Add(111, DUMMY_JSON); ds.Add(222, DUMMY_JSON); ds.Add(333, DUMMY_JSON); ds.Add(444, DUMMY_JSON);
    EXPECT_EQ(444, ds.GetLastId());
    ds.Delete(333);
    EXPECT_EQ(444, ds.GetLastId());
    ds.Delete(444);
    EXPECT_EQ(222, ds.GetLastId());
}

TEST(DataSourceInternalTest, SetReference) {
    DataSourceInternal ds;

    EXPECT_NO_THROW(ds.SetReference("ref-123", "testdata", "abc"));
    EXPECT_THROW(ds.SetReference("ref-123", "testdata", "abc"), RefException); // Reference ref-123 exists already
}

TEST(DataSourceInternalTest, GetReference) {
    DataSourceInternal ds;

    EXPECT_THROW(ds.GetReference("ref-123"), RefException); // Reference ref-123 not found
    ds.SetReference("ref-123", "testdata", "abc");
    EXPECT_NO_THROW(ds.GetReference("ref-123"));

    auto ref = ds.GetReference("ref-123");
    EXPECT_EQ(0, ref.id_);

    // file content
    std::ofstream file;
    file.open("DataSourceInternalTest.data");
    file << "testdata";
    file.close();

    ASSERT_FALSE((std::ifstream("DataSourceInternalTest.data")).fail());
    EXPECT_NO_THROW(ds.Add(123, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"DataSourceInternalTest.data\"}"));
    ASSERT_TRUE((std::ifstream("DataSourceInternalTest.data")).fail());

    EXPECT_NO_THROW(ds.GetReference("ref-0"));
    ref = ds.GetReference("ref-0");
    EXPECT_EQ(123, ref.id_);
    EXPECT_EQ("data", ref.format_);
    EXPECT_EQ("testdata", ref.content_);
}

TEST(DataSourceInternalTest, DeleteRefMapping) {
    DataSourceInternal ds{3};

    ds.SetReference("ref-111", "testdata", "abc");
    ds.SetReference("ref-222", "testdata", "abc");
    ds.SetReference("ref-333", "testdata", "abc");
    ds.SetReference("ref-444", "testdata", "abc");
    ds.SetReference("ref-555", "testdata", "abc");

    EXPECT_NO_THROW(ds.GetReference("ref-111"));
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444"));
    EXPECT_NO_THROW(ds.GetReference("ref-555"));

    ds.Add(1, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-111\"}");
    ds.Add(2, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-222\"}");
    ds.Add(3, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-333\"}");

    EXPECT_NO_THROW(ds.GetReference("ref-111"));
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444"));
    EXPECT_NO_THROW(ds.GetReference("ref-555"));

    // ring buffer overflow
    ds.Add(4, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-444\"}");
    ds.Add(5, "{\"NAME\":\"a\",\"TYPE\":\"REF\",\"VALUE\":\"ref-555\"}");

    EXPECT_THROW(ds.GetReference("ref-111"), RefException); // Reference ref-111 not found
    EXPECT_THROW(ds.GetReference("ref-222"), RefException); // Reference ref-222 not found
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444"));
    EXPECT_NO_THROW(ds.GetReference("ref-555"));
}

// NEW COMPREHENSIVE OVERFLOW REFERENCE DELETION TESTS

TEST(DataSourceInternalTest, OverflowRefDeletionBasic) {
    // Test basic overflow scenario with references
    DataSourceInternal ds{3}; // Small buffer to trigger overflow quickly

    // Set up references
    ds.SetReference("ref-111", "data1", "txt");
    ds.SetReference("ref-222", "data2", "txt");
    ds.SetReference("ref-333", "data3", "txt");
    ds.SetReference("ref-444", "data4", "txt");

    // Add entries with references
    ds.Add(1, "{\"NAME\":\"test1\",\"TYPE\":\"REF\",\"VALUE\":\"ref-111\"}");
    ds.Add(2, "{\"NAME\":\"test2\",\"TYPE\":\"REF\",\"VALUE\":\"ref-222\"}");
    ds.Add(3, "{\"NAME\":\"test3\",\"TYPE\":\"REF\",\"VALUE\":\"ref-333\"}");

    // Verify all references are accessible
    EXPECT_NO_THROW(ds.GetReference("ref-111"));
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444")); // unused reference

    // Trigger overflow - this should delete entry 1 and its reference
    ds.Add(4, "{\"NAME\":\"test4\",\"TYPE\":\"REF\",\"VALUE\":\"ref-444\"}");

    // Entry 1 should be gone, so ref-111 should be deleted
    EXPECT_THROW(ds.GetReference("ref-111"), RefException);
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444"));
}

TEST(DataSourceInternalTest, OverflowRefDeletionMultipleOverflows) {
    // Test multiple sequential overflows
    DataSourceInternal ds{2}; // Very small buffer

    for (int i = 1; i <= 10; i++) {
        std::string ref_name = "ref-" + std::to_string(i);
        ds.SetReference(ref_name, "data" + std::to_string(i), "txt");
        ds.Add(i, "{\"NAME\":\"test" + std::to_string(i) + "\",\"TYPE\":\"REF\",\"VALUE\":\"" + ref_name + "\"}");
    }

    // Only the last 2 entries should remain (ids 9 and 10)
    EXPECT_EQ(2, ds.GetSize());
    
    // Only references for the last 2 entries should exist
    for (int i = 1; i <= 8; i++) {
        std::string ref_name = "ref-" + std::to_string(i);
        EXPECT_THROW(ds.GetReference(ref_name), RefException) << "Reference " << ref_name << " should be deleted";
    }
    
    // Last 2 references should still exist
    EXPECT_NO_THROW(ds.GetReference("ref-9"));
    EXPECT_NO_THROW(ds.GetReference("ref-10"));
}

TEST(DataSourceInternalTest, OverflowRefDeletionMixedEntryTypes) {
    // Test overflow with mixed entry types (some with refs, some without)
    DataSourceInternal ds{3};

    ds.SetReference("ref-111", "data1", "txt");
    ds.SetReference("ref-333", "data3", "txt");

    // Mix of reference and non-reference entries
    ds.Add(1, "{\"NAME\":\"test1\",\"TYPE\":\"REF\",\"VALUE\":\"ref-111\"}");
    ds.Add(2, "{\"NAME\":\"test2\",\"TYPE\":\"STRING\",\"VALUE\":\"regular_data\"}");
    ds.Add(3, "{\"NAME\":\"test3\",\"TYPE\":\"REF\",\"VALUE\":\"ref-333\"}");

    EXPECT_NO_THROW(ds.GetReference("ref-111"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));

    // Add non-reference entry to trigger overflow
    ds.Add(4, "{\"NAME\":\"test4\",\"TYPE\":\"INT\",\"VALUE\":42}");

    // Entry 1 (with ref-111) should be deleted
    EXPECT_THROW(ds.GetReference("ref-111"), RefException);
    EXPECT_NO_THROW(ds.GetReference("ref-333"));

    // Add another reference entry
    ds.SetReference("ref-555", "data5", "txt");
    ds.Add(5, "{\"NAME\":\"test5\",\"TYPE\":\"REF\",\"VALUE\":\"ref-555\"}");

    // Entry 2 (non-reference) should be deleted, ref-333 should remain
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-555"));
}

TEST(DataSourceInternalTest, OverflowRefDeletionFileReferences) {
    // Test overflow with file-based references
    DataSourceInternal ds{2};

    // Create test files
    std::ofstream file1("test_file_1.dat");
    file1 << "file_content_1";
    file1.close();

    std::ofstream file2("test_file_2.dat");
    file2 << "file_content_2";
    file2.close();

    std::ofstream file3("test_file_3.dat");
    file3 << "file_content_3";
    file3.close();

    ASSERT_FALSE((std::ifstream("test_file_1.dat")).fail());
    ASSERT_FALSE((std::ifstream("test_file_2.dat")).fail());
    ASSERT_FALSE((std::ifstream("test_file_3.dat")).fail());

    // Add entries with file references
    ds.Add(1, "{\"NAME\":\"test1\",\"TYPE\":\"REF\",\"VALUE\":\"test_file_1.dat\"}");
    ds.Add(2, "{\"NAME\":\"test2\",\"TYPE\":\"REF\",\"VALUE\":\"test_file_2.dat\"}");

    // Files should be consumed and auto-generated references created
    ASSERT_TRUE((std::ifstream("test_file_1.dat")).fail());
    ASSERT_TRUE((std::ifstream("test_file_2.dat")).fail());
    EXPECT_NO_THROW(ds.GetReference("ref-0"));
    EXPECT_NO_THROW(ds.GetReference("ref-1"));

    // Trigger overflow
    ds.Add(3, "{\"NAME\":\"test3\",\"TYPE\":\"REF\",\"VALUE\":\"test_file_3.dat\"}");
    
    // First auto-generated reference should be deleted
    EXPECT_THROW(ds.GetReference("ref-0"), RefException);
    EXPECT_NO_THROW(ds.GetReference("ref-1"));
    EXPECT_NO_THROW(ds.GetReference("ref-2"));

    // Files should be consumed
    ASSERT_TRUE((std::ifstream("test_file_3.dat")).fail());
}

TEST(DataSourceInternalTest, OverflowRefDeletionEdgeCaseEmptyBuffer) {
    // Test edge case: overflow on empty buffer (shouldn't happen but let's be safe)
    DataSourceInternal ds{1};

    ds.SetReference("ref-111", "data1", "txt");
    
    // Add one entry
    ds.Add(1, "{\"NAME\":\"test1\",\"TYPE\":\"REF\",\"VALUE\":\"ref-111\"}");
    EXPECT_NO_THROW(ds.GetReference("ref-111"));

    // Reset buffer
    ds.Reset(ResetReason::UNKNOWN);
    
    // Reference should be cleared after reset
    EXPECT_THROW(ds.GetReference("ref-111"), RefException);
}

TEST(DataSourceInternalTest, OverflowRefDeletionSameRefMultipleEntries) {
    // Test edge case: multiple entries referencing the same reference during overflow
    DataSourceInternal ds{3};

    ds.SetReference("shared-ref", "shared_data", "txt");

    // All entries use the same reference - this should fail after first use
    ds.Add(1, "{\"NAME\":\"test1\",\"TYPE\":\"REF\",\"VALUE\":\"shared-ref\"}");
    
    // These should fail since reference is already in use
    EXPECT_THROW(ds.Add(2, "{\"NAME\":\"test2\",\"TYPE\":\"REF\",\"VALUE\":\"shared-ref\"}"), RefException);
    EXPECT_THROW(ds.Add(3, "{\"NAME\":\"test3\",\"TYPE\":\"REF\",\"VALUE\":\"shared-ref\"}"), RefException);

    EXPECT_NO_THROW(ds.GetReference("shared-ref"));
    EXPECT_EQ(1, ds.GetSize()); // Only one entry should exist
}

TEST(DataSourceInternalTest, OverflowRefDeletionStressTest) {
    // Stress test: many references with rapid overflow
    DataSourceInternal ds{5}; // Small buffer for rapid overflow

    const int num_iterations = 50;
    
    for (int i = 1; i <= num_iterations; i++) {
        std::string ref_name = "stress-ref-" + std::to_string(i);
        ds.SetReference(ref_name, "stress_data_" + std::to_string(i), "txt");
        ds.Add(i, "{\"NAME\":\"stress" + std::to_string(i) + "\",\"TYPE\":\"REF\",\"VALUE\":\"" + ref_name + "\"}");
        
        // Check that buffer doesn't exceed max size
        EXPECT_LE(ds.GetSize(), ds.GetMaxSize());
        
        // For iterations beyond buffer size, check that old references are cleaned up
        if (i > static_cast<int>(ds.GetMaxSize())) {
            int old_ref_index = i - static_cast<int>(ds.GetMaxSize()) - 1;
            if (old_ref_index > 0) {
                std::string old_ref_name = "stress-ref-" + std::to_string(old_ref_index);
                EXPECT_THROW(ds.GetReference(old_ref_name), RefException) 
                    << "Old reference " << old_ref_name << " should be deleted at iteration " << i;
            }
        }
    }
    
    // Final verification: only the last 5 references should exist
    for (int i = 1; i <= num_iterations - static_cast<int>(ds.GetMaxSize()); i++) {
        std::string ref_name = "stress-ref-" + std::to_string(i);
        EXPECT_THROW(ds.GetReference(ref_name), RefException) 
            << "Reference " << ref_name << " should be deleted in final check";
    }
    
    for (int i = num_iterations - static_cast<int>(ds.GetMaxSize()) + 1; i <= num_iterations; i++) {
        std::string ref_name = "stress-ref-" + std::to_string(i);
        EXPECT_NO_THROW(ds.GetReference(ref_name)) 
            << "Reference " << ref_name << " should still exist in final check";
    }
}

TEST(DataSourceInternalTest, OverflowRefDeletionCounterMode1) {
    // Test reference deletion in counter mode 1 (UNIQUE)
    DataSourceInternal ds{3, 1}; // Counter mode 1, buffer size 3

    ds.SetReference("ref-111", "data1", "txt");
    ds.SetReference("ref-222", "data2", "txt");
    ds.SetReference("ref-333", "data3", "txt");

    // Add entries in non-sequential order (allowed in mode 1)
    ds.Add(10, "{\"NAME\":\"test10\",\"TYPE\":\"REF\",\"VALUE\":\"ref-111\"}");
    ds.Add(5, "{\"NAME\":\"test5\",\"TYPE\":\"REF\",\"VALUE\":\"ref-222\"}");
    ds.Add(15, "{\"NAME\":\"test15\",\"TYPE\":\"REF\",\"VALUE\":\"ref-333\"}");

    EXPECT_NO_THROW(ds.GetReference("ref-111"));
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));

    // Add another entry to trigger overflow
    ds.SetReference("ref-444", "data4", "txt");
    ds.Add(20, "{\"NAME\":\"test20\",\"TYPE\":\"REF\",\"VALUE\":\"ref-444\"}");

    // One of the original references should be deleted (first one chronologically)
    EXPECT_THROW(ds.GetReference("ref-111"), RefException);
    EXPECT_NO_THROW(ds.GetReference("ref-222"));
    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_NO_THROW(ds.GetReference("ref-444"));
}

TEST(DataSourceInternalTest, ValueStringEscapeCharacterDoubleQuote) {
    DataSourceInternal ds;

    ds.Add(111, "{\"NAME\":\"aaa\",\"TYPE\":\"STRING\",\"VALUE\":\"\\\"AIF\\\"AIF.mpf\"}");
    boost::shared_lock<boost::shared_mutex> lock(ds.GetBufferSharedMutex());

    auto it = ds.begin();
    EXPECT_EQ("\"AIF\"AIF.mpf", boost::get<std::string>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(it, ds.end());
}

TEST(DataSourceInternalTest, ValueStringEscapeCharacterBackslash) {
    DataSourceInternal ds;

    ds.Add(111, "{\"NAME\":\"aaa\",\"TYPE\":\"STRING\",\"VALUE\":\"\\\\AIF\\\\AIF.mpf\"}");
    boost::shared_lock<boost::shared_mutex> lock(ds.GetBufferSharedMutex());

    auto it = ds.begin();
    EXPECT_EQ("\\AIF\\AIF.mpf", boost::get<std::string>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(it, ds.end());
}

TEST(DataSourceInternalTest, ValueStringEscapeMultipleCharacter) {
    DataSourceInternal ds;

    ds.Add(111, "{\"NAME\":\"aaa\",\"TYPE\":\"STRING\",\"VALUE\":\"\\t\\n\\r\\\\AIF\\t\\n\\r\\\\AIF.mpf\"}");
    boost::shared_lock<boost::shared_mutex> lock(ds.GetBufferSharedMutex());

    auto it = ds.begin();
    EXPECT_EQ("\t\n\r\\AIF\t\n\r\\AIF.mpf", boost::get<std::string>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(it, ds.end());
}