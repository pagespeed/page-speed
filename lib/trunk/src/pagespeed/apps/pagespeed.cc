// Copyright 2009 Google Inc. All Rights Reserved.
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

// Command line utility that runs lint rules on the provided input set.

#include <stdio.h>

#include <fstream>
#include <iostream>  // for std::cin and std::cout

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/stubs/common.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/input_capabilities.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_input_util.h"
#include "pagespeed/core/pagespeed_version.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/dom/json_dom.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/image_compression/image_attributes_factory.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/l10n/gettext_localizer.h"
#include "pagespeed/l10n/register_locale.h"
#include "pagespeed/pdf/generate_pdf_report.h"
#include "pagespeed/proto/formatted_results_to_json_converter.h"
#include "pagespeed/proto/formatted_results_to_text_converter.h"
#include "pagespeed/proto/pagespeed_input.pb.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/proto/proto_resource_utils.h"
#include "pagespeed/proto/results_to_json_converter.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/rule_provider.h"
#include "pagespeed/timeline/json_importer.h"
#include "third_party/gflags/src/google/gflags.h"

DEFINE_string(input_format, "har",
              "Format of input_file. One of 'har' or 'proto'.");
DEFINE_string(output_format, "text",
              "Format of the output. "
              "One of 'proto', 'text', 'unformatted_json', "
              "'formatted_json', 'formatted_proto', or 'pdf'.");
DEFINE_string(input_file, "", "Path to the input file. '-' to read from stdin");
DEFINE_string(output_file, "-",
              "Path to the output file. '-' to write to stdout (the default)");
DEFINE_string(instrumentation_input_file, "",
              "Path to the instrumentation data JSON file. Optional.");
DEFINE_string(dom_input_file, "", "Path to the DOM JSON file. Optional.");
DEFINE_string(locale, "", "Locale to use, if localizing results.");
DEFINE_string(strategy, "desktop",
              "The strategy to use. Valid values are 'desktop', 'mobile'.");
DEFINE_bool(show_locales, false, "List all available locales and exit.");
DEFINE_bool(v, false, "Show the Page Speed version and exit.");
DEFINE_string(log_file, "",
              "Path to log file. "
              "Logs will be printed only to console if not specified.");
DEFINE_bool(also_log_to_stderr, false,
            "Output logs to error console along with the log file. ");

// gflags defines its own version flag, which doesn't actually provide
// any way to show the version of the program. We disable processing
// of that flag and handle it ourselves.
DECLARE_bool(version);

