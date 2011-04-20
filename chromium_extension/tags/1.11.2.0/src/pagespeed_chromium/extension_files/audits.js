// Copyright 2010 Google Inc. All Rights Reserved.
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

// NOTE: There is currently much duplicated code between this file and
//       pagespeed.js.  The expectation is that we will completely remove one
//       or the other at some point, so we're not bothering to factor out the
//       common code in the meantime.

"use strict";

// Wrap a function with an error handler.  Given a function, return a new
// function that behaves the same but catches and logs errors thrown by the
// wrapped function.
function withErrorHandler(func) {
  return function (/*arguments*/) {
    try {
      return func.apply(this, arguments);
    } catch (e) {
      var message = 'Error in Page Speed audits:\n' + e.stack;
      alert(message + '\n\nPlease file a bug at\n' +
            'http://code.google.com/p/page-speed/issues/');
      webInspector.log(message);
    }
  };
}

// Compare the two arguments, as for a sort function.  Useful for building up
// larger comparators.
function compare(a, b) {
  return a < b ? -1 : a > b ? 1 : 0;
}

// Get a version of a URL that is suitable for display in the UI
// (~100 characters or fewer).
function getDisplayUrl(fullUrl) {
  var kMaxLinkTextLen = 100;
  return (fullUrl.length > kMaxLinkTextLen ?
          fullUrl.substring(0, kMaxLinkTextLen) + '...' :
          fullUrl);
}

// Build a string from a format array (as returned from the Page Speed module).
function formatRuleTitle(formatArray) {
  var elements = [];
  formatArray.forEach(function (item) {
    if (item.type === 'url') {
      elements.push(item.alt || getDisplayUrl(item.value));
    } else if (item.type === 'str') {
      elements.push(item.value);
    }
  });
  return elements.join('');
}

// Given format children (as returned from the Page Speed module), append
// children recursively to the parent audit details node.
function formatChildren(children, parent_node, audit) {
  (children || []).forEach(function (child) {
    var elements = [];
    child.format.forEach(function (item) {
      if (item.type === 'url') {
        elements.push(audit.url(item.value, item.alt ||
                                getDisplayUrl(item.value)));
      } else if (item.type === 'str') {
        elements.push(item.value);
      }
    });
    var child_node = parent_node.addChild(elements);
    child_node.expanded = true;
    formatChildren(child.children, child_node, audit);
  });
}

// Build and add the audit results, and then declare the audit to be done.
function showResults(response, audit) {
  // Sort the results, first by score, then by name.
  var results = response.results.slice();
  results.sort(function (result1, result2) {
    return (compare(result1.score, result2.score) ||
            compare(result1.name, result2.name));
  });

  results.forEach(function (result) {
    // TODO(mdsteele): I don't yet fully understand this API (since it's still
    //   experimental and therefore not thouroughly documented).  The empty
    //   strings we pass in below for audit.createResult() and
    //   audit.addResult() give us mostly what we want, but there might be a
    //   cleaner way to format these the way we want.
    var details = audit.createResult("");
    details.expanded = true;
    formatChildren(result.children, details, audit);
    var rule_severity = (result.score > 80 ? audit.Severity.Info :
                         result.score > 60 ? audit.Severity.Warning :
                         audit.Severity.Severe);
    audit.addResult(formatRuleTitle(result.format), "",
                    rule_severity, details);
  });

  audit.done();
}

function onStartAuditCategory(audit) {
  webInspector.resources.getAll(withErrorHandler(
    function (resources) {
      var ids = resources.map(function (resource) { return resource.id; });
      webInspector.resources.getContent(ids, withErrorHandler(
        function (bodies) {
          // Get the resource bodies.
          bodyMap = {};
          bodies.forEach(function (body) {
            if (body.isError) {
              webInspector.log(
                "Page Speed failed to get resource content: " +
                JSON.stringify(body.details));
            } else {
              bodyMap[body.id] = body;
            }
          });

          // Collect the HAR data for the inspected page.
          var entries = resources.map(function (resource) {
            var entry = resource.har;
            var body = bodyMap[resource.id];
            if (body) {
              var content = entry.response.content;
              content.text = body.content;
              content.encoding = body.encoding;
            }
            return entry;
          });
          var har = {log: {entries: entries}};

          // Prepare the request.
          var analyze = 'all';  // TODO(mdsteele): Support choosing a filter.
          var input = {analyze: analyze, har: har};
          var tab_id = webInspector.inspectedWindow.tabId;
          var request = {kind: 'runPageSpeed', input: input, tab_id: tab_id};

          // Tell the background page to run the Page Speed rules.
          chrome.extension.sendRequest(request, withErrorHandler(
            function (response) {
              if (!response) {
                throw new Error("No response to runPageSpeed request.");
              } else if (response.error_message) {
                webInspector.log(response.error_message);
                audit.done();
              } else {
                showResults(response, audit);
              }
            }
          ));
        }
      ));
    }
  ));
}

// Create and register a new audits category.
// The second argument of webInspector.audits.addCategory() is the maximum
// number of rules in that category.  The category is automatically counted as
// "done" as soon as that many results arrive, or you can declare it done
// manually by calling audit.done().  For us, it's easiest to just set the
// limit to (effectively) infinity, and call audit.done() when we're done.
var pagespeed_audits_category =
  webInspector.audits.addCategory("Page Speed", 999999);
pagespeed_audits_category.onAuditStarted.addListener(onStartAuditCategory);
