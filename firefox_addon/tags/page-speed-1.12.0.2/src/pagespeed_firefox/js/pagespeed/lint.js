/**
 * Copyright 2007-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Defines the LintRule class.
 *
 * @author Kyle Scholz
 */

/** @constructor */
PAGESPEED.LintRulesImpl = function() {
  this.lintRules = [];
  this.nativeRuleResults = [];
};

/**
 * Registers a lint rule with Page Speed for execution. Registered lint rules run
 * with performance tests.
 * @param {PAGESPEED.LintRule} lintRule A lint function.
 */
PAGESPEED.LintRulesImpl.prototype.registerLintRule = function(lintRule) {
  var placed = false;
  for (var i = 0, len = this.lintRules.length; i < len; i++) {
    if (lintRule.type.precedence < this.lintRules[i].type.precedence) {
      this.lintRules.splice(i, 0, lintRule);
      placed = true;
      break;
    }
  }
  if (!placed) this.lintRules.push(lintRule);
};

/**
 * Executes all lint rules.
 * @param {Object} browserTab The browser object of the tab PageSpeed is
 *     linting.
 */
PAGESPEED.LintRulesImpl.prototype.exec = function(browserTab) {
  this.browserTab_ = browserTab;
  this.onProgress(0, 'Running Page Speed rules');
  this.completed = false;  // Set in ruleCompleted() when all rules are done.
  this.url = null; // Set in ruleCompleted() when all rules are done.
  this.rulesRemaining = this.lintRules.length;
  this.nativeRuleResults = [];
  this.score = 0;
  // TODO Note that this next line will freeze the UI while the native rules
  //      run; usually, this takes a fraction of a second, and is thus
  //      acceptable, but in the future we should be running pieces of the
  //      native rule set asynchronously, as we do with the JavaScript rules.
  try {
    var fullResults = PAGESPEED.NativeLibrary.buildLintRuleResults(
        PAGESPEED.NativeLibrary.invokeNativeLibraryAndFormatResults());
    this.nativeRuleResults = fullResults.lintRules;
    this.score = fullResults.score;
  } catch (e) {
    PS_LOG("Exception while running pagespeed library rules: " +
           PAGESPEED.Utils.formatException(e));
  }
  this.ruleCompleted();
};

/**
 * Stop execution of lint rules, if they are currently running.
 * @return {boolean} True iff the lint rules were running and were
 *     stopped, false if no rules were running.
 */
PAGESPEED.LintRulesImpl.prototype.stop = function() {
  var stopped = false;

  if (this.timeoutId_) {
    window.clearTimeout(this.timeoutId_);
    this.timeoutId_ = undefined;
    stopped = true;
  }

  // Clear all other state.
  this.browserTab_ = null;
  this.rulesRemaining = 0;
  this.completed = false;

  return stopped;
};

/**
 * Schedule the given function to run asynchronously.
 * @param {Function} fn The function to run asynchronously.
 */
PAGESPEED.LintRulesImpl.prototype.runAsync = function(fn) {
  this.timeoutId_ = window.setTimeout(
      function(f, self) {
        return function() {
          self.timeoutId_ = undefined;
          if (gBrowser.selectedBrowser != self.browserTab_) {
            // The user changed tabs. We need to abort this run.
            self.stop();
            var panel = Firebug.currentContext.getPanel('pagespeed');
            panel.initializeNode(panel);
            return;
          }
          f();
        };
      }(fn, this), 1);
};

/**
 * Updates the progress bar displayed to the user.
 * @param {number} rulesCompleted The number of rules that have
 *     completed execution.
 * @param {string?} opt_message The message to display in the progress
 *     bar, or undefined if the message should not be changed.
 */
PAGESPEED.LintRulesImpl.prototype.onProgress = function(
    rulesCompleted, opt_message) {
  var numRules = this.lintRules.length;

  var majorProgress = 100 * (rulesCompleted / numRules);
  var overallProgress = Math.round(majorProgress);

  Firebug.currentContext.getPanel('pagespeed').setProgress(
    overallProgress, opt_message);
};

/**
 * Updates the progress bar displayed to the user.
 * @param {number} rulePartialProgress The progress of the currently
 *     running rule (0..1).
 * @param {string?} opt_message The submessage to display in the progress
 *     bar, or undefined if the submessage should be cleared.
 */
PAGESPEED.LintRulesImpl.prototype.onPartialProgress = function(
    rulePartialProgress, opt_message) {
  var numRules = this.lintRules.length;

  // rulesRemaining is decremented as soon as the current rule's first
  // callback is scheduled. We need to account for this by subtracting
  // 1 from the computed number of rules completed (since we haven't
  // actually completed the currently running rule).
  var rulesCompleted =
      (this.lintRules.length - this.rulesRemaining) - 1;
  var majorProgress = 100 * (rulesCompleted / numRules);
  var minorProgress = rulePartialProgress * (100 / numRules);
  var overallProgress = Math.round(majorProgress + minorProgress);

  Firebug.currentContext.getPanel('pagespeed').setProgress(
      overallProgress, undefined, opt_message);
};

