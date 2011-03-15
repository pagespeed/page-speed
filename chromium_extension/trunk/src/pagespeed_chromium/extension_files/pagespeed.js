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
        var message = 'Error in Page Speed panel:\n ' + e.stack;
        alert(message + '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
        webInspector.log(message);
        pagespeed.endCurrentRun();
        pagespeed.setStatusText('ERROR');
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

  // Set the text of the status bar.
  setStatusText: function (text) {
    var status_container = document.getElementById('status-text');
    pagespeed.removeAllChildren(status_container);
    if (text) {
      status_container.appendChild(document.createTextNode(text));
    }
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

  ruleDocumentationUrl: function (rule_name) {
    return 'http://code.google.com/speed/page-speed/docs/' + ({
      AvoidBadRequests: 'rtt.html#AvoidBadRequests',
      AvoidCssImport: 'rtt.html#AvoidCssImport',
      AvoidDocumentWrite: 'rtt.html#AvoidDocumentWrite',
      CombineExternalCss: 'rtt.html#CombineExternalCss',
      CombineExternalJavaScript: 'rtt.html#CombineExternalJS',
      EnableGzipCompression: 'payload.html#GzipCompression',
      EnableKeepAlive: 'rtt.html#EnableKeepAlive',
      InlineSmallCss: 'caching.html#InlineSmallResources',
      InlineSmallJavaScript: 'caching.html#InlineSmallResources',
      LeverageBrowserCaching: 'caching.html#LeverageBrowserCaching',
      MinifyCss: 'payload.html#MinifyCss',
      MinifyHTML: 'payload.html#MinifyHTML',
      MinifyJavaScript: 'payload.html#MinifyJS',
      MinimizeDnsLookups: 'rtt.html#MinimizeDNSLookups',
      MinimizeRedirects: 'rtt.html#AvoidRedirects',
      MinimizeRequestSize: 'request.html#MinimizeRequestSize',
      OptimizeImages: 'payload.html#CompressImages',
      OptimizeTheOrderOfStylesAndScripts: 'rtt.html#PutStylesBeforeScripts',
      ParallelizeDownloadsAcrossHostnames: 'rtt.html#ParallelizeDownloads',
      PreferAsyncResources: 'rtt.html#PreferAsyncResources',
      PutCssInTheDocumentHead: 'rendering.html#PutCSSInHead',
      RemoveQueryStringsFromStaticResources:
        'caching.html#LeverageProxyCaching',
      ServeResourcesFromAConsistentUrl: 'payload.html#duplicate_resources',
      ServeScaledImages: 'payload.html#ScaleImages',
      SpecifyACacheValidator: 'caching.html#LeverageBrowserCaching',
      SpecifyAVaryAcceptEncodingHeader: 'caching.html#LeverageProxyCaching',
      SpecifyCharsetEarly: 'rendering.html#SpecifyCharsetEarly',
      SpecifyImageDimensions: 'rendering.html#SpecifyImageDimensions',
      SpriteImages: 'rtt.html#SpriteImages'
    }[rule_name] || 'rules_intro.html');
  },

  // Given a list of objects produced by
  // FormattedResultsToJsonConverter::ConvertFormatString(),
  // build an array of DOM nodes, suitable to be passed to makeElement().
  formatFormatString: function (format_string) {
    var elements = [];
    var string = format_string.format;
    var index;
    // Search for the next "$n" (where n is a digit from 1 to 9).
    while ((index = string.search(/\$[1-9]/)) >= 0) {
      // Add everything up to the "$n" as a literal string.
      elements.push(string.substr(0, index));
      // Get the digit "n" as a string.
      var argnum = string.substr(index + 1, 1);
      // Parse "n" into a number and get the (n-1)th format argument.
      var arg = format_string.args[parseInt(argnum, 10) - 1];
      if (!arg) {
        // If there's no (n-1)th argument, replace with question marks.
        elements.push('???');
      } else if (arg.type === 'url') {
        // If the argument is a URL, create a link.
        elements.push(pagespeed.makeLink(
          arg.string_value, pagespeed.getDisplayUrl(arg.localized_value)));
      } else {
        // Otherwise, replace the argument with a string.
        elements.push(arg.localized_value);
      }
      // Clip off the beginning of the format string, up to and including the
      // "$n" that we found, and then keep going.
      string = string.substr(index + 2);
    }
    // There's no more "$n"'s left, so add the remainder of the format string.
    elements.push(string);
    return elements;
  },

  // Given a list of objects produced by
  // FormattedResultsToJsonConverter::ConvertFormattedUrlBlockResults(),
  // build an array of DOM nodes, suitable to be passed to makeElement().
  formatUrlBlocks: function (url_blocks) {
    return (url_blocks || []).map(function (url_block) {
      return pagespeed.makeElement('p', null, [
        pagespeed.formatFormatString(url_block.header),
        (!url_block.urls ? [] :
         pagespeed.makeElement('ul', null, url_block.urls.map(function (url) {
           return pagespeed.makeElement('li', null, [
             pagespeed.formatFormatString(url.result),
             (!url.details ? [] :
              pagespeed.makeElement('ul', null, url.details.map(function (dt) {
                return pagespeed.makeElement(
                  'li', null, pagespeed.formatFormatString(dt));
              })))]);
         })))]);
    });
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
    pagespeed.endCurrentRun();
    pagespeed.currentResults = null;
    var results_container = document.getElementById('results-container');
    results_container.style.display = 'none';
    pagespeed.removeAllChildren(results_container);
    var welcome_container = document.getElementById('welcome-container');
    welcome_container.style.display = 'block';
    pagespeed.setRunButtonText(chrome.i18n.getMessage('run_page_speed'));
  },

  // Format and display the current results.
  showResults: function () {
    pagespeed.assert(pagespeed.currentResults !== null,
                     "showResults: pagespeed.currentResults must not be null");

    // Remove the previous results.
    var results_container = document.getElementById('results-container');
    pagespeed.removeAllChildren(results_container);

    // Sort the rule results, first by score, then by name.
    // TODO(mdsteele): Once we have impact-based scores, sort by impact.
    var rule_results = pagespeed.currentResults.results.rule_results.slice();
    rule_results.sort(function (result1, result2) {
      return (pagespeed.compare(result1.rule_score, result2.rule_score) ||
              pagespeed.compare(result1.localized_rule_name,
                                result2.localized_rule_name));
    });
    var overall_score = pagespeed.currentResults.results.score;

    // Create the score bar.
    var analyze = pagespeed.currentResults.analyze;
    results_container.appendChild(pagespeed.makeElement('div', 'score-bar', [
      pagespeed.makeElement('div', null, chrome.i18n.getMessage(
        (analyze === 'ads' ? 'overall_score_ads' :
         analyze === 'trackers' ? 'overall_score_trackers' :
         analyze === 'content' ? 'overall_score_content' :
         'overall_score_all'), [overall_score])),
      pagespeed.makeScoreIcon(overall_score),
      pagespeed.makeButton(chrome.i18n.getMessage('clear_results'),
                           pagespeed.clearResults),
      pagespeed.makeButton(chrome.i18n.getMessage('collapse_all'),
                           pagespeed.collapseAllResults),
      pagespeed.makeButton(chrome.i18n.getMessage('expand_all'),
                           pagespeed.expandAllResults)
    ]));

    // Create the rule results.
    var rules_container = pagespeed.makeElement('div');
    rules_container.id = 'rules-container';
    rule_results.forEach(function (rule_result) {
      var header = pagespeed.makeElement('div', 'header', [
        pagespeed.makeScoreIcon(rule_result.rule_score),
        rule_result.localized_rule_name
      ]);
      if (rule_result.rule_score >= 100) {
        header.style.fontWeight = 'normal';
      }
      var formatted = pagespeed.formatUrlBlocks(rule_result.url_blocks);
      var result_div = pagespeed.makeElement('div', 'result', [
        header,
        pagespeed.makeElement('div', 'details', [
          (formatted.length > 0 ? formatted :
           pagespeed.makeElement('p', null, chrome.i18n.getMessage(
             'no_rule_violations'))),
          pagespeed.makeElement('p', null, pagespeed.makeLink(
            pagespeed.ruleDocumentationUrl(rule_result.rule_name),
            chrome.i18n.getMessage('more_information')))
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
    pagespeed.setRunButtonText(chrome.i18n.getMessage('refresh_results'));
  },

  // Run Page Speed and display the results. This is done
  // asynchronously using the ResourceAccumulator.
  runPageSpeed: function () {
    pagespeed.endCurrentRun();
    document.getElementById('run-button').disabled = true;
    document.getElementById('spinner-img').style.display = 'inline';
    pagespeed.resourceAccumulator = new pagespeed.ResourceAccumulator(
      pagespeed.withErrorHandler(pagespeed.onResourceAccumulatorComplete));
    pagespeed.resourceAccumulator.start();
  },

  // Invoked when the ResourceAccumulator has finished collecting data
  // from the web inspector.
  onResourceAccumulatorComplete: function (har) {
    pagespeed.resourceAccumulator = null;

    // Prepare the request.
    var analyze = document.getElementById('analyze-dropdown').value;
    var input = {analyze: analyze, har: har};
    var tab_id = webInspector.inspectedWindow.tabId;
    var request = {kind: 'runPageSpeed', input: input, tab_id: tab_id};

    // Tell the background page to run the Page Speed rules.
    pagespeed.setStatusText(chrome.i18n.getMessage('running_rules'));
    chrome.extension.sendRequest(
        request, pagespeed.withErrorHandler(pagespeed.onPageSpeedResults));
  },

  // Invoked in response to our background page running Page Speed on
  // the current page.
  onPageSpeedResults: function (response) {
    if (!response) {
      throw new Error("No response to runPageSpeed request.");
    } else if (response.error_message) {
      webInspector.log(response.error_message);
    } else if (response.results.error_message) {
      webInspector.log(response.results.error_message);
    } else {
      pagespeed.currentResults = response;
      pagespeed.showResults();
    }
    pagespeed.endCurrentRun();
  },

  // Cancel the current run, if any, and reset the status indicators.
  endCurrentRun: function () {
    if (pagespeed.resourceAccumulator) {
      pagespeed.resourceAccumulator.cancel();
      pagespeed.resourceAccumulator = null;
    }
    document.getElementById('run-button').disabled = false;
    document.getElementById('spinner-img').style.display = 'none';
    pagespeed.setStatusText(null);
  },

  // Callback to be called when the user changes the value of the "Analyze"
  // menu.
  onAnalyzeDropdownChange: function () {
    if (pagespeed.currentResults !== null &&
        document.getElementById('analyze-dropdown').value !==
        pagespeed.currentResults.analyze) {
      pagespeed.runPageSpeed();
    }
  },

  // Callback for when we navigate to a new page.
  onPageNavigate: function () {
    // If there's an active ResourceAccumulator, it must be trying to reload
    // the page, so don't do anything.  Otherwise, if there are results
    // showing, they're from another page, so clear them.
    if (pagespeed.currentResults && !pagespeed.resourceAccumulator) {
      // TODO(mdsteele): Alternatively, we could automatically re-run the
      //   rules.  Maybe we should have a user preference to decide which?
      pagespeed.clearResults();
    }
  },

  // Callback for when the inspected page loads.
  onPageLoaded: function () {
    // If there's an active ResourceAccumulator, it must be trying to reload
    // the page, so let it know that it loaded.
    if (pagespeed.resourceAccumulator) {
      pagespeed.resourceAccumulator.onPageLoaded();
    }
  }

};

// ResourceAccumulator manages a flow of asynchronous callbacks from
// the web inspector, storing results along the way and finally
// invoking the client callback when all results have been
// accumulated.
pagespeed.ResourceAccumulator = function (clientCallback) {
  this.clientCallback_ = clientCallback;
  this.nextEntryIndex_ = 0;
  this.cancelled_ = false;
  this.har_ = null;
  this.doingReload_ = false;
};

// Start the accumulator.
pagespeed.ResourceAccumulator.prototype.start = function () {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  pagespeed.setStatusText(chrome.i18n.getMessage('fetching_har'));
  webInspector.resources.getHAR(
    pagespeed.withErrorHandler(this.onHAR_.bind(this)));
};

// Cancel the accumulator.
pagespeed.ResourceAccumulator.prototype.cancel = function () {
  this.cancelled_ = true;
};

pagespeed.ResourceAccumulator.prototype.onPageLoaded = function () {
  if (this.doingReload_) {
    this.doingReload_ = false;
    if (!this.cancelled_) {
      // The page finished loading, but let's wait 100 milliseconds for
      // post-onLoad things to finish up before we start scoring.
      setTimeout(pagespeed.withErrorHandler(this.start.bind(this)), 100);
    }
  }
};

pagespeed.ResourceAccumulator.prototype.onHAR_ = function (har) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  pagespeed.assert(this.har_ === null);

  // The HAR will only include resources that were loaded while the DevTools
  // panel was open, but we need all the resources.  Our trick is this: if (and
  // only if) the DevTools panel was open when the page started loading, then
  // the first resource entry will be the main document resource (I think).
  // So, we check to see if the URL of the first resource matches the URL of
  // the page.  If so, we assume we have everything, and continue.  If not, we
  // reload the page; when the page finishes loading, our callback will call
  // the onPageLoaded() method of this ResourceAccumulator, and we can try
  // again.
  if (har.entries.length === 0 ||
      har.entries[0].request.url !== har.entries[0].pageref) {
    pagespeed.setStatusText(chrome.i18n.getMessage('reloading_page'));
    this.doingReload_ = true;
    webInspector.inspectedWindow.reload();
  } else {
    this.har_ = har;
    this.getNextEntryBody_();
  }
};

pagespeed.ResourceAccumulator.prototype.getNextEntryBody_ = function () {
  if (this.nextEntryIndex_ >= this.har_.entries.length) {
    this.clientCallback_({log: this.har_});  // We're finished.
  } else {
    var entry = this.har_.entries[this.nextEntryIndex_];
    pagespeed.setStatusText(chrome.i18n.getMessage(
      'fetching_content', [this.nextEntryIndex_ + 1, this.har_.entries.length,
                           entry.request.url]));
    entry.getContent(pagespeed.withErrorHandler(this.onBody_.bind(this)));
  }
};

pagespeed.ResourceAccumulator.prototype.onBody_ = function (text, encoding) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  var content = this.har_.entries[this.nextEntryIndex_].response.content;
  // We need the || here because sometimes we get back null for `text'.
  // TODO(mdsteele): That's a bad thing.  Is it fixable?
  content.text = text || "";
  content.encoding = encoding;
  ++this.nextEntryIndex_;
  this.getNextEntryBody_();
};

// Listen for when we change pages.
webInspector.inspectedWindow.onNavigated.addListener(
  pagespeed.withErrorHandler(pagespeed.onPageNavigate));

// Listen for when the page finishes (re)loading.
webInspector.inspectedWindow.onLoaded.addListener(
  pagespeed.withErrorHandler(pagespeed.onPageLoaded));
