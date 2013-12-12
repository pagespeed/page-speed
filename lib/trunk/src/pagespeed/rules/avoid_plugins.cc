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

#include "pagespeed/rules/avoid_plugins.h"

#include <stddef.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_piece.h"
#include "net/instaweb/util/public/basictypes.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/input_capabilities.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_collection.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

using pagespeed::AvoidPluginsDetails_PluginType;
using pagespeed::AvoidPluginsDetails_PluginType_UNKNOWN;
using pagespeed::AvoidPluginsDetails_PluginType_FLASH;
using pagespeed::AvoidPluginsDetails_PluginType_SILVERLIGHT;
using pagespeed::AvoidPluginsDetails_PluginType_JAVA;
using pagespeed::string_util::StringCaseEndsWith;
using pagespeed::string_util::StringCaseStartsWith;

namespace {

// Plain old data pair of a PluginType and a string.
struct PluginID {
  AvoidPluginsDetails_PluginType type;
  const char* id;
};

// Table of plugin types.
// These are matched as prefixes, so application/x-silverlight can also be used
// to match application/x-silverlight-2.
static const PluginID kPluginMimeTypes[] = {
    {AvoidPluginsDetails_PluginType_FLASH, "application/x-shockwave-flash"},
    {AvoidPluginsDetails_PluginType_SILVERLIGHT, "application/x-silverlight"},
    {AvoidPluginsDetails_PluginType_JAVA, "application/x-java-applet"},
    {AvoidPluginsDetails_PluginType_JAVA, "application/java"}
};

// Whitelist of MIME prefixes that most browsers can directly interpret without
// a plugin.
static const char* kAllowedMimeTypes[] = {
    "image/",
    "audio/",
    "video/",
    "text/",
    // Allow preloaded javascript hacks:
    // http://www.phpied.com/preload-cssjavascript-without-execution/
    "application/javascript",
    "application/x-javascript"
};

// Table of ActiveX classids.
// Classids must be lowercase, are compared as a prefix.
static const PluginID kPluginClassids[] = {
    {AvoidPluginsDetails_PluginType_FLASH,
        "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"},
    {AvoidPluginsDetails_PluginType_JAVA,
        "clsid:8ad9c840-044e-11d1-b3e9-00805f499d93"},
    // This should be "clsid:cafeefac-????-????-????-abcdeffedcba", but we
    // don't have access to a regex engine. Using a prefix should work well
    // enough.
    {AvoidPluginsDetails_PluginType_JAVA, "clsid:cafeefac-"}
};

// Table of plugin extensions for heuristic type detection.
// Must include the ".".
static const PluginID kPluginFileExtensions[] = {
    {AvoidPluginsDetails_PluginType_FLASH, ".swf"},
    {AvoidPluginsDetails_PluginType_SILVERLIGHT, ".xap"},
    {AvoidPluginsDetails_PluginType_JAVA, ".class"},
    {AvoidPluginsDetails_PluginType_JAVA, ".jar"}
};

// Whitelist of file extensions most browsers can directly interpret without
// a plugin. Must include the ".".
static const char* kAllowedFileExtensions[] = {
    ".mp3",
    ".mp4",
    ".ogg",
    ".wav",
    ".webm"
};

// Human-readable names for known plugin types. As these are product names,
// they should not be localized.
static const PluginID kPluginHumanNames[] = {
    {AvoidPluginsDetails_PluginType_FLASH, "Flash"},
    {AvoidPluginsDetails_PluginType_SILVERLIGHT, "Silverlight"},
    {AvoidPluginsDetails_PluginType_JAVA, "Java"}
};

// Base impact to be assigned to all plugins. Currently equal to
// Rule::kImpactMediumCutoff
static const double kPluginBaseImpact = 3.0;

// If we have dimensions for a plugin, it's impact will be the kPluginBaseImpact
// plus the percentage of the ATF content covered by this plugin times this
// constant. The logic behind the current value is if a single plugin is more
// than 20% of the ATF content, it should trigger a high impact result.
// Therefore, it is currently equal to
// (Rule::kImpactHighCutoff - Rule::kImpactMediumCutoff) / 0.2.
static const double kPluginAtfImpactMultiplier = (10.0 - 3.0) / 0.2;

// Maximum depth to recurse when looking for a child of an object or element
// tag that embeds a plugin in order to avoid double counting nested tags.
//
// Some recursion is necessary to catch the Java examples such as:
//
// <object classid="clsid:CAFEEFAC-0015-0000-0000-ABCDEFFEDCBA">
//   <param name="code" value="Applet1.class">
//   <comment>
//     <embed code="Applet1.class"
//         type="application/x-java-applet;jpi-version=1.5.0">
//       <noembed>
//         No Java Support.
//       </noembed>
//     </embed>
//   </comment>
// </object>
static const int kMaximumChildRecursionDepth = 5;

// Caller is responsible for freeing returned DomElement
const pagespeed::DomElement* GetChildNode(
    const pagespeed::DomElement& node, int idx) {
  const pagespeed::DomElement* child = NULL;
  if (node.GetChild(&child, idx) != pagespeed::DomElement::SUCCESS) {
    LOG(INFO) << "DomElement::GetChild() failed.";
  }
  return child;
}

// Searches through the direct children of the node for a tag of the form
// <param name="param_name" value="out_value"/>. Returns true and sets
// out_value if a matching parameter was found.
bool PullSrcFromParam(const pagespeed::DomElement& node,
                      const std::string param_name,
                      std::string* out_value) {
  out_value->clear();

  size_t size = 0;
  if (node.GetNumChildren(&size) != pagespeed::DomElement::SUCCESS) {
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
      if (child->GetAttributeByName("name", &name) &&
          pagespeed::string_util::StringCaseEqual(name, param_name) &&
          child->GetAttributeByName("value", out_value)) {
        return true;
      }
    }
  }
  return false;
}