namespace {

enum OutputFormat {
  PROTO_OUTPUT,
  TEXT_OUTPUT,
  JSON_OUTPUT,
  FORMATTED_JSON_OUTPUT,
  FORMATTED_PROTO_OUTPUT,
  PDF_OUTPUT
};

enum Strategy {
  DESKTOP,
  MOBILE,
};

// UTF-8 byte order mark.
const char* kUtf8Bom = "\xEF\xBB\xBF";
const size_t kUtf8BomSize = strlen(kUtf8Bom);

bool ReadFileToString(const std::string &file_name, std::string *dest) {
  std::ifstream file_stream;
  file_stream.open(
      file_name.c_str(), std::ifstream::in | std::ifstream::binary);
  if (file_stream.fail()) {
    return false;
  }
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
  return true;
}

pagespeed::PagespeedInput* ParseProtoInput(const std::string& file_contents) {
  pagespeed::ProtoInput input_proto;
  ::google::protobuf::io::ArrayInputStream input_stream(
      file_contents.data(), file_contents.size());
  bool success = input_proto.ParseFromZeroCopyStream(&input_stream);
  CHECK(success);

  pagespeed::PagespeedInput *input = new pagespeed::PagespeedInput;
  pagespeed::proto::PopulatePagespeedInput(input_proto, input);
  if (!input_proto.identifier().empty()) {
    input->SetPrimaryResourceUrl(input_proto.identifier());
  }
  return input;
}

void PrintUsage() {
  ::google::ShowUsageWithFlagsRestrict(::google::GetArgv0(), __FILE__);
}

void PrintLocales() {
  fprintf(stderr, "Available locales:");
  std::vector<std::string> locales;
  pagespeed::l10n::RegisterLocale::GetAllLocales(&locales);
  for (size_t i = 0; i < locales.size(); i++) {
    fprintf(stderr, " %s", locales[i].c_str());
  }
  fprintf(stderr, "\n");
}

void PrintVersion() {
  pagespeed::Version version;
  pagespeed::GetPageSpeedVersion(&version);
  fprintf(stderr, "Page Speed v%d.%d. %s\n",
          version.major(), version.minor(),
          (version.official_release() ? "" : "(unofficial release)"));
#ifndef NDEBUG
  fprintf(stderr, "Debug build.\n");
#endif
}

bool RunPagespeed(const std::string& out_format,
                  const std::string& in_format,
                  const std::string& in_filename,
                  const std::string& dom_filename,
                  const std::string& instrumentation_filename,
                  const std::string& out_filename) {
  OutputFormat output_format;
  if (out_format == "proto") {
    output_format = PROTO_OUTPUT;
  } else if (out_format == "text") {
    output_format = TEXT_OUTPUT;
  } else if (out_format == "unformatted_json") {
    output_format = JSON_OUTPUT;
  } else if (out_format == "formatted_json") {
    output_format = FORMATTED_JSON_OUTPUT;
  } else if (out_format == "json") {
    LOG(WARNING) << "'--output_format json' is deprecated. "
                 << "Please use '--output_format formatted_json' instead.";
    output_format = FORMATTED_JSON_OUTPUT;
  } else if (out_format == "formatted_proto") {
    output_format = FORMATTED_PROTO_OUTPUT;
  } else if (out_format == "pdf") {
    output_format = PDF_OUTPUT;
  } else {
    fprintf(stderr, "Invalid output format %s.\n", out_format.c_str());
    PrintUsage();
    return false;
  }

  Strategy strategy;
  if (FLAGS_strategy == "desktop") {
    strategy = DESKTOP;
  } else if (FLAGS_strategy == "mobile") {
    strategy = MOBILE;
  } else {
    fprintf(stderr, "Invalid strategy %s.\n", FLAGS_strategy.c_str());
    PrintUsage();
    return false;
  }

  std::string file_contents;
  if (in_filename == "-") {
    // Special case: if user specifies input file as '-', read the
    // input from stdin.
    file_contents.assign(std::istreambuf_iterator<char>(std::cin),
                         std::istreambuf_iterator<char>());
  } else if (!ReadFileToString(in_filename, &file_contents)) {
    fprintf(stderr, "Could not read input from %s.\n", in_filename.c_str());
    PrintUsage();
    return false;
  }

  scoped_ptr<pagespeed::l10n::Localizer> localizer;
  if (!FLAGS_locale.empty()) {
    localizer.reset(pagespeed::l10n::GettextLocalizer::Create(FLAGS_locale));
    if (!localizer.get()) {
      fprintf(stderr, "Invalid locale %s.\n", FLAGS_locale.c_str());
      PrintLocales();
      PrintUsage();
      return false;
    }
  } else {
    localizer.reset(new pagespeed::l10n::BasicLocalizer());
  }

  // TODO(lsong): Add support for byte order mark.
  // For now, strip byte order mark of the content if exists.
  if (file_contents.compare(0, kUtf8BomSize, kUtf8Bom) == 0) {
    file_contents.erase(0, kUtf8BomSize);
    LOG(INFO) << "Byte order mark ignored.";
  }

  scoped_ptr<pagespeed::PagespeedInput> input;
  if (in_format == "har") {
    input.reset(pagespeed::ParseHttpArchive(file_contents));
  } else if (in_format == "proto") {
    input.reset(ParseProtoInput(file_contents));
  } else {
    fprintf(stderr, "Invalid input format %s.\n", in_format.c_str());
    PrintUsage();
    return false;
  }
  if (input == NULL) {
    fprintf(stderr, "Failed to parse input.\n");
    return false;
  }
  if (input->primary_resource_url().empty() && input->num_resources() > 0) {
    // If no primary resource URL was specified, assume the first
    // resource is the primary resource.
    input->SetPrimaryResourceUrl(input->GetResource(0).GetRequestUrl());
  }

  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());