/**
 * Called each time a rule completes. If there are more rules to run, it will
 * kick off the next rule. If all rules have run, it will display the
 * performance score card.
 */
PAGESPEED.LintRulesImpl.prototype.ruleCompleted = function() {
  var nextRuleToRun = this.lintRules.length - this.rulesRemaining;

  if (this.rulesRemaining > 0) {
    // Queue the next rule to run asynchronously so it doesn't freeze the UI.
    this.runAsync((function(self, index) {
                         return function() {
                           self.onProgress(
                               index,
                               'Running ' + self.lintRules[index].name);
                           self.lintRules[index].runRule_();
                         };
                       })(this, nextRuleToRun));

    --this.rulesRemaining;
  } else if (!this.completed) {
    // Ensure that processResults() is only called once.
    this.completed = true;
    this.onProgress(this.lintRules.length, 'Done');

    var currentURI = this.browserTab_.currentURI;
    if (currentURI && currentURI.spec) {
      this.url = currentURI.spec;
    } else {
      this.url = null;
    }

    PAGESPEED.PageSpeedContext.processResults(
        Firebug.currentContext.getPanel('pagespeed'),
        this.browserTab_);

    this.browserTab_ = null;
  } else {
    // At this point, this.rulesRemaining==0 and this.completed was already
    // set to true by a previous call to this function.
    PS_LOG('In ruleCompleted(), more rules have been completed ' +
           'than were run.');
  }
};

// Set up LintRules as a Singleton
PAGESPEED.LintRules = new PAGESPEED.LintRulesImpl();

/**
 * Defines a lint rule.
 *
 * @param {string} name The name of the lint rule.
 * @param {PAGESPEED.RuleGroup} type The RuleGroup to which this rule belongs.
 * @param {string} href The url of a document that describes this rule.
 * @param {Function} lintFunction The function implementation of the rule. This
 *     function should modify the LintRule's "score" attribute and may
 *     optionally populate "warnings".
 * @param {number} weight The weight for this rule [0.0-4.0]. Higher
 *     weighted rules have more impact on the overall score, and are
 *     displayed higher on the results list.
 * @param {string} shortName A rule name short enough to be encoded as a
 *     parameter in a beacon that sends results for storage.
 * @constructor
 */
PAGESPEED.LintRule = function(
    name, type, href, lintFunction, weight, shortName) {
  this.name = name;
  this.type = type;
  this.weight = weight;
  this.href = href;
  this.lintFunction = lintFunction;
  this.shortName = shortName;

  this.warnings = '';  // Used for lint warnings that count against the score.
  this.information = '';  // Used for information messages that don't count
                          // against the score.
  this.score = 0;
};

/**
 * As a lint rule runs, it can install a function that will be
 * run after it returns.  This function is wrapped in a window.setTimeout
 * call.  This allows lint rules to yield.  this.lintFnsToRun_ will only
 * exist when the lint rule's function is running, so that calling
 * this function at any other time is an error.
 *
 * @param {Function} continuation The function that must be run before
 *    this lint rule is considered complete.
 */
PAGESPEED.LintRule.prototype.addContinuation = function(continuation) {
  if (this.numDeclaredContinuations_ > 0) {
    PS_LOG('Attempting to add a continuation for ' + this.name +
        ' after calling doneAddingContinuations.');
    return;
  }
  this.lintFnsToRun_.push(continuation);
};

/**
 * This function allows a lint rule to declare that it will not add
 * more continuations by calling LintRule.addContinuation().  Knowing
 * that all continuations are installed allows us to compute what
 * percent of functions remain to be called, which is used to update
 * the progress bar.
 */
PAGESPEED.LintRule.prototype.doneAddingContinuations = function() {
  this.numDeclaredContinuations_ = this.lintFnsToRun_.length;
};

/**
 * Set some rule defaults, so taht each rule can assume the rule's fields
 * have reasonable starting values.
 * @private
 */
PAGESPEED.LintRule.prototype.initializeRule_ = function() {
  // Reset score and messages before rule is run.
  this.score = 100;
  this.warnings = '';
  this.information = '';
};

/**
 * Executes this lint rule and, if it returns a number score, clips it
 * to the range [0..100].
 * @private
 */