bool DetermineTypeFromClassid(const pagespeed::DomElement& node,
                              AvoidPluginsDetails_PluginType* out_type) {
  std::string classid;
  if (node.GetAttributeByName("classid", &classid)) {
    pagespeed::string_util::StringToLowerASCII(&classid);
    for (size_t i = 0; i < arraysize(kPluginClassids); ++i) {
      const PluginID& plugin_id = kPluginClassids[i];
      // Ideally this would be a regex for matching Java's versioned classids,
      // but a prefix works well enough.
      if (pagespeed::string_util::StringCaseStartsWith(classid, plugin_id.id)) {
        *out_type = plugin_id.type;
        return true;
      }
    }
    *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
    return true;
  }
  return false;
}

AvoidPluginsDetails_PluginType DetermineTypeFromMime(
    const std::string& mime_type) {
  for (size_t i = 0; i < arraysize(kPluginMimeTypes); ++i) {
    const PluginID& plugin_id = kPluginMimeTypes[i];
    // Known type attributes should be treated as a prefix, as valid types
    // can take the form "application/x-java-applet;jpi-version=1.5.0". This
    // also allows us to use "application/x-silverlight" to match
    // "application/x-silverlight-2".
    // MIME types are case insensitive per RFC 2045.
    if (pagespeed::string_util::StringCaseStartsWith(mime_type, plugin_id.id)) {
      return plugin_id.type;
    }
  }
  return AvoidPluginsDetails_PluginType_UNKNOWN;
}

// If true, this MIME type can likely be interpreted directly by browsers and
// should not be recorded as a plugin.
bool DetermineAllowedFromMime(const std::string& mime_type) {
  for (size_t i = 0; i < arraysize(kAllowedMimeTypes); ++i) {
    if (StringCaseStartsWith(mime_type, kAllowedMimeTypes[i])) {
      return true;
    }
  }
  return false;
}

// If true, this file's extension can likely be interpreted directly by
// browsers and should not be recorded as a plugin. The provided url should not
// contain any query params or fragments.
bool DetermineAllowedFromExtension(const std::string& url) {
  for (size_t i = 0; i < arraysize(kAllowedFileExtensions); ++i) {
    if (StringCaseEndsWith(url, kAllowedFileExtensions[i])) {
      return true;
    }
  }
  return false;
}

class PluginElementVisitor : public pagespeed::DomElementVisitor {
 public:
  PluginElementVisitor(const pagespeed::RuleInput* rule_input,
               const pagespeed::DomDocument* document,
               pagespeed::ResultProvider* provider);

