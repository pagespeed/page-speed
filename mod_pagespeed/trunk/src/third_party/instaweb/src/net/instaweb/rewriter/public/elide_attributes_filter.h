// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_ELIDE_ATTRIBUTES_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_ELIDE_ATTRIBUTES_FILTER_H_

#include <map>

#include "base/basictypes.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

// Remove attributes and attribute values that can be safely elided.
class ElideAttributesFilter : public EmptyHtmlFilter {
 public:
  explicit ElideAttributesFilter(HtmlParse* html_parse);

  virtual void StartDocument();
  virtual void Directive(HtmlDirectiveNode* directive);
  virtual void StartElement(HtmlElement* element);

 private:
  typedef std::map<Atom, const char*, AtomCompare> AtomMap;
  typedef std::map<Atom, AtomSet, AtomCompare> AtomSetMap;
  typedef std::map<Atom, AtomMap, AtomCompare> AtomMapMap;

  bool xhtml_mode_;  // is this an XHTML document?

  AtomSetMap one_value_attrs_map_;  // tag/attrs with only one possible value
  AtomMapMap default_value_map_;  // tag/attrs with default values

  DISALLOW_COPY_AND_ASSIGN(ElideAttributesFilter);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_ELIDE_ATTRIBUTES_FILTER_H_