  std::vector<const pagespeed::InstrumentationData*> instrumentation_data;
  {
    if (!instrumentation_filename.empty()) {
      std::string instrumentation_file_contents;
      if (!ReadFileToString(instrumentation_filename,
                            &instrumentation_file_contents)) {
        fprintf(stderr, "Could not read input from %s.\n",
                instrumentation_filename.c_str());
        PrintUsage();
        return false;
      }

      if (!pagespeed::timeline::CreateTimelineProtoFromJsonString(
              instrumentation_file_contents, &instrumentation_data)) {
        fprintf(stderr, "Failed to parse instrumentation data from %s.\n",
                instrumentation_filename.c_str());
        PrintUsage();
        return false;
      }
    }
  }

  if (!instrumentation_data.empty()) {
    input->AcquireInstrumentationData(&instrumentation_data);
  }

  scoped_ptr<pagespeed::DomDocument> document;
  {
    if (!dom_filename.empty()) {
      std::string dom_file_contents;
      if (!ReadFileToString(dom_filename, &dom_file_contents)) {
        fprintf(stderr, "Could not read input from %s.\n",
                dom_filename.c_str());
        PrintUsage();
        return false;
      }

      std::string error_msg_out;
      scoped_ptr<const Value> document_json(
          base::JSONReader::ReadAndReturnError(
              dom_file_contents,
              true,  // allow_trailing_comma
              NULL,  // error_code_out (ReadAndReturnError permits NULL here)
              &error_msg_out));
      if (document_json == NULL) {
        fprintf(stderr, "Could not parse DOM: %s.\n", error_msg_out.c_str());
        PrintUsage();
        return false;
      }
      if (document_json->IsType(Value::TYPE_DICTIONARY)) {
        document.reset(pagespeed::dom::CreateDocument(
            static_cast<const DictionaryValue*>(document_json.release())));
      }
      if (document == NULL) {
        fprintf(stderr, "Failed to parse DOM from %s.\n",
                dom_filename.c_str());
        PrintUsage();
        return false;
      }
    }
  }

  if (document != NULL) {
    input->AcquireDomDocument(document.release());
  }

  if (strategy == MOBILE) {
    pagespeed::ClientCharacteristics cc;
    pagespeed::pagespeed_input_util::PopulateMobileClientCharacteristics(&cc);
    input->SetClientCharacteristics(cc);
  }

  input->Freeze();

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  const bool save_optimized_content = true;
  pagespeed::rule_provider::AppendPageSpeedRules(save_optimized_content,
                                                 &rules);
  if (strategy == MOBILE) {
    pagespeed::rule_provider::AppendRuleSet(
        save_optimized_content,
        pagespeed::rule_provider::MOBILE_BROWSER_RULES,
        &rules);
  }
  pagespeed::InputCapabilities capabilities = input->EstimateCapabilities();
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::RemoveIncompatibleRules(
      &rules, &incompatible_rule_names, capabilities);
  if (!incompatible_rule_names.empty()) {
    std::string incompatible_rule_list =
        pagespeed::string_util::JoinString(incompatible_rule_names, ' ');
    LOG(INFO) << "Removing incompatible rules: " << incompatible_rule_list
              << "; Capabilities: " << capabilities.DebugString();
  }

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  pagespeed::Results results;
  engine.ComputeResults(*input, &results);

  // If the output format is "proto", print the raw results proto; otherwise,
  // use an appropriate converter.
  std::string out;
  if (output_format == PROTO_OUTPUT) {
    ::google::protobuf::io::StringOutputStream out_stream(&out);
    results.SerializeToZeroCopyStream(&out_stream);
  } else if (output_format == JSON_OUTPUT) {
      pagespeed::proto::ResultsToJsonConverter::Convert(
          results, &out);
  } else {
    // Format the results.
    pagespeed::FormattedResults formatted_results;
    formatted_results.set_locale(localizer->GetLocale());
    pagespeed::formatters::ProtoFormatter formatter(localizer.get(),
                                                    &formatted_results);
    engine.FormatResults(results, &formatter);

    // Convert the FormattedResults into text/json.
    if (output_format == TEXT_OUTPUT) {
      pagespeed::proto::FormattedResultsToTextConverter::Convert(
          formatted_results, &out);
    } else if (output_format == FORMATTED_JSON_OUTPUT) {
      pagespeed::proto::FormattedResultsToJsonConverter::Convert(
          formatted_results, &out);
    } else if (output_format == FORMATTED_PROTO_OUTPUT) {
      ::google::protobuf::io::StringOutputStream out_stream(&out);
      formatted_results.SerializeToZeroCopyStream(&out_stream);
    } else if (output_format == PDF_OUTPUT) {
      // We only over write PDF output to a file (enforced in main()).
      DCHECK(out_filename != "-");
      return GeneratePdfReportToFile(formatted_results, out_filename);
    } else {
      LOG(DFATAL) << "unexpected output_format value: " << output_format;
    }
  }