  void SetFrameVisible(bool visible);
  void SetFrameBounds(int x1, int y1, int x2, int y2);

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  void AddResult(const pagespeed::DomElement& node,
                 AvoidPluginsDetails_PluginType type,
                 const std::string& mime,
                 const std::string& src);
  bool NodeHasPlugin(const pagespeed::DomElement& node,
                     AvoidPluginsDetails_PluginType* out_type,
                     std::string* out_mime,
                     std::string* out_src);
  bool HasChildPlugin(const pagespeed::DomElement& node);
  bool HasChildPluginHelper(const pagespeed::DomElement& node, int depth);
  bool ProcessObjectTag(const pagespeed::DomElement& node,
                        AvoidPluginsDetails_PluginType* out_type,
                        std::string* out_mime,
                        std::string* out_url);
  bool ProcessEmbedTag(const pagespeed::DomElement& node,
                       AvoidPluginsDetails_PluginType* out_type,
                       std::string* out_mime,
                       std::string* out_url);
  bool ProcessAppletTag(const pagespeed::DomElement& node,
                        AvoidPluginsDetails_PluginType* out_type,
                        std::string* out_url);
  void DetermineTypeFromUrl(const std::string url, bool* out_allowed,
                            AvoidPluginsDetails_PluginType* out_type,
                            std::string* out_mime);
  bool DetermineJavaUrlFromAttributes(const pagespeed::DomElement& node,
                                      std::string* out_url);
  bool DetermineJavaUrl(const std::string& archive,
                        const std::string& code,
                        const std::string& object,
                        const std::string& codebase,
                        std::string* out_url);

  const pagespeed::RuleInput* rule_input_;
  const pagespeed::DomDocument* document_;
  bool frame_visible_;

  // Bounds of this frame within the window. If frame_x2_ and frame_y2 are -1,
  // the width and height of the frame are treated as unbounded.
  int frame_x1_;
  int frame_y1_;
  int frame_x2_;
  int frame_y2_;
  pagespeed::ResultProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(PluginElementVisitor);
};

PluginElementVisitor::PluginElementVisitor(
    const pagespeed::RuleInput* rule_input,
    const pagespeed::DomDocument* document,
    pagespeed::ResultProvider* provider) :
        rule_input_(rule_input),
        document_(document),
        frame_visible_(true),
        frame_x1_(0),
        frame_y1_(0),
        frame_x2_(-1),
        frame_y2_(-1),
        provider_(provider) {
}

// Sets the bounds of this frame within the main window. Must describe a non
// zero sized box in at least partially in positive coordinate space,
// or else will set the frame visibility to false instead.
void PluginElementVisitor::SetFrameBounds(int x1, int y1, int x2, int y2) {
  if (x2 < 0 || y2 < 0 || x2 < x1 || y2 < y1) {
    SetFrameVisible(false);
    return;
  }

  frame_x1_ = x1;
  frame_y1_ = y1;
  frame_x2_ = x2;
  frame_y2_ = y2;
}

// Sets whether this frame is visible in the main window.
void PluginElementVisitor::SetFrameVisible(bool visible) {
  frame_visible_ = visible;
}

void PluginElementVisitor::Visit(const pagespeed::DomElement& node) {
  std::string tag_name = node.GetTagName();

  // Do a recursive document traversal for iframes.
  if (tag_name == "IFRAME") {
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      PluginElementVisitor checker(rule_input_, child_doc.get(), provider_);
      int x, y, width, height;
      if (frame_visible_ &&
          node.GetX(&x) == pagespeed::DomElement::SUCCESS &&
          node.GetY(&y) == pagespeed::DomElement::SUCCESS &&
          node.GetActualWidth(&width) == pagespeed::DomElement::SUCCESS &&
          node.GetActualHeight(&height) == pagespeed::DomElement::SUCCESS) {
        const int x1 = frame_x1_ + x;
        const int y1 = frame_y1_ + y;
        int x2 = x1 + width;
        int y2 = y1 + height;
        // If the x2 and y2 coordinates of the frame containing this iframe are
        // bounded, clip the iframe's x2 and y2 coordinates to match.
        if (frame_x2_ >= 0 && frame_y2_ >= 0) {
          x2 = std::min(x2, frame_x2_);
          y2 = std::min(y2, frame_y2_);
        }
        checker.SetFrameBounds(x1, y1, x2, y2);
      } else {
        checker.SetFrameVisible(false);
      }
      child_doc->Traverse(&checker);
    }
  }

  // Check if this node contains a plugin, and record it if it does.
  AvoidPluginsDetails_PluginType type = AvoidPluginsDetails_PluginType_UNKNOWN;
  std::string mime;
  std::string url;
  if (NodeHasPlugin(node, &type, &mime, &url)) {
    AddResult(node, type, mime, url);
  }
}

