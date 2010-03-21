// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/css_sprite_filter.h"

#include <assert.h>
#include <string>
#include "net/instaweb/rewriter/public/add_head_filter.h"
#include "net/instaweb/rewriter/public/resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/rewriter/public/sprite_resource.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {
// TODO(jmarantz): This spriting code assumes we are synchronously loading
// the css files from the file system.  We need to support asynchronously
// loading from an external site.
//
// TODO(jmarantz) We exhibit zero intelligence about which sprites to
// combine; we sprite whatever is possible.  This can reduce performance
// by combining highly cacheable shared resources with transient ones.
//
// TODO(jmarantz): We do not recognize IE directives as spriting boundaries.
// We should supply a meaningful IEDirective method as a boundary.
//
// TODO(jmarantz): allow spriting of CSS elements found in the body, whether
// or not the head has already been flushed.

CssSpriteFilter::CssSpriteFilter(
    HtmlParse* html_parse,
    ResourceManager* resource_manager)
    : html_parse_(html_parse),
      resource_manager_(resource_manager) {
  s_head_ = html_parse->Intern("head");
  s_link_ = html_parse->Intern("link");
  s_href_ = html_parse->Intern("href");
  s_type_ = html_parse->Intern("type");
  s_text_css_ = html_parse->Intern("text/css");
  s_rel_  = html_parse->Intern("rel");
  s_stylesheet_  = html_parse->Intern("stylesheet");
  s_media_  = html_parse->Intern("media");
  head_element_ = NULL;
}

// Examines an HTML Link element to determine whether it's a CSS reference.
// If so, extract the name and return it, and write the 'media' parameter
// value to std::string* media, if that parameters exists.  If this element
// is not a valid CSS reference, then NULL is returned.
const char* CssSpriteFilter::FindCssHref(
    const HtmlElement* element, std::string* media) {
  const char* href = NULL;

  // We must have all attributes rel='stylesheet' href='name.css', and
  // type='text/css', although they can be in any order.  There should
  // be no other attributes, as they would be lost in the sprite.
  int num_attrs = element->attribute_size();
  if ((num_attrs == 3) || (num_attrs == 4)) {   // 'media=' is optional
    for (int i = 0; i < num_attrs; ++i) {
      const HtmlElement::Attribute& attr =
          element->attribute(i);
      if (attr.name_ == s_href_) {
        href = attr.value_;
      } else if (attr.name_ == s_media_) {
        *media = attr.value_;
      } else if (!(((attr.name_ == s_rel_) && (attr.value_ == s_stylesheet_)) ||
                   ((attr.name_ == s_type_) && (attr.value_ == s_text_css_)))) {
        href = NULL;  // unrecognized attribute
        break;
      }
    }
  }
  return href;
}

void CssSpriteFilter::StartDocument() {
  head_element_ = NULL;
}

void CssSpriteFilter::StartElement(HtmlElement* element) {
  element_stack_.push_back(element);
  if (element->tag() == s_head_) {
    head_element_ = element;
  }
}

void CssSpriteFilter::EndElement(HtmlElement* element) {
  if (element->tag() == s_link_) {
    std::string media;
    if (FindCssHref(element, &media) != NULL) {
      css_elements_.push_back(element);
    }
  } else if (element->tag() == s_head_) {
    EmitSprites();
  }
  assert(element == element_stack_.back());
  element_stack_.pop_back();
}

// An IE directive that includes any stylesheet info should be a barrier
// for css spriting.  It's OK to emit the spriting we've seen so far.
void CssSpriteFilter::IEDirective(const std::string& directive) {
  // TODO(jmarantz): consider recursively invoking the parser and
  // parsing all the IE-specific code properly.
  if (directive.find("stylesheet") != std::string::npos) {
  }
}

