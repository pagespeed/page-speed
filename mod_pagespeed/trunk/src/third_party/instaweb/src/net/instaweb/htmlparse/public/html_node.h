// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_NODE_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_NODE_H_

#include "base/basictypes.h"
#include "net/instaweb/htmlparse/public/html_parser_types.h"
#include <string>

namespace net_instaweb {

// Base class for HtmlElement and HtmlLeafNode
class HtmlNode {
 public:
  virtual ~HtmlNode();
  friend class HtmlParse;

 protected:
  HtmlNode() {}

  // Create new event object(s) representing this node, and insert them into
  // the queue just before the given iterator; also, update this node object as
  // necessary so that begin() and end() will return iterators pointing to
  // the new event(s).  The line number for each event should probably be -1.
  virtual void SynthesizeEvents(const HtmlEventListIterator& iter,
                                HtmlEventList* queue) = 0;

  // Return an iterator pointing to the first event associated with this node.
  virtual HtmlEventListIterator begin() const = 0;
  // Return an iterator pointing to the last event associated with this node.
  virtual HtmlEventListIterator end() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(HtmlNode);
};

// Base class for leaf nodes (like HtmlCharactersNode and HtmlCommentNode)
class HtmlLeafNode : public HtmlNode {
 public:
  virtual ~HtmlLeafNode();
  friend class HtmlParse;

 protected:
  explicit HtmlLeafNode(const HtmlEventListIterator& iter) : iter_(iter) {}
  virtual HtmlEventListIterator begin() const { return iter_; }
  virtual HtmlEventListIterator end() const { return iter_; }
  void set_iter(const HtmlEventListIterator& iter) { iter_ = iter; }

 private:
  HtmlEventListIterator iter_;
  DISALLOW_COPY_AND_ASSIGN(HtmlLeafNode);
};

// Leaf node representing a CDATA section
class HtmlCdataNode : public HtmlLeafNode {
 public:
  virtual ~HtmlCdataNode();
  const std::string& contents() { return contents_; }
  friend class HtmlParse;

 protected:
  virtual void SynthesizeEvents(const HtmlEventListIterator& iter,
                                HtmlEventList* queue);

 private:
  HtmlCdataNode(const std::string& contents,
                const HtmlEventListIterator& iter)
      : HtmlLeafNode(iter), contents_(contents) {}
  const std::string contents_;
  DISALLOW_COPY_AND_ASSIGN(HtmlCdataNode);
};

// Leaf node representing raw characters in HTML
class HtmlCharactersNode : public HtmlLeafNode {
 public:
  virtual ~HtmlCharactersNode();
  const std::string& contents() { return contents_; }
  friend class HtmlParse;

 protected:
  virtual void SynthesizeEvents(const HtmlEventListIterator& iter,
                                HtmlEventList* queue);

 private:
  HtmlCharactersNode(const std::string& contents,
                     const HtmlEventListIterator& iter)
      : HtmlLeafNode(iter), contents_(contents) {}
  const std::string contents_;
  DISALLOW_COPY_AND_ASSIGN(HtmlCharactersNode);
};

// Leaf node representing an HTML comment
class HtmlCommentNode : public HtmlLeafNode {
 public:
  virtual ~HtmlCommentNode();
  const std::string& contents() { return contents_; }
  friend class HtmlParse;

 protected:
  virtual void SynthesizeEvents(const HtmlEventListIterator& iter,
                                HtmlEventList* queue);

 private:
  HtmlCommentNode(const std::string& contents,
                  const HtmlEventListIterator& iter)
      : HtmlLeafNode(iter), contents_(contents) {}
  const std::string contents_;
  DISALLOW_COPY_AND_ASSIGN(HtmlCommentNode);
};

// Leaf node representing an HTML directive
class HtmlDirectiveNode : public HtmlLeafNode {
 public:
  virtual ~HtmlDirectiveNode();
  const std::string& contents() { return contents_; }
  friend class HtmlParse;

 protected:
  virtual void SynthesizeEvents(const HtmlEventListIterator& iter,
                                HtmlEventList* queue);

 private:
  HtmlDirectiveNode(const std::string& contents,
                    const HtmlEventListIterator& iter)
      : HtmlLeafNode(iter), contents_(contents) {}
  const std::string contents_;
  DISALLOW_COPY_AND_ASSIGN(HtmlDirectiveNode);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_NODE_H_
