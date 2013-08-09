// Copyright 2012 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/avoid_flash_on_mobile.h"

#include "net/instaweb/util/public/google_url.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

static const char* kRuleName = "AvoidFlashOnMobile";
static const char* kFlashMime = "application/x-shockwave-flash";
static const char* kFlashClassid = "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000";

// Caller is responsible for freeing returned DomElement
const pagespeed::DomElement* GetChildNode(
    const pagespeed::DomElement* node, int idx) {
  const pagespeed::DomElement* child = NULL;
  if (node->GetChild(&child, idx) != pagespeed::DomElement::SUCCESS) {
    LOG(INFO) << "DomElement::GetChild() failed.";
  }
  return child;
}

// Check for ActiveX classid,
//    <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000">
bool DetermineIfActiveXFlash(const pagespeed::DomElement* node) {
  if (node->GetTagName() == "OBJECT") {
    std::string classid;
    if (node->GetAttributeByName("classid", &classid)
        && pagespeed::string_util::StringCaseEqual(classid, kFlashClassid)) {
      return true;
    }
  }
  return false;
}

// Searches through the children of the specified node for a tag of the form
//     <param name="movie" value="movie_name.swf"/>
// @return true if the src was found
bool PullSrcFromMovieParam(const pagespeed::DomElement* node,
                                         std::string* src) {
  size_t size = 0;
  if (node->GetNumChildren(&size) != pagespeed::DomElement::SUCCESS) {
    LOG(INFO) << "DomElement::GetNumChildren() failed.";
    return false;
  }
  for (size_t idx = 0; idx < size; ++idx) {
    scoped_ptr<const pagespeed::DomElement> child(GetChildNode(node, idx));
    if (child == NULL) {
      LOG(INFO) << "DomElement::GetChild() returned NULL.";
      continue;
    }
    if (child->GetTagName() == "PARAM") {
      std::string name;
      if (child->GetAttributeByName("name", &name)
          && pagespeed::string_util::StringCaseEqual(name, "movie")
          && child->GetAttributeByName("value", src)) {
        return true;
      }
    }
  }
  return false;
}

class FlashChecker : public pagespeed::DomElementVisitor {
 public:
  FlashChecker(const pagespeed::RuleInput* rule_input,
               const pagespeed::DomDocument* document,
               pagespeed::ResultProvider* provider)
      : rule_input_(rule_input),
        document_(document),
        provider_(provider) {}

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  void ProcessFlashIncludeTag(const pagespeed::DomElement* node);
  bool DetermineIfFlashAndGetURI(const pagespeed::DomElement* node,
                                 std::string* uri);
  bool DetermineIfUriIsFlash(const std::string uri);
  bool HasChildFlashElement(const pagespeed::DomElement* node);

  const pagespeed::RuleInput* rule_input_;
  const pagespeed::DomDocument* document_;
  pagespeed::ResultProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(FlashChecker);
};

void FlashChecker::Visit(const pagespeed::DomElement& node) {
  std::string tag_name = node.GetTagName();

  if (tag_name == "EMBED" || tag_name == "OBJECT") {
    if (!rule_input_->pagespeed_input().has_resource_with_url(
        document_->GetDocumentUrl())) {
      return;
    }
    ProcessFlashIncludeTag(&node);
  } else if (node.GetTagName() == "IFRAME") {
    // Do a recursive document traversal.
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      FlashChecker checker(rule_input_, child_doc.get(), provider_);
      child_doc->Traverse(&checker);
    }
  }
}

void FlashChecker::ProcessFlashIncludeTag(const pagespeed::DomElement* node) {
  std::string tag_name = node->GetTagName();
  CHECK(tag_name == "EMBED" || tag_name == "OBJECT");

  // Skip this tag if a child node embeds flash, as we will record the child
  // when the visitor reaches the child. This will avoid double counting nested
  // objects / "twice cooked" embedding methods at the cost of checking the
  // child node for flash twice
  if (DetermineIfActiveXFlash(node) && HasChildFlashElement(node)) {
    return;
  }

  std::string uri;
  if (!DetermineIfFlashAndGetURI(node, &uri)) {
    return;
  }

  pagespeed::Result* result = provider_->NewResult();
  result->add_resource_urls(uri);

  pagespeed::ResultDetails* details = result->mutable_details();
  pagespeed::AvoidFlashOnMobileDetails* avoid_flash_details = details
      ->MutableExtension(
      pagespeed::AvoidFlashOnMobileDetails::message_set_extension);

  std::string width;
  if (node->GetAttributeByName("width", &width)) {
    avoid_flash_details->set_width(width);
  }

  std::string height;
  if (node->GetAttributeByName("height", &height)) {
    avoid_flash_details->set_height(height);
  }
}

