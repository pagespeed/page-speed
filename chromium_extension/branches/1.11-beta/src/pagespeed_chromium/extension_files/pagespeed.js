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

  // A port object connected to the extension background page; this is
  // initialized at the bottom of this file.
  connectionPort: null,

  // The current results, in JSON form.  This is null iff there are no
  // currently displayed results in the UI.
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
      pagespeed.connectionPort.postMessage({kind: 'openUrl', url: href});
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

  // TODO(mdsteele): This is a hack -- impact scores are relative, not
  //   absolute, so we shouldn't be comparing them to constants.  We should
  //   decide on a better way to do this.
  makeImpactIcon: function (impact) {
    pagespeed.assert(typeof(impact) === 'number',
                     'makeImpactIcon: score must be a number');
    pagespeed.assert(isFinite(impact), 'makeImpactIcon: score must be finite');
    var icon = pagespeed.makeElement('div', (impact < 3 ? 'icon-okay' :
                                             impact < 10 ? 'icon-warn' :
                                             'icon-error'));
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

    // Sort the rule results, first by impact (descending), then by number of
    // results (descending), then by name (ascending).  Sorting by number of
    // results is in there so that zero-impact rules with results come before
    // zero-impact rules with no results.
    var rule_results = pagespeed.currentResults.results.rule_results.slice();
    rule_results.sort(function (result1, result2) {
      return (pagespeed.compare(result2.rule_impact, result1.rule_impact) ||
              pagespeed.compare((result2.url_blocks || []).length,
                                (result1.url_blocks || []).length) ||
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
        (localStorage.debug ? pagespeed.makeScoreIcon(rule_result.rule_score) :
         pagespeed.makeImpactIcon(rule_result.rule_impact)),
        (localStorage.debug ? '[' + rule_result.rule_impact + '] ' : null),
        rule_result.localized_rule_name
      ]);
      if (!rule_result.url_blocks) {
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

  showErrorMessage: function (problem) {
    // Remove the previous results.
    var results_container = document.getElementById('results-container');
    pagespeed.removeAllChildren(results_container);

    // Create the error pane.
    var error_container = pagespeed.makeElement('div');
    error_container.id = 'error-container';
    // TODO(mdsteele): Localize these error messages.
    if (problem === 'incognito') {
      // TODO(mdsteele): It'd be nice if "loading this page in a regular
      //   browser window" were a link that would do just that.
      error_container.appendChild(pagespeed.makeElement('p', null, [
        "Oops, looks like you're trying to run Page Speed in an incognito ",
        "window, but you haven't enabled the Page Speed extension in ",
        "incognito windows.  Try either loading this page in a regular ",
        "browser window, or else going to the ",
        pagespeed.makeLink('chrome://extensions', 'extensions page'),
        " and checking the \"Allow in incognito\" box next to the ",
        "Page Speed extension."
      ]));
    } else if (problem === 'url') {
      error_container.appendChild(pagespeed.makeElement('p', null, [
        "Sorry, Page Speed can only analyze pages at ",
        pagespeed.makeElement('code', null, 'http://'), " or ",
        pagespeed.makeElement('code', null, 'https://'),
        " URLs.  Please try another page."
      ]));
    } else {
      throw new Error("Unexpected problem: " + JSON.stringify(problem));
    }
    error_container.appendChild(pagespeed.makeButton(
      'Clear', pagespeed.clearResults)),
    results_container.appendChild(error_container);

    // Display the results.
    var welcome_container = document.getElementById('welcome-container');
    welcome_container.style.display = 'none';
    results_container.style.display = 'block';
    document.getElementById('run-button').disabled = true;
  },

  // Handle messages from the background page (coming over the connectionPort).
  messageHandler: function (message) {
    if (message.kind === 'approveTab') {
      if (pagespeed.resourceAccumulator) {
        pagespeed.resourceAccumulator.start();
      }
    } else if (message.kind === 'rejectTab') {
      pagespeed.endCurrentRun();
      pagespeed.showErrorMessage(message.reason);
    } else if (message.kind === 'setStatusText') {
      pagespeed.setStatusText(message.message);
    } else if (message.kind === 'results') {
      pagespeed.onPageSpeedResults(message.results);
    } else {
      throw new Error('Unknown message kind: ' + message.kind);
    }
  },

  // Run Page Speed and display the results. This is done asynchronously using
  // the ResourceAccumulator.
  runPageSpeed: function () {
    // Cancel the previous run, if any.
    pagespeed.endCurrentRun();
    // Indicate in the UI that we are currently running.
    document.getElementById('run-button').disabled = true;
    document.getElementById('spinner-img').style.display = 'inline';
    // Instatiate a resource accumulator now, so that when an approveTab
    // message comes back, we know we're ready to run.
    pagespeed.resourceAccumulator = new pagespeed.ResourceAccumulator(
      pagespeed.withErrorHandler(pagespeed.onResourceAccumulatorComplete));
    // Before we start, we need to determine if we'll be able to run our
    // content script on this tab.  However, to determine this we need to call
    // chrome.tab.get(), which we can't do from here.  Thus, we ask the
    // background page to do it for us.  It will reply with either an
    // approveTab or rejectTab message, which will be handled in
    // pagespeed.messageHandler().
    pagespeed.setStatusText('Checking tab...');
    pagespeed.connectionPort.postMessage({
      kind: 'checkTab',
      tab_id: webInspector.inspectedWindow.tabId
    });
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
    pagespeed.setStatusText('Sending request to background page...');
    pagespeed.connectionPort.postMessage(request);
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
    pagespeed.connectionPort.postMessage({
      kind: 'cancelRun',
      tab_id: webInspector.inspectedWindow.tabId
    });
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
    if (!pagespeed.resourceAccumulator) {
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
  },

  // Called once when we first load pagespeed-panel.html, to initialize the UI,
  // with localization.
  initializeUI: function () {
    // Initialize the "analyze" dropdown menu.
    var analyze = document.getElementById('analyze-dropdown');
    [['all', chrome.i18n.getMessage('analyze_entire_page')],
     ['ads', chrome.i18n.getMessage('analyze_ads_only')],
     ['trackers', chrome.i18n.getMessage('analyze_trackers_only')],
     ['content', chrome.i18n.getMessage('analyze_content_only')]
    ].forEach(function (item) {
      var option = pagespeed.makeElement('option', null, item[1]);
      option.setAttribute('value', item[0]);
      analyze.appendChild(option);
    });
    // Initialize the welcome pane.
    // TODO(mdsteele): Localize this stuff too, once we decide what it should
    //   look like.
    var whatsnew = document.getElementById('whats-new-container');
    whatsnew.appendChild(pagespeed.makeElement('h1', null, "Page Speed"));
    whatsnew.appendChild(pagespeed.makeElement(
      'h2', null, "What's new in Page Speed 1.11 beta?"));
    whatsnew.appendChild(pagespeed.makeElement('ul', null, [
      pagespeed.makeElement('li', null, "It's in Chrome!"),
      pagespeed.makeElement('li', null, "Localized rule results"),
      pagespeed.makeElement('li', null, "Improved scoring and suggestion ordering"),
      pagespeed.makeElement('li', null, [ "New rules",
        pagespeed.makeElement('ul', null, [
          pagespeed.makeElement('li', null, "Defer parsing of JavaScript"),
          pagespeed.makeElement('li', null, "Enable Keep-Alive"),
          pagespeed.makeElement('li', null, "Make landing page redirects cacheable")
        ])
      ])
    ]));
    whatsnew.appendChild(pagespeed.makeElement('p', null, [
      'See the ', pagespeed.makeLink(
        'http://code.google.com/speed/page-speed/docs/rules_intro.html',
        'Page Speed documentation'),
      ' for detailed information on the rules used to evaluate web pages.'
    ]));
    whatsnew.appendChild(pagespeed.makeElement(
      'p', null, 'Page Speed Copyright \xA9 2011 Google Inc.'));
    // Refresh the run button, etc.
    pagespeed.clearResults();
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
  this.timeoutId_ = null;
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
    // Ask the DevTools panel to give us the content of this resource.
    entry.getContent(pagespeed.withErrorHandler(
      this.onBody_.bind(this, this.nextEntryIndex_)));
    // Sometimes the above call never calls us back.  This is a bug.  In the
    // meantime, give it at most 2 seconds before we time out and move on.
    pagespeed.assert(this.timeoutId_ === null);
    this.timeoutId_ = setTimeout(pagespeed.withErrorHandler(
      this.timeOut_.bind(this, this.nextEntryIndex_)), 2000);
  }
};

pagespeed.ResourceAccumulator.prototype.timeOut_ = function (index) {
  if (this.cancelled_ || index !== this.nextEntryIndex_) {
    return;  // We've been cancelled so ignore the callback.
  }
  webInspector.log("Timed out while fetching [" + index + "/" +
                   this.har_.entries.length + "] " +
                   this.har_.entries[index].request.url);
  this.timeoutId_ = null;
  ++this.nextEntryIndex_;
  this.getNextEntryBody_();
};

pagespeed.ResourceAccumulator.prototype.onBody_ = function (index, text,
                                                            encoding) {
  if (this.cancelled_ || index !== this.nextEntryIndex_) {
    return;  // We've been cancelled so ignore the callback.
  }
  pagespeed.assert(this.timeoutId_ !== null);
  clearTimeout(this.timeoutId_);
  this.timeoutId_ = null;
  var content = this.har_.entries[this.nextEntryIndex_].response.content;
  // We need the || here because sometimes we get back null for `text'.
  // TODO(mdsteele): That's a bad thing.  Is it fixable?
  content.text = text || "";
  content.encoding = encoding;
  ++this.nextEntryIndex_;
  this.getNextEntryBody_();
};

// Connect to the extension background page.
pagespeed.connectionPort = chrome.extension.connect();
pagespeed.connectionPort.onMessage.addListener(
  pagespeed.withErrorHandler(pagespeed.messageHandler));

// Listen for when we change pages.
webInspector.inspectedWindow.onNavigated.addListener(
  pagespeed.withErrorHandler(pagespeed.onPageNavigate));

// Listen for when the page finishes (re)loading.
webInspector.inspectedWindow.onLoaded.addListener(
  pagespeed.withErrorHandler(pagespeed.onPageLoaded));