  if (out_filename == "-") {
    // Special case: if user specifies output file as '-', write the output to
    // stdout.
    std::cout << out;
  } else {
    std::ofstream out_stream(out_filename.c_str(),
                             std::ios::out | std::ios::binary);
    if (!out_stream) {
      fprintf(stderr, "Could not write output to %s.\n", out_filename.c_str());
      return false;
    }
    out_stream << out;
    out_stream.close();
  }

  return true;
}

// Helper class that will run our exit functions in its destructor.
class ScopedShutDown {
 public:
  ~ScopedShutDown() {
    pagespeed::ShutDown();
    ::google::protobuf::ShutdownProtobufLibrary();
    ::google::ShutDownCommandLineFlags();
  }
};

// We need to declare this as a global variable since gflags and other
// libraries may invoke exit(), so simply declaring a ScopedShutDown
// in main() would not guarantee that shutdown hooks get run (should
// gflags invoke exit() to terminate execution early). This is only
// necessary to make sure that valgrind, etc, don't detect leaks on
// shutdown.
ScopedShutDown g_shutdown;

}  // namespace

int main(int argc, char** argv) {
  // Some of our code uses Singleton<>s, which require an
  // AtExitManager to schedule their destruction.
  base::AtExitManager at_exit_manager;

  pagespeed::Init();

  ::google::SetUsageMessage(
      "Reads a file (such as a HAR) and emits Page Speed results "
      "in one of several formats.");
  ::google::ParseCommandLineNonHelpFlags(&argc, &argv, true);

  // We need to initialize CommandLine to support logging
  // since logging module checks for several switches from command line.
  CommandLine::Init(argc, argv);

  if (FLAGS_v || FLAGS_version) {
    PrintVersion();
    return 0;
  }
  if (FLAGS_show_locales) {
    PrintLocales();
    return 0;
  }
  if (FLAGS_input_file.empty()) {
    fprintf(stderr, "Must specify --input_file.\n");
    PrintUsage();
    return 1;
  }

  if (FLAGS_output_format == "pdf" && FLAGS_output_file == "-") {
    fprintf(stderr, "Must specify --output_file for --output_format=pdf.\n");
    PrintUsage();
    return 1;
  }

  logging::LoggingDestination log_destination =
      FLAGS_log_file.empty() ?
      logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG : logging::LOG_ONLY_TO_FILE;

  if (!FLAGS_log_file.empty() && FLAGS_also_log_to_stderr) {
    log_destination = logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG;
  }

// logging::InitLogging expects a wchar_t* on windows, and char* on
// other platforms.
#if defined (_WIN32)
  std::wstring log_file_path;
  log_file_path.reserve(FLAGS_log_file.length());

  // NOTE: this assumes that FLAGS_log_file contains no multibyte
  // characters.
  log_file_path.assign(FLAGS_log_file.begin(), FLAGS_log_file.end());
#else
  std::string log_file_path(FLAGS_log_file);
#endif

  logging::InitLogging(
      log_file_path.c_str(),
      log_destination,
      // Since we are entirely single-threaded no need to lock the log file.
      logging::DONT_LOCK_LOG_FILE,
      logging::APPEND_TO_OLD_LOG_FILE,
      logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);

  if (RunPagespeed(FLAGS_output_format,
                   FLAGS_input_format,
                   FLAGS_input_file,
                   FLAGS_dom_input_file,
                   FLAGS_instrumentation_input_file,
                   FLAGS_output_file)) {
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