void PluginElementVisitor::AddResult(const pagespeed::DomElement& node,
                                     AvoidPluginsDetails_PluginType type,
                                     const std::string& mime,
                                     const std::string& url) {
  pagespeed::Result* result = provider_->NewResult();
  result->add_resource_urls(url);

  pagespeed::ResultDetails* details = result->mutable_details();
  pagespeed::AvoidPluginsDetails* avoid_plugins_details = details
      ->MutableExtension(
      pagespeed::AvoidPluginsDetails::message_set_extension);

  avoid_plugins_details->set_type(type);
  if (!mime.empty()) {
    avoid_plugins_details->set_mime(mime);
  }
  std::string classid;
  if (node.GetAttributeByName("classid", &classid)) {
    avoid_plugins_details->set_classid(classid);
  }

  // Don't attempt to calculate dimensions if we are in a frame that does not
  // have layout coordinates.
  if (!frame_visible_) {
    return;
  }

  int x, y, width, height;
  if (node.GetX(&x) == pagespeed::DomElement::SUCCESS &&
      node.GetY(&y) == pagespeed::DomElement::SUCCESS &&
      node.GetActualWidth(&width) == pagespeed::DomElement::SUCCESS &&
      node.GetActualHeight(&height) == pagespeed::DomElement::SUCCESS) {
    // Translate the bounds of this plugin based on the containing frame.
    const int x1 = std::max(0, frame_x1_ + x);
    const int y1 = std::max(0, frame_y1_ + y);
    int x2 = std::max(0, frame_x1_ + x + width);
    int y2 = std::max(0, frame_y1_ + y + height);

    // If the x2 and y2 coordinates of the frame containing this plugin are
    // bounded, clip the plugin's x2 and y2 coordinates to match.
    if (frame_x2_ > 0 && frame_y2_ > 0) {
      x2 = std::min(x2, frame_x2_);
      y2 = std::min(y2, frame_y2_);
    }

    avoid_plugins_details->set_x(x1);
    avoid_plugins_details->set_y(y1);
    avoid_plugins_details->set_width(x2 - x1);
    avoid_plugins_details->set_height(y2 - y1);

    const int viewport_w = rule_input_->pagespeed_input().viewport_width();
    const int viewport_h = rule_input_->pagespeed_input().viewport_height();

    // Sanity check that the viewport width and height are set before
    // calculating ATF percentage.
    if (viewport_w > 0 && viewport_h > 0) {
      const int atf_pixels = viewport_w * viewport_h;

      const int clamped_x1 = std::min(std::max(x1, 0), viewport_w);
      const int clamped_y1 = std::min(std::max(y1, 0), viewport_h);
      const int clamped_x2 = std::min(std::max(x2, 0), viewport_w);
      const int clamped_y2 = std::min(std::max(y2, 0), viewport_h);

      const int atf_plugin_pixels =
          (clamped_x2 - clamped_x1) * (clamped_y2 - clamped_y1);

      avoid_plugins_details->set_atf_ratio(
          static_cast<double>(atf_plugin_pixels) /
          static_cast<double>(atf_pixels));
    }
  }
}

bool PluginElementVisitor::NodeHasPlugin(
    const pagespeed::DomElement& node,
    AvoidPluginsDetails_PluginType* out_type,
    std::string* out_mime,
    std::string* out_src) {
  *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
  out_mime->clear();
  out_src->clear();

  std::string tag_name = node.GetTagName();
  if (tag_name == "EMBED") {
    return ProcessEmbedTag(node, out_type, out_mime, out_src);
  } else if (tag_name == "OBJECT") {
    return ProcessObjectTag(node, out_type, out_mime, out_src);
  } else if (tag_name == "APPLET") {
    return ProcessAppletTag(node, out_type, out_src);
  }
  return false;
}

// Recursively check if a child of this node embeds a plugin. Useful for
// avoiding double counting nested object and element tags.
bool PluginElementVisitor::HasChildPlugin(const pagespeed::DomElement& node) {
  return HasChildPluginHelper(node, 0);
}

bool PluginElementVisitor::HasChildPluginHelper(
    const pagespeed::DomElement& node, int depth) {
  size_t size = 0;
  if (node.GetNumChildren(&size) != pagespeed::DomElement::SUCCESS) {
    LOG(INFO) << "DomElement::GetNumChildren() failed.";
    return false;
  }
  for (size_t idx = 0; idx < size; ++idx) {
    scoped_ptr<const pagespeed::DomElement> child(GetChildNode(node, idx));
    if (child == NULL) {
      LOG(INFO) << "Child node " << idx << " out of " << size << " was NULL.";
      continue;
    }

    // Check if this child embeds a plugin.
    AvoidPluginsDetails_PluginType type;
    std::string src;
    std::string mime_type;
    if (NodeHasPlugin(*child, &type, &mime_type, &src)) {
      return true;
    }

    // Recursively check children of this child node if we haven't exceeded the
    // maximum depth. We've already checked one level of children above, so
    // add one to depth for the check.
    if ((depth + 1) < kMaximumChildRecursionDepth &&
        HasChildPluginHelper(*child, depth + 1)) {
      return true;
    }
  }
  return false;
}