PAGESPEED.LintRule.prototype.runRule_ = function() {
  var self = this;
  var lintFnArgs = arguments;

  self.initializeRule_();
  self.lintFnsToRun_ = [this.lintFunction];
  this.numDeclaredContinuations_ = -1;

  /**
   * Run a single rule function, and schedule the next one
   * with setTimeout if there is a next function.  If not,
   * clip the score and call ruleCompleted() to start the next rule.
   */
  var runOneFnAndYield = function() {
    try {
      if (!self.lintFnsToRun_.length) {
        PS_LOG('Error in lint rule runner: Imposible to call ' +
            'runOneFnAndYield() when there is no lint function to run.');
      }

      var lintFunction = self.lintFnsToRun_.shift();

      var opt_msg = lintFunction.apply(self, lintFnArgs);
      if (self.numDeclaredContinuations_ > 0) {
        // Update progress.
        var numContinuationsExecuted =
            self.numDeclaredContinuations_ - self.lintFnsToRun_.length;
        PAGESPEED.LintRules.onPartialProgress(
            numContinuationsExecuted / self.numDeclaredContinuations_,
            opt_msg);
      }

      // lintFunction may add items to self.lintFnsToRun_ by calling
      // LintRule.addContinuation().  It is not safe to expect that the
      // length at this point is the same as the length above.
      if (self.lintFnsToRun_.length) {
        PAGESPEED.LintRules.runAsync(runOneFnAndYield);
      } else {
        // All done.  Clip scores and call ruleCompleted() to start the
        // next rule.
        self.clipScore_();
        delete self.lintFnsToRun_;
        PAGESPEED.LintRules.ruleCompleted();
      }
    } catch (e){
      self.score = 'error';
      if (e.message) {
        self.information = e.message;
      } else {
        self.information =
          'Sorry, there was an error while running ' +
          'this rule. Please file a bug with these ' +
          'details: ' + PAGESPEED.Utils.formatException(e);
      }

      PAGESPEED.LintRules.ruleCompleted();
    }
  };

  runOneFnAndYield();
};

/**
 * After a rule is run, clip the score to the range [0..100]
 * and round to one decimal place.
 * @private
 */
PAGESPEED.LintRule.prototype.clipScore_ = function() {
  if (!isNaN(this.score)) {
    if (this.score < 0) {
      this.score = 0;
    } else if (this.score > 100) {
      this.score = 100;
    } else {
      // Round to a single decimal place:
      var roundedScore = Math.round(this.score * 10) / 10;
      this.score = roundedScore;
    }
  }
};

/**
 * Add statistics about the page being scored.  Statistics
 * are stored as key value pairs for each rule in the results
 * container.
 * @param {Object} stats Statistics to store, encoded like
 *     this: {'key': 'value'}.
 */
PAGESPEED.LintRule.prototype.addStatistics_ = function(stats) {
  if (!this.statistics)
    this.statistics_ = {};

  for (var key in stats) {
    // TODO: Consider throwing an error if a key is overwritten.
    this.statistics_[key] = stats[key];
  }
};

/**
 * Get statistics that were set on this rule.  Today this
 * function is trivial, but it should be used so that if we
 * want to lazily compute statistics in the future, the
 * change will be easy.
 * @return {Object} The statistics set by calls to
 *     LintRule.addStatistics_().
 */
PAGESPEED.LintRule.prototype.getStatistics = function(stats) {
  return this.statistics_ || {};
};

/**
 * Defines a lint rule group which each lint rule should pass to its ctor.
 *
 * @param {string} name A name that is displayed to the user to describe this
 *     group.
 * @param {number} precedence The precedence used to dictate the order
 *     in which the lint rule is executed and output.
 * @constructor
 */
PAGESPEED.RuleGroup = function(name, precedence) {
  this.name = name;
  this.precedence = precedence;
};

// All rules should fit into one of the following types.

// Any rule that increases the browser's ability to cache content.
PAGESPEED.CACHE_GROUP = new PAGESPEED.RuleGroup('Caching', 1);

// Any rule that reduces the number of bytes transmitted to the client.
PAGESPEED.PAYLOAD_GROUP = new PAGESPEED.RuleGroup('Response Size', 2);

// Any rule that reduces the number of bytes sent by the client.
PAGESPEED.REQUEST_GROUP = new PAGESPEED.RuleGroup('Request Size', 3);

// Any rule that reduces the Round Trip Time (RTT).
PAGESPEED.RTT_GROUP = new PAGESPEED.RuleGroup('Round Trip Time', 4);

// Any rule that reduces rendering time on the client.
PAGESPEED.RENDERING_GROUP = new PAGESPEED.RuleGroup('Rendering', 5);

// Any rule that is very likely to have false positives or false negatives
// should be added to this group. These rules do not affect the overall score
// and only show up if they detect a violation.
PAGESPEED.NONSCORING_GROUP = new PAGESPEED.RuleGroup(
    'Non-Scoring Suggestions', 6);

if (PAGESPEED.PageSpeedContext) {
  PAGESPEED.PageSpeedContext.callbacks.watchWindow.addCallback(
      /**
       * When a window is loaded, reset the flag that indicates that the
       * lint rules have been run on the current page.
       */
      function() {
        PAGESPEED.LintRules.completed = false;
      });
}
