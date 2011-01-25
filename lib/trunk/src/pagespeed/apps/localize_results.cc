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

// Command line utility that takes in a Results proto and prints it, formatted
// and localized.

#include <stdio.h>

#include <fstream>
#include <iostream>

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/string_util.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/l10n/gettext_localizer.h"
#include "pagespeed/l10n/register_locale.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/rules/rule_provider.h"

// TODO(aoates): merge this into pagespeed_bin

namespace {

void PrintUsage() {
  fprintf(stderr, "Usage: localize_results <locale> <input>\n");
}

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

std::string DoFormatString(const pagespeed::FormatString& str) {
  std::vector<std::string> subst;

  for (int i = 0; i < str.args_size(); ++i) {
    const pagespeed::FormatArgument& arg = str.args(i);
    std::string val = arg.localized_value();

    switch (arg.type()) {
      case pagespeed::FormatArgument::URL:
        if (val.length() > 80)
          val = val.substr(0, 80) + "...";
        subst.push_back("[" + val + "]");
        break;
      default:
        subst.push_back(val);
        break;
    }
  }

  return ReplaceStringPlaceholders(str.format(), subst, NULL);
}

bool PrintFormattedResultsToStream(const pagespeed::FormattedResults& results,
                          std::ostream& out) {
  std::string indent = "  ";
  out << "Locale: " << results.locale() << "\n\n";

  for (int rule_idx = 0; rule_idx < results.rule_results_size(); ++rule_idx) {
    const pagespeed::FormattedRuleResults& rule_results =
        results.rule_results(rule_idx);

    out << "[" << rule_results.localized_rule_name() << "]\n";

    for (int block_idx = 0;
         block_idx < rule_results.url_blocks_size();
         block_idx++) {
      const pagespeed::FormattedUrlBlockResults& block =
          rule_results.url_blocks(block_idx);

      if (block.has_header())
        out << indent << DoFormatString(block.header()) << "\n";

      for (int url_idx = 0; url_idx < block.urls_size(); ++url_idx) {
        const pagespeed::FormattedUrlResult& url = block.urls(url_idx);
        out << indent << indent << "* "
            << DoFormatString(url.result()) << "\n";

        for (int detail_idx = 0;
             detail_idx < url.details_size();
             ++detail_idx) {
          out << indent << indent << indent
              << "o " << DoFormatString(url.details(detail_idx)) << "\n";
        }
      }

      if (block_idx < rule_results.url_blocks_size()-1)
        out << "\n";
    }

    out << "\n\n";
  }

  return true;
}

bool LocalizeResults(const std::string& locale,
                     const pagespeed::Results& results,
                     pagespeed::FormattedResults* out) {
  // Allocate all the rules we know about
  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  pagespeed::rule_provider::AppendAllRules(false, &rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  scoped_ptr<pagespeed::l10n::Localizer> localizer;
  localizer.reset(pagespeed::l10n::GettextLocalizer::Create(locale));
  if (!localizer.get()) {
    std::cerr << "error: locale '" << locale << "' not found" << std::endl;
    std::cerr << "available locales: " << std::endl;

    std::vector<std::string> locales;
    pagespeed::l10n::RegisterLocale::GetAllLocales(&locales);
    for (size_t i=0; i < locales.size(); i++)
      std::cerr << "  " << locales[i] << std::endl;

    return false;
  }
  pagespeed::formatters::ProtoFormatter formatter(localizer.get(), out);

  return engine.FormatResults(results, &formatter);
}

bool LocalizeResults(const std::string& locale, const std::string& fname) {
  std::string file_contents;
  if (!ReadFileToString(fname, &file_contents))
    return false;

  pagespeed::Results results;
  results.ParseFromString(file_contents);
  if (!results.IsInitialized()) {
    std::cerr << "error: could not parse input file" << std::endl;
    return false;
  }

  pagespeed::FormattedResults localized_results;
  localized_results.set_locale(locale);
  if (!LocalizeResults(locale, results, &localized_results)) {
    std::cerr << "error: could not localize results" << std::endl;
    return false;
  }

  return PrintFormattedResultsToStream(localized_results, std::cout);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    PrintUsage();
    return 1;
  }

  // Some of our code uses Singleton<>s, which require an
  // AtExitManager to schedule their destruction.
  base::AtExitManager at_exit_manager;

  pagespeed::Init();
  bool result = LocalizeResults(argv[1], argv[2]);
  pagespeed::ShutDown();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