bool PluginElementVisitor::ProcessObjectTag(
    const pagespeed::DomElement& node,
    AvoidPluginsDetails_PluginType* out_type,
    std::string* out_mime,
    std::string* out_url) {
  DCHECK(node.GetTagName() == "OBJECT");
  *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
  out_mime->clear();
  out_url->clear();

  // A common strategy for embedding Flash and Java is to use an object tag
  // with a classid for IE containing an embed tag for everything else. Since
  // we don't want to report the same piece of content twice, we check if a
  // child of this node embeds a plugin. If so, skip processing this tag since
  // we'll record the child.
  if (HasChildPlugin(node)) {
    return false;
  }

  // Try and set the MIME type first from the type attribute, and if it isn't
  // present or is empty, look in the codetype attribute. If neither is set,
  // make sure we don't report a MIME type.
  if (!node.GetAttributeByName("type", out_mime) || out_mime->empty()) {
    if (!node.GetAttributeByName("codetype", out_mime)) {
      out_mime->clear();
    }
  }

  bool found_type = DetermineTypeFromClassid(node, out_type);
  if (!found_type && !out_mime->empty()) {
    *out_type = DetermineTypeFromMime(*out_mime);
    // At this point, we know the MIME type from the tag's attributes. Even
    // if we didn't recognize the type as one of our known plugins, the type
    // was still identified and we shouldn't continue with further heuristics.
    // Put more amusingly: at this point, the type is known to be
    // PluginType_UNKNOWN, where as before it was an unknown unknown.
    found_type = true;

    if (*out_type == AvoidPluginsDetails_PluginType_UNKNOWN) {
      if (DetermineAllowedFromMime(*out_mime)) {
        return false;
      }
    }
  }

  bool found_src = false;
  std::string src;

  switch (*out_type) {
    case AvoidPluginsDetails_PluginType_FLASH:
      // First check for a data attribute, and if not found, check for a
      // child movie param.
      found_src = node.GetAttributeByName("data", &src) ||
          PullSrcFromParam(node, "movie", &src);
      break;
    case AvoidPluginsDetails_PluginType_SILVERLIGHT:
      // Silverlight docs recommend setting the data attribute to
      // data="data:application/x-silverlight-2," for performance reasons in
      // some browsers. Therefore, the data attribute is generally a useless
      // data URI and we should skip straight to the param.
      found_src = PullSrcFromParam(node, "source", &src);
      break;
    case AvoidPluginsDetails_PluginType_JAVA:
      {
        std::string code;
        std::string object;
        std::string archive;
        std::string codebase;

        PullSrcFromParam(node, "code", &code);
        PullSrcFromParam(node, "object", &object);
        PullSrcFromParam(node, "archive", &archive);
        PullSrcFromParam(node, "codebase", &codebase);

        DetermineJavaUrl(code, object, archive, codebase, &src);
        found_src = true;
      }
      break;
    case AvoidPluginsDetails_PluginType_UNKNOWN:
      FALLTHROUGH_INTENDED;
    default:
      // For plugin types we don't know about, the best we can do is hope that
      // the standard "data" attribute contains the source URL unless we want
      // to look through the params for something that looks like a URL.
      found_src = node.GetAttributeByName("data", &src);
      break;
  }

  if (found_src) {
    // HTML 4.01 specifies the codebase attribute as "the base path used to
    // resolve relative URIs specified by the classid, data, and archive
    // attributes. When absent, its default value is the base URI of the
    // current document." In practice, this attribute was used for the plugin
    // download page, and this step isn't in the WHATWG spec. Therefore, parse
    // the URL relative to the document.
    *out_url = document_->ResolveUri(src);
    if (!found_type) {
      bool allowed_type;
      DetermineTypeFromUrl(*out_url, &allowed_type, out_type, out_mime);
      if (allowed_type) {
        return false;
      }
    }
  }

  return true;
}

