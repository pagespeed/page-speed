// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

// html_filter that passes data through unmodified, but
// logs statistics about the data as it goes by.
// It should be possible to create many instances of this
//     class and insert them at different points in the rewriting flow
// Goal is to log:
//   NUM_EXPLICIT_CLOSED - <tag> </tag> pairs
//   NUM_IMPLICIT_CLOSED - <tag> for implicitly-closed tag
//   NUM_BRIEF_CLOSED    - </tag>
//   NUM_CLOSED          - Sum of above three
//   NUM_UNCLOSED        - <tag> without matching </tag>
//   NUM_SPURIOUS_CLOSED - </tag> without preceding <tag>; UNCOUNTED RIGHT NOW!
//   NUM_TAGS            - Total number of opening tags
//   NUM_CDATA           - cdata sections
//   NUM_COMMENTS        - comments
//   NUM_DIRECTIVES      - directives
//   NUM_DOCUMENTS       - started documents
//   NUM_IE_DIRECTIVES   - ie directives
// Reporting:
//     We report this information via a StatisticsLog: filter.ToString(log)
//     Two sets of statistics (eg before and after processing) can be
//         compared using before.Equals(after),

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_LOGGING_HTML_FILTER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_LOGGING_HTML_FILTER_H_

#include <string>

#include "net/instaweb/htmlparse/public/empty_html_filter.h"

namespace net_instaweb {

class StatisticsLog;

class LoggingFilter : public EmptyHtmlFilter {
 public:
  // internal names of statistics.
  // NOTE: must match string names in kStatisticNames at top of
  // logging_html_filter.c
  enum Statistic {
    MIN_STAT = 0,
    NUM_EXPLICIT_CLOSED = 0,
    NUM_IMPLICIT_CLOSED,
    NUM_BRIEF_CLOSED,
    NUM_CLOSED,
    NUM_UNCLOSED,
    NUM_SPURIOUS_CLOSED,
    NUM_TAGS,
    NUM_CDATA,
    NUM_COMMENTS,
    NUM_DIRECTIVES,
    NUM_DOCUMENTS,
    NUM_IE_DIRECTIVES,
    MAX_STAT
  };

  LoggingFilter();

  // HtmlFilter methods.
  virtual void StartDocument();
  virtual void EndDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Cdata(const std::string& cdata);
  virtual void Comment(const std::string& comment);
  virtual void IEDirective(const std::string& directive);
  virtual void Characters(const std::string& characters);
  virtual void IgnorableWhitespace(const std::string& whitespace);
  virtual void Directive(const std::string& directive);
  virtual void Flush();

  // Getter for individual statistics; NO BOUNDS CHECKS.
  inline int get(const Statistic statistic) const {
    return stats_[statistic];
  }

  // Logging, diffing, and aggregation

  // Report all statistics
  void LogStatistics(StatisticsLog *statistics_log) const;

  // Return true if all statistics in this are equal to those in that.
  bool Equals(const LoggingFilter &that) const;

  // Report all statistics in this and that which differ
  void LogDifferences(
      const LoggingFilter &that, StatisticsLog *statistics_log) const;

  // Add all statistics in that into this.
  void Aggregate(const LoggingFilter &that);

  // Aggregate differences between two sets of statistics into this.
  //     this.get(stat) += first.get(stat) - second.get(stat)
  void AggregateDifferences(const LoggingFilter &first,
                            const LoggingFilter &second);

  void Reset();

 private:
  int stats_[MAX_STAT];
};
}
#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_LOGGING_HTML_FILTER_H_
