/**
 * Copyright 2008-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade
//
// Validate CallGraphMetadata methods.

#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "call_graph_metadata.h"
#include "profile.pb.h"

#include "testing/gtest/include/gtest/gtest.h"

class CallGraphMetadataTest : public testing::Test {
 protected:
  virtual void SetUp() {
    profile_.reset(new activity::Profile);
    metadata_.reset(new activity::CallGraphMetadata(profile_.get()));
  }

  virtual void TearDown() {
    metadata_.reset();
    profile_.reset();
  }

  // Get the internal metadata map for the CallGraphMetadata object.
  const activity::CallGraphMetadata::MetadataMap &metadata_map() {
    return *metadata_->map();
  }

  const activity::FunctionMetadata *GetMetadataEntry(int32 tag) {
    activity::CallGraphMetadata::MetadataMap::const_iterator it =
        metadata_map().find(tag);
    if (metadata_map().end() == it) {
      return NULL;
    }
    return it->second;
  }

  scoped_ptr<activity::Profile> profile_;
  scoped_ptr<activity::CallGraphMetadata> metadata_;
};

namespace {

const char *kFileName = "foo.js";
const char *kFunctionSource = "function() {}";
const char *kFunctionNameFormat = "foo%d";
const int32 kEntryCount = 100;

TEST_F(CallGraphMetadataTest, Basic) {
  // Verify that the entries do not exist.
  for (int32 i = 0; i <= kEntryCount; i++) {
    ASSERT_FALSE(metadata_->HasEntry(i));
  }
  ASSERT_TRUE(metadata_map().empty());

  // Add all entries.
  for (int32 i = 0; i <= kEntryCount; i++) {
    // make up a fake init time
    const int64 init_time_usec = i / 2;
    std::string function_name = StringPrintf(kFunctionNameFormat, i);

    metadata_->AddEntry(
        i,
        kFileName,
        function_name.c_str(),
        kFunctionSource,
        init_time_usec);

    // Make sure the sizes of the maps grow as we expect them to.
    ASSERT_EQ(i + 1, metadata_map().size());
  }

  // Verify that the entries exist.
  for (int32 i = 0; i <= kEntryCount; i++) {
    ASSERT_TRUE(metadata_->HasEntry(i));
    const activity::FunctionMetadata *entry = GetMetadataEntry(i);
    ASSERT_NE(static_cast<const activity::FunctionMetadata*>(NULL), entry);
    ASSERT_EQ(i, entry->function_tag());
    ASSERT_STREQ(kFileName, entry->file_name().c_str());
    ASSERT_STREQ(kFunctionSource, entry->function_source_utf8().c_str());
    std::string function_name = StringPrintf(kFunctionNameFormat, i);
    ASSERT_EQ(function_name, entry->function_name());
  }
}

}  // namespace