bool PluginElementVisitor::ProcessEmbedTag(
    const pagespeed::DomElement& node,
    AvoidPluginsDetails_PluginType* out_type,
    std::string* out_mime,
    std::string* out_url) {
  DCHECK(node.GetTagName() == "EMBED");
  *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
  out_mime->clear();
  out_url->clear();

  node.GetAttributeByName("type", out_mime);

  bool found_type = false;
  // If the type attribute is present and not empty, determine the plugin type.
  if (node.GetAttributeByName("type", out_mime) && !out_mime->empty()) {
    *out_type = DetermineTypeFromMime(*out_mime);
    found_type = true;

    if (*out_type == AvoidPluginsDetails_PluginType_UNKNOWN) {
      if (DetermineAllowedFromMime(*out_mime)) {
        return false;
      }
    }
  }

  bool found_src = false;
  std::string src;

  switch (*out_type) {
    case AvoidPluginsDetails_PluginType_JAVA:
      found_src = DetermineJavaUrlFromAttributes(node, &src) ||
          node.GetAttributeByName("src", &src);
      break;
    case AvoidPluginsDetails_PluginType_FLASH:
      FALLTHROUGH_INTENDED;  // Uses src attribute.
    case AvoidPluginsDetails_PluginType_SILVERLIGHT:
      FALLTHROUGH_INTENDED;  // Not expected in embed tag.
    case AvoidPluginsDetails_PluginType_UNKNOWN:
      FALLTHROUGH_INTENDED;
    default:
      found_src = node.GetAttributeByName("src", &src);
      break;
  }

  if (found_src) {
    *out_url = document_->ResolveUri(src);
    if (!found_type) {
      bool allowed_type;
      DetermineTypeFromUrl(*out_url, &allowed_type, out_type, out_mime);
      if (allowed_type) {
        return false;
      }
    }
  }
  return true;
}

bool PluginElementVisitor::ProcessAppletTag(
    const pagespeed::DomElement& node,
    AvoidPluginsDetails_PluginType* out_type,
    std::string* out_url) {
  DCHECK(node.GetTagName() == "APPLET");

  *out_type = AvoidPluginsDetails_PluginType_JAVA;
  out_url->clear();
  DetermineJavaUrlFromAttributes(node, out_url);
  return true;
}

// Determine the plugin type given a URL. If the resource is in the
// PagespeedInput, uses the Content-Type header and sets out_mime. Otherwise,
// guesses from the file extension and does not change out_mime.
void PluginElementVisitor::DetermineTypeFromUrl(
    const std::string url, bool* out_allowed,
    AvoidPluginsDetails_PluginType* out_type, std::string* out_mime) {
  *out_allowed = false;

  if (url.empty()) {
    *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
    return;
  }

  // See if we fetched the resource and have its MIME type.
  const pagespeed::Resource* resource = NULL;
  const pagespeed::Resource* resource_pre_redirection =
      rule_input_->pagespeed_input().GetResourceWithUrlOrNull(url);
  if (resource_pre_redirection != NULL) {
    resource = rule_input_->pagespeed_input().GetResourceCollection()
        .GetRedirectRegistry()->GetFinalRedirectTarget(
        resource_pre_redirection);
  }

  if (resource != NULL) {
    std::string content_type = resource->GetResponseHeader("Content-Type");
    if (!content_type.empty()) {
      *out_mime = content_type;
      *out_type = DetermineTypeFromMime(content_type);
      if (*out_type == AvoidPluginsDetails_PluginType_UNKNOWN) {
        *out_allowed = DetermineAllowedFromMime(content_type);
      }
      return;
    }
  }

  // If we don't have the resource or it doesn't have a content type header,
  // guess from the file extension.
  std::string url_no_query;
  if (pagespeed::uri_util::GetUriWithoutQueryOrFragment(
          document_->ResolveUri(url), &url_no_query)) {
    for (size_t i = 0; i < arraysize(kPluginFileExtensions); ++i) {
      const PluginID& plugin_id = kPluginFileExtensions[i];
      if (pagespeed::string_util::StringCaseEndsWith(
              url_no_query, plugin_id.id)) {
        *out_type = plugin_id.type;
        return;
      }
    }

    // If it's not a known plugin, maybe it should be allowed.
    *out_allowed = DetermineAllowedFromExtension(url_no_query);
  }

  // If that didn't work, give up.
  *out_type = AvoidPluginsDetails_PluginType_UNKNOWN;
}

// Determine the source URL of a Java plugin from attributes on the tag.
// Both the embed tag and applet tag share the same attributes.
bool PluginElementVisitor::DetermineJavaUrlFromAttributes(
    const pagespeed::DomElement& node, std::string* out_url) {
  out_url->clear();

  std::string code;
  std::string object;
  std::string archive;
  std::string codebase;

  node.GetAttributeByName("code", &code);
  node.GetAttributeByName("object", &object);
  node.GetAttributeByName("archive", &archive);
  node.GetAttributeByName("codebase", &codebase);

  return DetermineJavaUrl(code, object, archive, codebase, out_url);
}

