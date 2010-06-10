// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#include "public/html_node.h"

#include "net/instaweb/htmlparse/html_event.h"

namespace net_instaweb {

HtmlNode::~HtmlNode() {}

HtmlLeafNode::~HtmlLeafNode() {}

HtmlCdataNode::~HtmlCdataNode() {}

void HtmlCdataNode::SynthesizeEvents(const HtmlEventListIterator& iter,
                                     HtmlEventList* queue) {
  // We use -1 as a bogus line number, since the event is synthetic.
  HtmlCdataEvent* event = new HtmlCdataEvent(this, -1);
  set_iter(queue->insert(iter, event));
}

HtmlCharactersNode::~HtmlCharactersNode() {}

void HtmlCharactersNode::SynthesizeEvents(const HtmlEventListIterator& iter,
                                          HtmlEventList* queue) {
  // We use -1 as a bogus line number, since the event is synthetic.
  HtmlCharactersEvent* event = new HtmlCharactersEvent(this, -1);
  set_iter(queue->insert(iter, event));
}

HtmlCommentNode::~HtmlCommentNode() {}

void HtmlCommentNode::SynthesizeEvents(const HtmlEventListIterator& iter,
                                       HtmlEventList* queue) {
  // We use -1 as a bogus line number, since the event is synthetic.
  HtmlCommentEvent* event = new HtmlCommentEvent(this, -1);
  set_iter(queue->insert(iter, event));
}

HtmlDirectiveNode::~HtmlDirectiveNode() {}

void HtmlDirectiveNode::SynthesizeEvents(const HtmlEventListIterator& iter,
                                         HtmlEventList* queue) {
  // We use -1 as a bogus line number, since the event is synthetic.
  HtmlDirectiveEvent* event = new HtmlDirectiveEvent(this, -1);
  set_iter(queue->insert(iter, event));
}

}  // namespace net_instaweb