// Possible "valid" combinations:
//  Standard EMBED:
//    <embed type="application/x-shockwave-flash" src="<src>">
//
//  Standard OBJECT:
//    <object type="application/x-shockwave-flash" data="movie_name.swf">
//
//  ActiveX OBJECT:
//    <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000">
//      <param name="movie" value="<src>">
//    </object>
//
//  EMBED missing type:
//      - Works in Chrome 21 and FF 12:
//    <embed src="<src>">
//
//  OBJECT missing type:
//      - Works in Chrome 21, fails in FF 12:
//    <object data="movie_name.swf">
//
//  OBJECT with type and movie param, but no data
//      - Works in Chrome 21, fails in FF 12:
//    <object type="application/x-shockwave-flash>
//      <param name="movie" value="<src>">
//    </object>
bool FlashChecker::DetermineIfFlashAndGetURI(const pagespeed::DomElement* node,
                                             std::string* uri) {
  std::string src;
  std::string type;
  std::string tag_name = node->GetTagName();
  bool is_flash = false;

  uri->clear();

  // First place to check if this is flash is a MIME in the type attribute
  if (node->GetAttributeByName("type", &type)) {
    // MIME types are case insensitive per RFC 2045
    if (pagespeed::string_util::StringCaseEqual(type, kFlashMime)) {
      is_flash = true;
    } else {
      return false;
    }
  }

  // Next, look for IE's ActiveX classid
  is_flash = is_flash || DetermineIfActiveXFlash(node);

  // Look for the src of the flash object
  if (tag_name == "EMBED") {
    if (!node->GetAttributeByName("src", &src)) {
      return false;
    }
  } else if (tag_name == "OBJECT") {
    // OBJECT is difficult. Look for a data attribute, and if that fails, look
    // for a child "movie" param if we have already determined the tag is flash
    if (!node->GetAttributeByName("data", &src)) {
      if (is_flash && !PullSrcFromMovieParam(node, &src)) {
        return false;
      }
    }
  }

  uri->assign(document_->ResolveUri(src));

  // Return true if we already know the tag is flash, or look at the URI if we
  // were unlucky enough to not have a type or classid
  return (is_flash || DetermineIfUriIsFlash(*uri));
}

bool FlashChecker::DetermineIfUriIsFlash(const std::string uri) {
  // See if we fetched the resource and have its MIME type
  const pagespeed::Resource* resource = rule_input_->pagespeed_input()
      .GetResourceCollection().GetRedirectRegistry()->GetFinalRedirectTarget(
      rule_input_->pagespeed_input().GetResourceWithUrlOrNull(uri));

  if (resource != NULL) {
    return (resource->GetResourceType() == pagespeed::FLASH);
  }

  // Last ditch effort, guess if the URI is Flash from the extension
  net_instaweb::GoogleUrl google_url(uri);
  base::StringPiece no_query = google_url.AllExceptQuery();
  return pagespeed::string_util::StringCaseEndsWith(no_query, ".swf");
}

// Check if the node contains a tag embedding a flash object as a direct
// descendant, useful to avoid double counting duplicated ActiveX classid and
// application/x-shockwave-flash MIME object tags
bool FlashChecker::HasChildFlashElement(const pagespeed::DomElement* node) {
  size_t size = 0;
  if (node->GetNumChildren(&size) != pagespeed::DomElement::SUCCESS) {
    LOG(INFO) << "DomElement::GetNumChildren() failed.";
    return false;
  }
  for (size_t idx = 0; idx < size; ++idx) {
    scoped_ptr<const pagespeed::DomElement> child(GetChildNode(node, idx));
    if (child == NULL) {
      LOG(INFO) << "Child node " << idx << " out of " << size << " was NULL.";
      continue;
    }
    std::string child_tag_name = child->GetTagName();
    if (child_tag_name == "EMBED" || child_tag_name == "OBJECT") {
      std::string uri;
      if (DetermineIfFlashAndGetURI(child.get(), &uri)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

namespace pagespeed {

namespace rules {

AvoidFlashOnMobile::AvoidFlashOnMobile()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::RESPONSE_BODY)) {}

const char* AvoidFlashOnMobile::name() const {
  return kRuleName;
}

UserFacingString AvoidFlashOnMobile::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to avoid using
  // Adobe Flash on mobile webpages
  return _("Avoid flash on mobile webpages");
}

bool AvoidFlashOnMobile::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  const DomDocument* document = rule_input.pagespeed_input().dom_document();
  if (document) {
    FlashChecker visitor(&rule_input, document, provider);
    document->Traverse(&visitor);
  }
  return true;
}

void AvoidFlashOnMobile::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs of Adobe Flash
      // resources detected by Page Speed. "NUM_ELEMENTS" will be replaced by
      // the number of Flash elements found.
      _("The following %(NUM_ELEMENTS)s Flash elements are included on the "
        "page or from included iframes. Adobe Flash Player is not supported on "
        "Apple iOS or Android versions greater than 4.0.x. Consider removing "
        "Flash objects and finding suitable replacements."),
      IntArgument("NUM_ELEMENTS", results.size()));

  for (ResultVector::const_iterator iter = results.begin(), end = results.end();
      iter != end; ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (details.HasExtension(
        AvoidFlashOnMobileDetails::message_set_extension)) {
      const AvoidFlashOnMobileDetails& flash_details = details.GetExtension(
          AvoidFlashOnMobileDetails::message_set_extension);
      if (flash_details.has_width() && flash_details.has_height()) {
        body->AddUrlResult(not_localized("%(URL)s (%(WIDTH)s x %(HEIGHT)s)"),
                           UrlArgument("URL", result.resource_urls(0)),
                           StringArgument("WIDTH", flash_details.width()),
                           StringArgument("HEIGHT", flash_details.height()));
      } else {
        body->AddUrl(result.resource_urls(0));
      }
    } else {
      body->AddUrl(result.resource_urls(0));
    }
  }
}

int AvoidFlashOnMobile::ComputeScore(const InputInformation& input_info,
                                     const RuleResults& results) {
  // Scoring is binary: Flash == bad; no flash == good
  if (results.results_size() > 0) {
    return 0;
  }
  return 100;
}

bool AvoidFlashOnMobile::IsExperimental() const {
  return true;
}

}  // namespace rules

}  // namespace pagespeed