// Attempts to determine a representative URL for an embedded Java tag from the
// four relevant attributes or params.
bool PluginElementVisitor::DetermineJavaUrl(const std::string& code,
                                            const std::string& object,
                                            const std::string& archive,
                                            const std::string& codebase,
                                            std::string* out_url) {
  out_url->clear();

  std::string src;

  if (!archive.empty()) {
    // The archive parameter is a comma separated list of resources. We'll take
    // the first one to report to the user.
    base::StringPiece archives_piece(archive);
    const size_t first_comma_index = archives_piece.find(",");
    if (first_comma_index != base::StringPiece::npos) {
      archives_piece.substr(0, first_comma_index).CopyToString(&src);
    } else {
      src = archive;
    }
  }

  // Exactly one of the code or object params must be present, so if we didn't
  // find an archive, use one of those.
  if (src.empty()) {
    src = code;
  }

  if (src.empty()) {
    src = object;
  }

  // If we successfully found a source, resolve it against the codebase.
  if (!src.empty()) {
    // The codebase is the base for the applet relative to the page. Here we
    // ensure the codebase is treated as a directory by adding a trailing slash.
    std::string base_url = document_->ResolveUri(codebase + "/");
    *out_url = pagespeed::uri_util::ResolveUri(src, base_url);
    return true;
  }
  return false;
}

}  // namespace

namespace pagespeed {

namespace rules {

AvoidPlugins::AvoidPlugins()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::RESPONSE_BODY)) {}

const char* AvoidPlugins::name() const {
  return "AvoidPlugins";
}

UserFacingString AvoidPlugins::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to avoid using
  // browser plugins like Flash or Silverlight on webpages.
  return _("Avoid plugins");
}

bool AvoidPlugins::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  const DomDocument* document = rule_input.pagespeed_input().dom_document();
  if (document) {
    PluginElementVisitor visitor(&rule_input, document, provider);
    document->Traverse(&visitor);
  }
  return true;
}

UrlBlockFormatter* AvoidPlugins::CreateUrlBlockFormatterForType(
    const AvoidPluginsDetails_PluginType& type, RuleFormatter* formatter) {
  for (size_t i = 0; i < arraysize(kPluginHumanNames); ++i) {
    if (type == kPluginHumanNames[i].type) {
      return formatter->AddUrlBlock(
          // TRANSLATOR: Header at the top of a list of browser plugins
          // of a specified type detected on a webpage. PLUGIN_TYPE will be
          // replaced with a name such as "Flash" or "Silverlight", which are
          // not translated.
          _("Find alternatives for the following %(PLUGIN_TYPE)s plugins."),
          VerbatimStringArgument("PLUGIN_TYPE", kPluginHumanNames[i].id));
    }
  }

  if (type != AvoidPluginsDetails_PluginType_UNKNOWN) {
    LOG(WARNING) << "Missing human readable string for type " << type;
  }

  return formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of browser plugins of unknown
      // type detected on a webpage. "Plugins" refers to additional software
      // that needs to be installed to use portions of some web pages, such as
      // Flash or Silverlight.
      _("Find alternatives for the following plugins."));
}

