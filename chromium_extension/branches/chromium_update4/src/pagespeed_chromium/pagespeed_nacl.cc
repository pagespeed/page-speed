// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include "base/at_exit.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed_chromium/pagespeed_chromium.h"
#include "pagespeed/core/pagespeed_init.h"

// Include the methods that we stub out to make the linker happy. We
// do this via an include rather than passing the cc to the compiler
// because in the latter case the compiler infers that the code is
// unused and discards the translation unit.
#include "pagespeed_chromium/pagespeed_nacl_stubs.cc"

namespace {

const char* kUnknownMessageId = "unknown";

std::string MakeErrorJsonString(const std::string& value) {
  std::string error;
  // Let's build the result with value.
  scoped_ptr<DictionaryValue> root(new DictionaryValue);
  root->SetString("error", value);
  base::JSONWriter::Write(root.get(), false, &error);
  return error;
}

bool ExtractIdAndMessage(const std::string& in, std::string* out_id,
                         std::string* out_message) {
  const std::string search_part = in.substr(0, 10);
  size_t comma_pos = search_part.find(',');
  if (comma_pos == std::string::npos) {
    return false;
  }

  *out_id = in.substr(0, comma_pos);
  *out_message = in.substr(comma_pos+1);
  return true;
}

// A new pp::Instance object is instantiated for each <embed> element that
// references the nexe that hosts the module.
class PageSpeedInstance : public pp::Instance {
 public:
  explicit PageSpeedInstance(PP_Instance instance) : pp::Instance(instance) {}
  virtual ~PageSpeedInstance() {}

  virtual void HandleMessage(const pp::Var& var_message);

 private:
  void PostBackMessage(const std::string& id, const std::string& value);
};

// A pp::Module is a factory for creating pp::Instance objects. A
// single pp::Module is instantiated per process.
class PageSpeedModule : public pp::Module {
 public:
  // Only a single PageSpeedModule instance will be instantiated per
  // NaCL process, so we can safely assume that our Init() code in the
  // construtor will only be run once per process.
  PageSpeedModule() : pp::Module() {
    pagespeed::Init();
  }

  virtual ~PageSpeedModule() {
    pagespeed::ShutDown();
  }

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new PageSpeedInstance(instance);
  }
};

void PageSpeedInstance::HandleMessage(const pp::Var& in) {
  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  if (!in.is_string()) {
    // We don't know what is the id of the sending side.
    PostBackMessage(
        kUnknownMessageId,
        MakeErrorJsonString("Failed to process non-string message."));
    return;
  }


  const std::string json_with_id = in.AsString();
  // Extract the ID part, before we parse the JSON format of Page Speed input.
  // The ID part is seperated from the JSON with a comma (','). We search only
  // the first 10 charactor, which is large enough for a valid tadId. That way
  // if the input data is total garbage we don't scan it looking for a very long
  // string.
  std::string message_id, in_json;
  if (!ExtractIdAndMessage(json_with_id, &message_id, &in_json)) {
    PostBackMessage(
        kUnknownMessageId,
        MakeErrorJsonString("Failed to extract the message sender id."));
    return;
  }

  std::string result, error;
  if (!pagespeed_chromium::RunPageSpeedRules(in_json, &result, &error)) {
    // Let's build the result with error message.
    result = MakeErrorJsonString(error);
  }
  PostBackMessage(message_id, result);
}

void PageSpeedInstance::PostBackMessage(const std::string& id,
                                  const std::string& value) {
  // Prepend the id at the beginning of the message.
  std::string message;
  message.append(id);
  message.append(",");
  message.append(value);

  pp::Var out(message);
  PostMessage(out);
}

}  // namespace

#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

namespace pp {

// CreateModule() is the hook that gets exported and that will be
// invoked by the host process (e.g. the Chrome browser) to provide
// our module to the host process runtime. It allows the host process
// to instantiate a pp::Module which in turn allows instantiation of
// the pp::Instance objects that we can postMessage() to from
// JavaScript code. Our code never invokes CreateModule. It is
// provided to export our module to the host process.
Module* CreateModule() {
  return new PageSpeedModule();
}

}  // namespace pp

#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
