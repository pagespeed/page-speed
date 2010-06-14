// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/script_tag_scanner.h"

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

ScriptTagScanner::ScriptTagScanner(HtmlParse* html_parse)
    : s_script_(html_parse->Intern("script")),
      s_src_(html_parse->Intern("src")) {
}

}  // namespace net_instaweb