void AvoidPlugins::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
  if (results.empty()) {
    formatter->SetSummaryLine(
        // TRANSLATOR: A summary to give a general overview of this Page Speed
        // rule. "Plugins" refers to additional software that needs to be
        // installed to use portions of some web pages, such as Flash or
        // Silverlight. "Platforms" refer to devices like cell phones, and
        // browsers like Chrome or Internet Explorer.
        // The text between BEGIN_LINK and END_LINK will be displayed as a
        // clickable link in the browser, which takes the user to a document
        // providing additional information.
        _("Your page does not appear to use plugins, which would "
          "prevent content from being usable on many platforms. Learn more "
          "about the importance of %(BEGIN_LINK)savoiding "
          "plugins%(END_LINK)s."),
        HyperlinkArgument("LINK",
                          "https://developers.google.com/speed/docs/insights/"
                          "AvoidPlugins"));
    return;
  }

  formatter->SetSummaryLine(
      // TRANSLATOR: A summary to give a general overview of this Page Speed
      // rule. "Plugins" refers to additional software that needs to be
      // installed to use portions of some web pages, such as Flash or
      // Silverlight. "Platforms" refer to devices like cell phones, and
      // browsers like Chrome or Internet Explorer.
      // The text between BEGIN_LINK and END_LINK will be displayed as a
      // clickable link in the browser, which takes the user to a document
      // providing additional information.
      _("Your page uses plugins, which prevents portions of your "
        "page from being used on many platforms. %(BEGIN_LINK)sFind "
        "alternatives for plugin based content%(END_LINK)s to increase "
        "compatibility."),
      HyperlinkArgument("LINK",
                        "https://developers.google.com/speed/docs/insights/"
                        "AvoidPlugins"));

  // Sort the results into separate UrlBlocks based on the type of plugin.
  // While iterating through the results, this map holds the UrlBlockFormatter
  // that we instantiate for each type encountered.
  std::map<AvoidPluginsDetails_PluginType, UrlBlockFormatter*> url_blocks;

  for (ResultVector::const_iterator iter = results.begin();
       iter != results.end(); ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (details.HasExtension(AvoidPluginsDetails::message_set_extension)) {
      const AvoidPluginsDetails& plugin_details = details.GetExtension(
          AvoidPluginsDetails::message_set_extension);
      const AvoidPluginsDetails_PluginType& type = plugin_details.type();

      UrlBlockFormatter* url_block;
      std::map<AvoidPluginsDetails_PluginType, UrlBlockFormatter*>::iterator
          url_block_iter = url_blocks.find(type);

      if (url_block_iter == url_blocks.end()) {
        url_block = CreateUrlBlockFormatterForType(type, formatter);
        url_blocks.insert(std::make_pair(type, url_block));
      } else {
        url_block = url_block_iter->second;
      }

      if (result.resource_urls(0).empty()) {
        std::string type;
        if (plugin_details.has_mime()) {
          type = plugin_details.mime();
        } else {
          type = plugin_details.classid();
        }

        url_block->AddUrlResult(
            // TRANSLATOR: Entry of a list of plugins detected on a webpage.
            // TYPE will be replaced with an identifier like "application/java"
            // or "clsid:8AD9C840-044E-11D1-B3E9-00805F499D93"
            _("Unknown plugin of type %(TYPE)s."),
            VerbatimStringArgument("TYPE", type));
        continue;
      }

      if (plugin_details.has_x() &&
          plugin_details.has_y() &&
          plugin_details.has_width() &&
          plugin_details.has_height()) {
        if (type == AvoidPluginsDetails_PluginType_UNKNOWN &&
            plugin_details.has_mime()) {
          url_block->AddUrlResult(
              not_localized(
                  "%(URL)s (%(MIME)s: %(WIDTH)s x %(HEIGHT)s) %(SCREENSHOT)s."),
              UrlArgument("URL", result.resource_urls(0)),
              VerbatimStringArgument("MIME", plugin_details.mime()),
              IntArgument("WIDTH", plugin_details.width()),
              IntArgument("HEIGHT", plugin_details.height()),
              FinalRectArgument("SCREENSHOT",
                                plugin_details.x(),
                                plugin_details.y(),
                                plugin_details.width(),
                                plugin_details.height()));
        } else {
          url_block->AddUrlResult(
              not_localized("%(URL)s (%(WIDTH)s x %(HEIGHT)s) %(SCREENSHOT)s."),
              UrlArgument("URL", result.resource_urls(0)),
              IntArgument("WIDTH", plugin_details.width()),
              IntArgument("HEIGHT", plugin_details.height()),
              FinalRectArgument("SCREENSHOT",
                                plugin_details.x(),
                                plugin_details.y(),
                                plugin_details.width(),
                                plugin_details.height()));
        }
      } else {
        if (type == AvoidPluginsDetails_PluginType_UNKNOWN &&
            plugin_details.has_mime()) {
          url_block->AddUrlResult(
              not_localized("%(URL)s (%(MIME)s)."),
              UrlArgument("URL", result.resource_urls(0)),
              VerbatimStringArgument("MIME", plugin_details.mime()));
        } else {
          url_block->AddUrl(result.resource_urls(0));
        }
      }
    }
  }
}

double AvoidPlugins::ComputeResultImpact(const InputInformation& input_info,
                                         const Result& result) {
  const ResultDetails& details = result.details();
  if (details.HasExtension(AvoidPluginsDetails::message_set_extension)) {
    const AvoidPluginsDetails& plugin_details = details.GetExtension(
        AvoidPluginsDetails::message_set_extension);
    if (plugin_details.has_atf_ratio()) {
      return kPluginBaseImpact +
          (kPluginAtfImpactMultiplier * plugin_details.atf_ratio());
    }
  }

  return kPluginBaseImpact;
}

bool AvoidPlugins::IsExperimental() const {
  return true;
}

void AvoidPlugins::AppendRuleGroups(
    std::vector<FormattedRuleResults::RuleGroup>* out) const {
  out->push_back(FormattedRuleResults::USABILITY);
}

}  // namespace rules

}  // namespace pagespeed