void CssSpriteFilter::Flush() {
  // TODO(jmarantz): Ideally, all the css links will be encountered in the
  // <head>, before the first flush.  It's possible we'll get a Flush,
  // during the <head> parse, and there may be some css files before it,
  // and some afterward.  And there may be css links encountered in the body,
  // and there may have Flushed our head css sprite first.  So all of that
  // will have to be dealt with by calling EmitSprites, after finding the
  // appropriate place in the DOM to insert the sprite.
  //
  // The best performance will come when the entire document is parsed
  // without a Flush, in which case we can move all the css links into
  // the <head>, but even that is not yet implemented.
  css_elements_.clear();
}

void CssSpriteFilter::EmitSprites() {
  MessageHandler* message_handler = html_parse_->message_handler();

  // It's possible that we'll have found 2 css files to sprite, but one
  // of them became non-rewritable due to a flush, and thus we'll wind
  // up spriting just one file, so do a first pass counting rewritable
  // css linkes.  Also, load the CSS content in this pass.  We will only
  // do a sprite if we have more than one css element that successfully
  // loaded.
  std::vector<HtmlElement*> sprite_elements;
  std::vector<Resource*> sprite_resources;
  for (int i = 0, n = css_elements_.size(); i < n; ++i) {
    HtmlElement* element = css_elements_[i];
    std::string media;
    const char* href = FindCssHref(element, &media);
    if ((href != NULL) && html_parse_->IsRewritable(element)) {
      // TODO(jmarantz): consider async loads; exclude css file
      // from the sprite that are not yet loaded.  For now, our
      // loads are blocking.  Need to understand Apache module
      Resource* css_resource = resource_manager_->CreateResource(href);
      if (resource_manager_->Load(css_resource, message_handler)) {
        if (!media.empty()) {
          // TODO(jmarantz): Annotate sprite sections with 'media' as needed
          // css_resource->AddAttribute("media", media.c_str());
        }
        sprite_resources.push_back(css_resource);

        // Try to add this resource to the sprite.  We are not yet
        // committed to the sprite because we haven't written the
        // contents to disk yet, so don't mutate the DOM but keep
        // track of which elements will be involved
        sprite_elements.push_back(element);
      }
    }
  }

  if (sprite_elements.size() > 1) {
    // Ideally like to have a data-driven service tell us which elements should
    // be sprited together.  Note that both the resources and the elements
    // are managed, so we don't delete them even if the spriting fails.
    SpriteResource* sprite = resource_manager_->CreateSprite(".css");
    HtmlElement* sprite_element = html_parse_->NewElement(s_link_);
    sprite_element->AddAttribute(s_rel_, s_stylesheet_, "\"");
    sprite_element->AddAttribute(s_type_, s_text_css_, "\"");

    // Start building up the sprite.  At this point we are still
    // not committed to the sprite, because the 'write' can fail.
    //
    // TODO(jmarantz): In a scalable installation where the sprites
    // must be kept in a database, we cannot serve HTML that references
    // sprite resources that have not been committed yet, and
    // committing to a database may take too long to block on the HTML
    // rewrite.  So we will want to refactor this to check to see
    // whether the desired sprite is already known.  For now we'll
    // assume we can commit to serving the sprite during the HTML
    // rewriter.
    for (int i = 0, n = sprite_resources.size(); i < n; ++i) {
      sprite->AddResource(sprite_resources[i]);
    }

    // We've collected at least two CSS files to sprite, and whose
    // HTML elements are in the current flush window.  Last step
    // is to write the sprite.
    if (resource_manager_->WriteResource(sprite, sprite->filename().c_str(),
                                         message_handler)) {
      // commit by removing the elements from the DOM.
      for (size_t i = 0; i < sprite_elements.size(); ++i) {
        html_parse_->DeleteElement(sprite_elements[i]);
      }
      const char* href = html_parse_->Intern(sprite->url());
      sprite_element->AddAttribute(s_href_, href, "\"");
      html_parse_->InsertElementBeforeCurrent(sprite_element);
    }
  }
  css_elements_.clear();
}
}
