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

    ds.Reset();
    EXPECT_EQ(0, ds.GetSize());
}

TEST(DataSourceInternalTest, Iterator) {
    DataSourceInternal ds;

    ds.Add(111, "{\"NAME\":\"aaa\",\"TYPE\":\"STRING\",\"VALUE\":\"test-string\"}");
    ds.Add(222, "{\"NAME\":\"bbb\",\"TYPE\":\"INT\",\"VALUE\":123}");
    ds.Add(333, "{\"NAME\":\"ccc\",\"TYPE\":\"BOOL\",\"VALUE\":true}");

    std::shared_lock lock(ds.GetBufferSharedMutex());

    auto it = ds.begin();
    EXPECT_EQ(111, it->id_);
    EXPECT_EQ("aaa", it->measurements_->data()->name_);
    EXPECT_EQ("test-string", std::get<std::string>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(222, it->id_);
    EXPECT_EQ("bbb", it->measurements_->data()->name_);
    EXPECT_EQ(123, std::get<std::int64_t>(it->measurements_->data()->value_));

    ++it;
    EXPECT_EQ(333, it->id_);
    EXPECT_EQ("ccc", it->measurements_->data()->name_);
    EXPECT_EQ(true, std::get<bool>(it->measurements_->data()->value_));

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
    ds.Reset();
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

    // explicit delete
    ds.Delete(4);

    EXPECT_NO_THROW(ds.GetReference("ref-333"));
    EXPECT_THROW(ds.GetReference("ref-444"), RefException); // Reference ref-444 not found
    EXPECT_NO_THROW(ds.GetReference("ref-555"));
}
