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

"use strict";

var pagespeed = {

  // We track the current results in this global variable.
  currentResults: null,

  // The currently active ResourceAccumulator, if any.
  resourceAccumulator: null,

  // Throw an error (with an optional message) if the condition is false.
  assert: function (condition, opt_message) {
    if (!condition) {
      throw new Error('Assertion failed:' + (opt_message || '(no message)'));
    }
  },

  // Wrap a function with an error handler.  Given a function, return a new
  // function that behaves the same but catches and logs errors thrown by the
  // wrapped function.
  withErrorHandler: function (func) {
    pagespeed.assert(typeof(func) === 'function',
                     'withErrorHandler: func must be a function');
    return function (/*arguments*/) {
      try {
        return func.apply(this, arguments);
      } catch (e) {
        var message = 'Error in Page Speed panel\n: ' + e.stack;
        alert(message + '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
        webInspector.log(message);
        document.getElementById('run-button').disabled = false;
        document.getElementById('spinner-img').style.display = 'none';
      }
    };
  },

  // Compare the two arguments, as for a sort function.  Useful for building up
  // larger comparators.
  compare: function (a, b) {
    return a < b ? -1 : a > b ? 1 : 0;
  },

  // Remove all children from the given DOM node.
  removeAllChildren: function (domNode) {
    while (domNode.lastChild) {
      domNode.removeChild(domNode.lastChild);
    }
  },

  // Make a new DOM node.
  // tagName -- the tag name for the node
  // opt_className -- an optional class name for the node; either a string,
  //     or null/omitted for no class name
  // opt_contents -- optional contents for the node; this may be a string, a
  //     DOM node, or an array of strings, DOM nodes, and/or other arrays
  //     (which will be flattened)
  makeElement: function (tagName, opt_className, opt_contents) {
    var elem = document.createElement(tagName);
    if (opt_className) {
      elem.className = opt_className;
    }
    var addChildren = function (children) {
      if (children) {
        if (typeof(children) === 'object' && children.forEach) {
          children.forEach(addChildren);
        } else {
          elem.appendChild(typeof(children) === 'string' ?
                           document.createTextNode(children) : children);
        }
      }
    };
    addChildren(opt_contents);
    return elem;
  },

  // Make a new DOM node for a button.
  // label -- the text label for the button
  // action -- a thunk to be called when the button is pressed
  makeButton: function (label, action) {
    var button = pagespeed.makeElement('button', null, label);
    button.addEventListener('click', pagespeed.withErrorHandler(action),
                            false);
    return button;
  },

  // Make a new DOM node for a link.
  // href -- the URL to link to
  // label -- the visible text for the link
  makeLink: function (href, label) {
    var link = pagespeed.makeElement('a', null, label);
    link.href = 'javascript:null';
    var openUrl = function () {
      // Tell the background page to open the url in a new tab.
      chrome.extension.sendRequest({kind: 'openUrl', url: href});
    };
    link.addEventListener('click', pagespeed.withErrorHandler(openUrl), false);
    return link;
  },

  // Given a score, return a DOM node for a red/yellow/green icon.
  makeScoreIcon: function (score) {
    pagespeed.assert(typeof(score) === 'number',
                     'makeScoreIcon: score must be a number');
    pagespeed.assert(isFinite(score), 'makeScoreIcon: score must be finite');
    var icon = pagespeed.makeElement('div', (score > 80 ? 'icon-okay' :
                                             score > 60 ? 'icon-warn' :
                                             'icon-error'));
    icon.setAttribute('title', 'Score: ' + score + '/100');
    return icon;
  },

  // Set the text of the "Run Page Speed" button (e.g. to "Refresh Results").
  setRunButtonText: function (text) {
    var run_button = document.getElementById('run-button');
    pagespeed.removeAllChildren(run_button);
    run_button.appendChild(document.createTextNode(text));
  },

  // Get a version of a URL that is suitable for display in the UI
  // (~100 characters or fewer).
  getDisplayUrl: function (fullUrl) {
    pagespeed.assert(typeof(fullUrl) === 'string',
                     'getDisplayUrl: fullUrl must be a string');
    var kMaxLinkTextLen = 100;
    return (fullUrl.length > kMaxLinkTextLen ?
            fullUrl.substring(0, kMaxLinkTextLen) + '...' :
            fullUrl);
  },

  // Given a format array (as returned from the Page Speed module), build an
  // array of DOM nodes, suitable to be passed to makeElement().
  formatLine: function (formatArray) {
    var elements = [];
    formatArray.forEach(function (item) {
      if (item.type === 'url') {
        elements.push(pagespeed.makeLink(item.value, item.alt ||
                                         pagespeed.getDisplayUrl(item.value)));
      } else if (item.type === 'str') {
        elements.push(document.createTextNode(item.value));
      }
    });
    return elements;
  },

  // Given format children (as returned from the Page Speed module), build an
  // array of DOM nodes, suitable to be passed to makeElement().
  formatChildren: function (children, opt_grand) {
    var elements = [];
    (children || []).forEach(function (child) {
      elements.push(pagespeed.makeElement(opt_grand ? 'li' : 'p', null,
                                          pagespeed.formatLine(child.format)));
      if (child.children) {
        elements.push(pagespeed.makeElement('ul', null,
          pagespeed.formatChildren(child.children, true)));
      }
    });
    return elements;
  },

  // Expand all the results in the rules list.
  expandAllResults: function () {
    var rules_container = document.getElementById('rules-container');
    if (rules_container) {
      var result_divs = rules_container.childNodes;
      for (var index = 0; index < result_divs.length; ++index) {
        result_divs[index].lastChild.style.display = 'block';
      }
    }
  },

  // Collapse all the results in the rules list.
  collapseAllResults: function () {
    var rules_container = document.getElementById('rules-container');
    if (rules_container) {
      var result_divs = rules_container.childNodes;
      for (var index = 0; index < result_divs.length; ++index) {
        result_divs[index].lastChild.style.display = 'none';
      }
    }
  },

  // Toggle whether a given result DIV is expanded or collapsed.
  toggleResult: function (result_div) {
    if (result_div.lastChild.style.display !== 'block') {
      result_div.lastChild.style.display = 'block';
    } else {
      result_div.lastChild.style.display = 'none';
    }
  },

  // Clear and hide the results page, and make the welcome page visible again.
  clearResults: function () {
    pagespeed.currentResults = null;
    var results_container = document.getElementById('results-container');
    results_container.style.display = 'none';
    pagespeed.removeAllChildren(results_container);
    var welcome_container = document.getElementById('welcome-container');
    welcome_container.style.display = 'block';
    pagespeed.setRunButtonText('Run Page Speed');
  },

  // Format and display the current results.
  showResults: function () {
    pagespeed.assert(pagespeed.currentResults !== null,
                     "showResults: pagespeed.currentResults must not be null");

    // Remove the previous results.
    var results_container = document.getElementById('results-container');
    pagespeed.removeAllChildren(results_container);

    // Sort the results, first by score, then by name.
    var results = pagespeed.currentResults.results.slice();
    results.sort(function (result1, result2) {
      return (pagespeed.compare(result1.score, result2.score) ||
              pagespeed.compare(result1.name, result2.name));
    });

    // Compute the overall score.
    var overall_score = 0;
    var num_rules = 0;
    results.forEach(function (result) {
      num_rules += 1;
      overall_score += result.score;
    });
    overall_score = (num_rules === 0 ? 100 :
                     Math.round(overall_score / num_rules));

    // Create the score bar.
    var analyze = pagespeed.currentResults.analyze;
    results_container.appendChild(pagespeed.makeElement('div', 'score-bar', [
      pagespeed.makeElement('div', null, 'Page Speed Score' +
                            (analyze === 'ads' ? ' (ads only)' :
                             analyze === 'trackers' ? ' (trackers only)' :
                             analyze === 'content' ? ' (content only)' : '') +
                            ': ' + overall_score + '/100'),
      pagespeed.makeScoreIcon(overall_score),
      pagespeed.makeButton('Clear Results', pagespeed.clearResults),
      pagespeed.makeButton('Collapse All', pagespeed.collapseAllResults),
      pagespeed.makeButton('Expand All', pagespeed.expandAllResults)
    ]));

    // Create the rule results.
    var rules_container = pagespeed.makeElement('div');
    rules_container.id = 'rules-container';
    results.forEach(function (result) {
      var header = pagespeed.makeElement('div', 'header', [
        pagespeed.makeScoreIcon(result.score),
        pagespeed.formatLine(result.format)
      ]);
      if (result.score >= 100) {
        header.style.fontWeight = 'normal';
      }
      var formatted_children = pagespeed.formatChildren(result.children);
      var result_div = pagespeed.makeElement('div', 'result', [
        header,
        pagespeed.makeElement('div', 'details', [
          (formatted_children.length > 0 ? formatted_children :
           pagespeed.makeElement('p', null,
                                 'There were no violations of this rule.')),
          pagespeed.makeElement('p', null,
            pagespeed.makeLink('http://code.google.com/speed/page-speed/docs/' +
                               result.url, 'More information'))
        ])
      ]);
      rules_container.appendChild(result_div);
      header.addEventListener('mouseover', function () {
        header.style.backgroundColor = '#ddd';
      }, false);
      header.addEventListener('mouseout', function () {
        header.style.backgroundColor = '#eee';
      }, false);
      header.addEventListener('click', function () {
        pagespeed.toggleResult(result_div);
      }, false);
    });
    results_container.appendChild(rules_container);

    // Display the results.
    var welcome_container = document.getElementById('welcome-container');
    welcome_container.style.display = 'none';
    results_container.style.display = 'block';
    pagespeed.setRunButtonText('Refresh Results');
  },

  // Run Page Speed and display the results. This is done
  // asynchronously using the ResourceAccumulator.
  runPageSpeed: function () {
    document.getElementById('run-button').disabled = true;
    document.getElementById('spinner-img').style.display = 'inline';
    if (pagespeed.resourceAccumulator) {
      pagespeed.resourceAccumulator.cancel();
    }
    pagespeed.resourceAccumulator = new pagespeed.ResourceAccumulator(
      pagespeed.withErrorHandler(pagespeed.onResourceAccumulatorComplete));
    pagespeed.resourceAccumulator.start();
  },

  // Invoked when the ResourceAccumulator has finished collecting data
  // from the web inspector.
  onResourceAccumulatorComplete: function(har) {
    pagespeed.resourceAccumulator = null;

    // Prepare the request.
    var analyze = document.getElementById('analyze-dropdown').value;
    var input = {analyze: analyze, har: har};
    var tab_id = webInspector.inspectedWindow.tabId;
    var request = {kind: 'runPageSpeed', input: input, tab_id: tab_id};

    // Tell the background page to run the Page Speed rules.
    chrome.extension.sendRequest(
        request, pagespeed.withErrorHandler(pagespeed.onPageSpeedResults));
  },

  // Invoked in response to our background page running Page Speed on
  // the current page.
  onPageSpeedResults: function(response) {
    if (!response) {
      throw new Error("No response to runPageSpeed request.");
    } else if (response.error_message) {
      webInspector.log(response.error_message);
    } else {
      pagespeed.currentResults = response;
      pagespeed.showResults();
    }
    document.getElementById('run-button').disabled = false;
    document.getElementById('spinner-img').style.display = 'none';
  },

  // Callback to be called when the user changes the value of the "Analyze"
  // menu.
  onAnalyzeDropdownChange: function () {
    if (pagespeed.currentResults !== null &&
        document.getElementById('analyze-dropdown').value !==
        pagespeed.currentResults.analyze) {
      pagespeed.runPageSpeed();
    }
  }

};

// ResourceAccumulator manages a flow of asynchronous callbacks from
// the web inspector, storing results along the way and finally
// invoking the client callback when all results have been
// accumulated.
pagespeed.ResourceAccumulator = function(clientCallback) {
  this.clientCallback_ = clientCallback;
  this.nextEntryIndex_ = 0;
  this.cancelled_ = false;
  this.har_ = null;
};

// Start the accumulator.
pagespeed.ResourceAccumulator.prototype.start = function() {
  webInspector.resources.getHAR(
    pagespeed.withErrorHandler(this.onHAR_.bind(this)));
};

// Cancel the accumulator.
pagespeed.ResourceAccumulator.prototype.cancel = function() {
  this.cancelled_ = true;
};

pagespeed.ResourceAccumulator.prototype.onHAR_ = function(har) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  pagespeed.assert(this.har_ === null);
  this.har_ = har;
  this.getNextEntryBody_();
};

pagespeed.ResourceAccumulator.prototype.getNextEntryBody_ = function() {
  if (this.nextEntryIndex_ >= this.har_.entries.length) {
    this.clientCallback_({log: this.har_});  // We're finished.
  } else {
    this.har_.entries[this.nextEntryIndex_].getContent(
      pagespeed.withErrorHandler(this.onBody_.bind(this)));
  }
}

pagespeed.ResourceAccumulator.prototype.onBody_ = function(text, encoding) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  var content = this.har_.entries[this.nextEntryIndex_].response.content;
  content.text = text;
  content.encoding = encoding;
  ++this.nextEntryIndex_;
  this.getNextEntryBody_();
};
