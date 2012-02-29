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

  // The currently active DomCollector, if any.
  domCollector: null,

  // The currently active ContentWriter, if any.
  contentWriter: null,

  // All timeline events that have been recorded from when the page started
  // loading until it finished loading.
  timelineEvents: [],
  pageHasLoaded: true,

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
  // opt_label -- the visible text for the link (if omitted, use href)
  makeLink: function (href, opt_label) {
    var link = pagespeed.makeElement('a', null, opt_label || href);
    link.href = 'javascript:null';
    var openUrl = function () {
      // Tell the background page to open the url in a new tab.  It will use
      // chrome.tabs.create(), which we can't call from here.
      pagespeed.connectionPort.postMessage({kind: 'openUrl', url: href});
    };
    link.addEventListener('click', pagespeed.withErrorHandler(openUrl), false);
    return link;
  },

  // Given a score, return a DOM node for a red/yellow/green icon.
  makeScoreIcon: function (score, opt_hasNoResults, opt_isExperimentalRule) {
    pagespeed.assert(typeof(score) === 'number',
                     'makeScoreIcon: score must be a number');
    pagespeed.assert(isFinite(score), 'makeScoreIcon: score must be finite');
    var icon = pagespeed.makeElement('div',
      (opt_hasNoResults ? 'icon-na' : opt_isExperimentalRule ? 'icon-info' :
       score > 80 ? 'icon-okay' : score > 60 ? 'icon-warn' : 'icon-error'));
    icon.setAttribute('title', 'Score: ' + score + '/100');
    return icon;
  },

  // TODO(mdsteele): This is a hack -- impact scores are relative, not
  //   absolute, so we shouldn't be comparing them to constants.  We should
  //   decide on a better way to do this.
  makeImpactIcon: function (impact, opt_hasNoResults, opt_isExperimentalRule) {
    pagespeed.assert(typeof(impact) === 'number',
                     'makeImpactIcon: score must be a number');
    pagespeed.assert(isFinite(impact), 'makeImpactIcon: score must be finite');
    var icon = pagespeed.makeElement('div',
      (opt_hasNoResults ? 'icon-na' : opt_isExperimentalRule ? 'icon-info' :
       impact < 3 ? 'icon-okay' : impact < 10 ? 'icon-warn' : 'icon-error'));
    if (opt_hasNoResults) {
      icon.title = 'No suggestions for this rule. Good job!';
    } else if (opt_isExperimentalRule) {
      icon.title =
          'Experimental rule. Does not yet impact overall score. ' +
          'Send feedback to page-speed-discuss@googlegroups.com.';
    }
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
      CombineExternalCss: 'rtt.html#CombineExternalCSS',
      CombineExternalJavaScript: 'rtt.html#CombineExternalJS',
      EnableGzipCompression: 'payload.html#GzipCompression',
      EnableKeepAlive: 'rtt.html#EnableKeepAlive',
      InlineSmallCss: 'caching.html#InlineSmallResources',
      InlineSmallJavaScript: 'caching.html#InlineSmallResources',
      LeverageBrowserCaching: 'caching.html#LeverageBrowserCaching',
      MinifyCss: 'payload.html#MinifyCSS',
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
      } else if (arg.type === 'pre') {
        elements.push(
            pagespeed.makeElement('pre', 'result', arg.localized_value));
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

  formatOptimizedContentIfAny: function (id) {
    if (typeof(id) !== 'number') {
      return null;
    }
    var entry = pagespeed.currentResults.optimizedContent[id.toString()];
    if (!entry || !entry.url) {
      return null;
    }
    return ['  See ', pagespeed.makeLink(entry.url, 'optimized version'), '.'];
  },

  // Given a list of objects produced by
  // FormattedResultsToJsonConverter::ConvertFormattedUrlBlockResults(),
  // build an array of DOM nodes, suitable to be passed to makeElement().
  formatUrlBlocks: function (url_blocks) {
    return (url_blocks || []).map(function (url_block) {
      return pagespeed.makeElement('p', null, [
        pagespeed.formatFormatString(url_block.header),
        pagespeed.formatOptimizedContentIfAny(url_block.associated_result_id),
        (!url_block.urls ? [] :
         pagespeed.makeElement('ul', null, url_block.urls.map(function (url) {
           return pagespeed.makeElement('li', null, [
             pagespeed.formatFormatString(url.result),
             pagespeed.formatOptimizedContentIfAny(url.associated_result_id),
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

    // Sort the rule results.  All rules with no results come last, in
    // alphabetical order by rule name.  Experimental rules that have results
    // come second-to-last, ordered by impact (descending) and then by rule
    // name.  Non-experimental rules with results come first, again by impact
    // and then rule name.
    var rule_results = pagespeed.currentResults.results.rule_results.slice();
    rule_results.sort(function (result1, result2) {
      var empty1 = (result1.url_blocks || []).length === 0;
      var empty2 = (result2.url_blocks || []).length === 0;
      return (pagespeed.compare(empty1, empty2) ||
              (empty1 || empty2 ? 0 :
               (pagespeed.compare(!!result1.experimental,
                                  !!result2.experimental) ||
                pagespeed.compare(result2.rule_impact,
                                  result1.rule_impact)))  ||
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
        (localStorage.debug ?
         pagespeed.makeScoreIcon(rule_result.rule_score,
                                 (rule_result.url_blocks || []).length === 0,
                                 rule_result.experimental) :
         pagespeed.makeImpactIcon(rule_result.rule_impact,
                                  (rule_result.url_blocks || []).length === 0,
                                  rule_result.experimental)),
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
    if (problem === 'url') {
      error_container.appendChild(pagespeed.makeElement('p', null, [
        "Sorry, Page Speed can only analyze pages at ",
        pagespeed.makeElement('code', null, 'http://'), " or ",
        pagespeed.makeElement('code', null, 'https://'),
        " URLs.  Please try another page."
      ]));
    } else if (problem === 'moduleDidNotLoad') {
      error_container.appendChild(pagespeed.makeElement('p', null, [
        'Unfortunately, the Page Speed plugin was not able to load.',
        '  The usual reason for this is that "Page Speed Plugin" is disabled',
        ' in the ', pagespeed.makeLink('about:plugins'),
        ' page.  Try enabling "Page Speed Plugin" and then closing and',
        ' reopening the Chrome Developer Tools window, and try again.  If you',
        ' still get this error message, please ',
        pagespeed.makeLink('http://code.google.com/p/page-speed/issues/',
                           'file a bug'), '.'
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
    if (message.kind === 'partialResourcesFetched') {
      pagespeed.domCollector = new pagespeed.DomCollector(
        pagespeed.withErrorHandler(function (dom) {
          message.dom = dom;
          pagespeed.onCollectedInput(message);
      }));
      pagespeed.setStatusText("Collecting page DOM...");
      pagespeed.domCollector.start();
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

    // Check that the inspected window has an http[s] url.
    pagespeed.setStatusText('Checking tab...');
    chromeDevTools.inspectedWindow.eval("location.href.match(/^http/)",
      pagespeed.withErrorHandler(function (tabOk) {
        if (tabOk) {
          // Make sure the run has not been canceled.
          if (pagespeed.resourceAccumulator) {
            pagespeed.resourceAccumulator.start();
          }
        } else {
          pagespeed.endCurrentRun();
          pagespeed.showErrorMessage("url");
        }
    }));
  },

  // Invoked when the ResourceAccumulator has finished collecting data
  // from the web inspector.
  onResourceAccumulatorComplete: function (har) {
    pagespeed.resourceAccumulator = null;

    // Tell the background page to collect the missing bits of the
    // HAR.  It will respond with a partialResourcesFetched message,
    // which will be handled in pagespeed.messageHandler().
    pagespeed.setStatusText("Fetching partial resources...");
    pagespeed.connectionPort.postMessage({
      kind: 'fetchPartialResources',
      analyze: document.getElementById('analyze-dropdown').value,
      har: har,
    });
  },

  // Invoked when the background page has finished collecting any
  // missing parts of the HAR and the DOM has been retrieved.
  onCollectedInput: function (input) {
    pagespeed.setStatusText(chrome.i18n.getMessage('running_rules'));
    // Wrap this next part in a setTimeout (with zero delay) to give Chrome a
    // chance to redraw the screen and display the above message.
    setTimeout(pagespeed.withErrorHandler(function () {
      var pagespeed_module = document.getElementById('pagespeed-module');
      // Check if the module has loaded.
      try {
        pagespeed_module.ping();
      } catch (e) {
        pagespeed.endCurrentRun();
        pagespeed.showErrorMessage('moduleDidNotLoad');
        return;
      }
      // Run the rules.
      var saveOptimizedContent = !localStorage.noOptimizedContent;
      var output = JSON.parse(pagespeed_module.runPageSpeed(
        JSON.stringify(input.har),
        JSON.stringify(input.dom),
        JSON.stringify(pagespeed.timelineEvents),
        input.analyze,
        chrome.i18n.getMessage('@@ui_locale'),
        saveOptimizedContent));
      pagespeed.currentResults = {
        analyze: input.analyze,
        optimizedContent: output.optimizedContent,
        results: output.results
      };
      // If we need to save optimized content, then start up a ContentWriter
      // and tell it to show results (that is, call onContentWriterComplete)
      // when it finishes.  If we're not saving optimized content, skip
      // straight to showing the results (by calling onContentWriterComplete
      // immediately).
      if (saveOptimizedContent) {
        pagespeed.contentWriter = new pagespeed.ContentWriter(
          output.optimizedContent,
          pagespeed.withErrorHandler(pagespeed.onContentWriterComplete));
        pagespeed.setStatusText("Saving optimized content...");
        pagespeed.contentWriter.start();
      } else {
        pagespeed.onContentWriterComplete();
      }
    }), 0);
  },

  // Invoked when the ContentWriter has finished serializing optimized content.
  // Displays the results and ends the run.
  onContentWriterComplete: function () {
    pagespeed.contentWriter = null;
    pagespeed.showResults();
    pagespeed.endCurrentRun();
  },

  // Cancel the current run, if any, and reset the status indicators.
  endCurrentRun: function () {
    if (pagespeed.domCollector) {
      pagespeed.domCollector.cancel();
      pagespeed.domCollector = null;
    }
    if (pagespeed.resourceAccumulator) {
      pagespeed.resourceAccumulator.cancel();
      pagespeed.resourceAccumulator = null;
    }
    if (pagespeed.contentWriter) {
      pagespeed.contentWriter.cancel();
      pagespeed.contentWriter = null;
    }
    document.getElementById('run-button').disabled = false;
    document.getElementById('spinner-img').style.display = 'none';
    pagespeed.setStatusText(null);
    pagespeed.connectionPort.postMessage({
      kind: 'cancelRun',
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
    // Clear the list of timeline events.
    pagespeed.timelineEvents.length = 0;
    pagespeed.pageHasLoaded = false;
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
    pagespeed.pageHasLoaded = true;
    // If there's an active ResourceAccumulator, it must be trying to reload
    // the page, so let it know that it loaded.
    if (pagespeed.resourceAccumulator) {
      pagespeed.resourceAccumulator.onPageLoaded();
    }
    // Otherwise, if we have run-at-onload enabled, we should start a run now.
    else if (localStorage.runAtOnLoad) {
      pagespeed.runPageSpeed();
    }
  },

  // Callback for when a timeline event is recorded (via the timeline API).
  onTimelineEvent: function (event) {
    pagespeed.timelineEvents.push(event);
    if (event.type === "MarkLoad") {
      pagespeed.onPageLoaded();
    }
  },

  // Called once when we first load pagespeed-panel.html, to initialize the UI,
  // with localization.
  initializeUI: function () {
    // The content_security_policy does not allow us to inline event
    // handlers or style settings... initialize these settings here.
    var logo = document.getElementById('logo-img');
    logo.style.float = "left";

    var whats_new_container = document.getElementById('whats-new-container');
    whats_new_container.style.float = "left";

    var run_button = document.getElementById('run-button');
    run_button.onclick = pagespeed.runPageSpeed;

    // Initialize the "analyze" dropdown menu.
    var analyze = document.getElementById('analyze-dropdown');
    analyze.onchange = pagespeed.onAnalyzeDropdownChange;
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
    if (localStorage.debug) {
      // When the super-secret debug mode is enabled, give us a textbox where
      // we can eval things in the context of our Devtools panel.  There does
      // not appear to be any to get a proper console for this context, so this
      // feature is very helpful for debugging.
      var box = pagespeed.makeElement('input');
      box.setAttribute('size', '80');
      whatsnew.appendChild(pagespeed.makeElement('div', null, [
        pagespeed.makeButton('Eval', function () {
          try { alert('' + JSON.stringify(eval(box.value))); }
          catch (e) { alert(e.stack); }
        }), box
      ]));
    }
    whatsnew.appendChild(pagespeed.makeElement(
      'h2', null, "What's new in Page Speed 1.12 beta?"));
    whatsnew.appendChild(pagespeed.makeElement('ul', null, [
      pagespeed.makeElement('li', null, ["Support for running Page Speed " +
        "on Chrome for Android (", pagespeed.makeLink(
        'http://code.google.com/p/page-speed/wiki/PageSpeedOnChromeForAndroid',
        'details'), ')' ]),
      pagespeed.makeElement('li', null, "More accurate minification savings " +
                            "computation for gzip-compressible resources"),
      pagespeed.makeElement('li', null, 'Ignore data URIs in "Specify image ' +
                            'dimensions"'),
      pagespeed.makeElement('li', null, [ "New rules",
        pagespeed.makeElement('ul', null, [
          pagespeed.makeElement('li', null, "Avoid excess serialization"),
          pagespeed.makeElement('li', null, "Avoid long-running scripts"),
          pagespeed.makeElement('li', null, "Eliminate unnecessary reflows")
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
      'p', null, 'Page Speed Copyright \xA9 2012 Google Inc.'));
    // Refresh the run button, etc.
    pagespeed.clearResults();
  },

};

// DomCollector manages the asynchronous callback from the inspected
// page which contains the DOM.
pagespeed.DomCollector = function (clientCallback) {
  this.clientCallback_ = clientCallback;
  this.cancelled_ = false;
};

pagespeed.DomCollector.prototype.start = function () {
  // The collector function will be evaluated in the inspected window.
  // It will be to stringed, so should make no reference to its
  // current definition context.
  var collector = function () {

    function collectElement(element, outList) {
      // TODO(mhillyard): this ensures we don't include our injected
      // iframe element in the elements list.
      if (element === frameElement) return;

      var obj = {tag: element.tagName};
      // If the tag has any attributes, add an attribute map to the
      // output object.
      var attributes = element.attributes;
      if (attributes && attributes.length > 0) {
        obj.attrs = {};
        for (var i = 0, len = attributes.length; i < len; ++i) {
          var attribute = attributes[i];
          obj.attrs[attribute.name] = attribute.value;
        }
      }
      // If the tag has any attributes, add children list to the
      // output object.
      var children = element.children;
      if (children && children.length > 0) {
        for (var j = 0, len = children.length; j < len; ++j) {
          collectElement(children[j], outList);
        }
      }
      // If the tag has a content document, add that to the output
      // object.
      if (element.contentDocument) {
        obj.contentDocument = collectDocument(element.contentDocument);
      }
      // If this is an IMG tag, record the size to which the image is
      // scaled.
      if (element.tagName === 'IMG' && element.complete) {
        obj.width = element.width;
        obj.height = element.height;
      }
      outList.push(obj);
    }

    function collectDocument(document) {
      var elements = [];
      collectElement(document.documentElement, elements);
      return {
        documentUrl: document.URL,
        baseUrl: document.baseURI,
        elements: elements
      };
    }

    // Collect the page DOM and send it back to the devtools panel.
    // TODO(mhillyard): for now, the devtools eval implementation is
    // not sandboxed properly... it might grab a JSON.stringify
    // implementation that is provided by the inspected page.  Most
    // JSON implemntations will correctly handle strings, so let's
    // pre-stringify the DOM, and then de-stringify it on the client
    // side.  To get access to the default JSON object provided by
    // Chrome, create an iframe and then access the contentWindow.
    // Furthermore, to make sure that we don't use non-standard
    // toJSON, toString, and valueOf implementations, execute the
    // collectElement and collectDocument functions within the iframe
    // as well.  This Web Inspector bug is filed here:
    // https://bugs.webkit.org/show_bug.cgi?id=63083
    var iframe = document.createElement("iframe");
    document.documentElement.appendChild(iframe);
    iframe.contentWindow.eval(collectElement.toString());
    iframe.contentWindow.eval(collectDocument.toString());
    var domObj = iframe.contentWindow.collectDocument(document);
    var domStr = iframe.contentWindow.JSON.stringify(domObj);
    document.documentElement.removeChild(iframe);
    return domStr;
  }

  // Evaluate the collector function in the inspected page
  var this_ = this;
  chromeDevTools.inspectedWindow.eval(
    "(" + collector.toString() + ")();",
    pagespeed.withErrorHandler(function (domString) {
      this_.onDomCollected_(JSON.parse(domString));
  }));
};

pagespeed.DomCollector.prototype.cancel = function () {
  this.cancelled_ = true;
};

pagespeed.DomCollector.prototype.onDomCollected_ = function (dom) {
  if (!this.cancelled_) {
    this.clientCallback_(dom);
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
  chromeDevTools.network.getHAR(
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
  // the pages field of HAR is not empty.  So, we check to see if the pages
  // exist. If so, we assume we have everything, and continue.  If not, we
  // reload the page; when the page finishes loading, our callback will call the
  // onPageLoaded() method of this ResourceAccumulator, and we can try again.

  var need_reload = false;
  if (har.entries.length === 0 || har.pages.length == 0) {
    need_reload = true;
  }

  if (need_reload) {
    pagespeed.setStatusText(chrome.i18n.getMessage('reloading_page'));
    this.doingReload_ = true;
    chromeDevTools.inspectedWindow.reload();
  } else {
    // Devtools apparently sets the onLoad timing to NaN if onLoad hasn't
    // fired yet.  Page Speed will interpret that to mean that the onLoad
    // timing is unknown, but setting it to -1 will tell Page Speed that it
    // is known not to have happened yet.
    har.pages.forEach(function (page) {
      if (isNaN(page.pageTimings.onLoad)) {
        page.pageTimings.onLoad = -1;
      }
      // Devtools introduced a new bug that makes a invalid date for page
      // startedDateTime. https://bugs.webkit.org/show_bug.cgi?id=74188
      // The bug is fixed in Chrome trunk, but before it pushes to release, we
      // need to the following hack to get around of it.
      if (isNaN(page.startedDateTime.getTime())) {
        page.startedDateTime = har.entries[0].startedDateTime;
      }
    });
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

// ContentWriter manages the serialization of optimized content to a local
// filesystem, using the asynchronous filesystem API.  It will call the
// clientCallback when it finishes.
pagespeed.ContentWriter = function (optimizedContent, clientCallback) {
  this.cancelled_ = false;
  this.clientCallback_ = clientCallback;
  this.fileSystem_ = null;
  this.optimizedContent_ = optimizedContent;
  this.keyQueue_ = [];
  for (var key in optimizedContent) {
    if (optimizedContent.hasOwnProperty(key)) {
      this.keyQueue_.push(key);
    }
  }
};

// Start the content writer.
pagespeed.ContentWriter.prototype.start = function () {
  // Create a new temporary filesystem.  On some (but not all) Chrome versions,
  // this has a "webkit" prefix.
  var requestFS = (window.requestFileSystem ||
                   window.webkitRequestFileSystem);
  // Request 10MB for starters, but we can always exceed this later because we
  // use the "unlimitedStorage" permission in our manifest.json file.
  requestFS(window.TEMPORARY, 10 * 1024 * 1024 /*10MB*/,
            // On success:
            pagespeed.withErrorHandler(this.onFileSystem_.bind(this)),
            // On failure:
            this.makeErrorHandler_("requestFileSystem", this.clientCallback_));
};

// Cancel the content writer.
pagespeed.ContentWriter.prototype.cancel = function () {
  this.cancelled_ = true;
};

// Callback for when the filesystem is successfully created.
pagespeed.ContentWriter.prototype.onFileSystem_ = function (fs) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  this.fileSystem_ = fs;
  this.writeNextFile_();
};

// Start writing the next file in the queue.
pagespeed.ContentWriter.prototype.writeNextFile_ = function () {
  if (this.cancelled_) {
    return;
  }
  // If there are no more keys in the queue, we're done.
  if (this.keyQueue_.length <= 0) {
    this.clientCallback_();
    return;
  }
  // Otherwise, create a file for the next key in the queue.  If the file
  // already exists, we'll overwrite it, which is okay because the filenames we
  // choose include content hashes.
  var key = this.keyQueue_.pop();
  var entry = this.optimizedContent_[key];
  this.fileSystem_.root.getFile(
    entry.filename, {create: true},
    // On success:
    pagespeed.withErrorHandler(this.onGotFile_.bind(this, entry)),
    // On failure:
    this.makeErrorHandler_("getFile", this.writeNextFile_.bind(this)));
};

// Callback for when a file is successfully created.
pagespeed.ContentWriter.prototype.onGotFile_ = function (entry, file) {
  if (this.cancelled_) {
    return;  // We've been cancelled so ignore the callback.
  }
  // Decode the base64 data into a byte array, which we can then append to a
  // BlobBuilder.  I don't know of any quicker way to just write base64 data
  // straight into a Blob, but thanks to V8, the below is still very fast.
  var decoded = atob(entry.content);
  delete entry.content;  // free up memory
  var size = decoded.length;
  var array = new Uint8Array(size);
  for (var index = 0; index < size; ++index) {
    array[index] = decoded.charCodeAt(index);
  }

  if (array.buffer.byteLength <= 0) {
    // Do not write this file, because it may crash WebKit's FileWriter on
    // 0-sized write. Keep the url and move to the next one.
    entry.url = file.toURL(entry.mimetype);
    this.writeNextFile_();
    return;
  }

  var writeNext = this.writeNextFile_.bind(this);
  var onWriterError = this.makeErrorHandler_("write", writeNext);
  file.createWriter(pagespeed.withErrorHandler(function (writer) {
    // Provide callbacks for when the writer finishes or errors.
    writer.onwriteend = pagespeed.withErrorHandler(function () {
      entry.url = file.toURL(entry.mimetype);
      writeNext();
    });
    writer.onerror = onWriterError;

    // Use a BlobBuilder to write the file.  In some (but not all) Chrome
    // versions, this has a "WebKit" prefix.
    var bb = new (window.BlobBuilder || window.WebKitBlobBuilder)();
    bb.append(array.buffer);
    writer.write(bb.getBlob(entry.mimetype));
  }), this.makeErrorHandler_("createWriter", writeNext));
};

// Given a string representing where the error happened, and a callback to call
// after handling the error, return an error handling function suitable to be
// passed to one of the filesystem API calls.
pagespeed.ContentWriter.prototype.makeErrorHandler_ = function (where, next) {
  return pagespeed.withErrorHandler((function (error) {
    var msg;
    switch (error.code) {
    case FileError.QUOTA_EXCEEDED_ERR:
      msg = 'QUOTA_EXCEEDED_ERR';
      break;
    case FileError.NOT_FOUND_ERR:
      msg = 'NOT_FOUND_ERR';
      break;
    case FileError.SECURITY_ERR:
      msg = 'SECURITY_ERR';
      break;
    case FileError.INVALID_MODIFICATION_ERR:
      msg = 'INVALID_MODIFICATION_ERR';
      break;
    case FileError.INVALID_STATE_ERR:
      msg = 'INVALID_STATE_ERR';
      break;
    default:
      msg = 'Unknown Error';
      break;
    };
    if (!this.cancelled_) {
      next();
    }
  }).bind(this));
};

fetchDevToolsAPI(function () {
  pagespeed.withErrorHandler(function () {

    // Connect to the extension background page.
    pagespeed.connectionPort = chrome.extension.connect();
    pagespeed.connectionPort.onMessage.addListener(
      pagespeed.withErrorHandler(pagespeed.messageHandler));

    // Register for navigation events from the inspected window.
    chromeDevTools.network.onNavigated.addListener(
      pagespeed.withErrorHandler(pagespeed.onPageNavigate));

    // The listener will disconnect when we close the devtools panel.
    // The timeline api is still experimental.
    var timeline = chromeDevTools.timeline;
    if (!timeline) {
      timeline = chrome.experimental.devtools.timeline;
    }
    timeline.onEventRecorded.addListener(
      pagespeed.withErrorHandler(pagespeed.onTimelineEvent));

    pagespeed.initializeUI();
  })();
});
