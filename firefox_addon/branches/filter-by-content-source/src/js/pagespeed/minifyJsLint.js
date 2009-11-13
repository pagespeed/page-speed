/**
 * Copyright 2009 Google Inc.
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
 * @fileoverview Runs Doug Crockford's jsmin over all JS on the page
 * and generates a score based on how much jsmin is able to compress
 * the sources. We could use a more aggressive/complex js minifier to
 * squeeze a few additional bytes from the source, but jsmin has been
 * around a long time, is very simple, and is available in many
 * different languages, so it should be possible for virtually any web
 * application to integrate it with their build process.
 *
 * For more information on jsmin, see:
 * http://www.crockford.com/javascript/jsmin.html
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

/**
 * Minimum number of bytes we expect to save when minifying JS
 * before we warn the user about it.
 * @type {number}
 */
var MIN_BYTES_SAVED = 25;

/**
 * Minimum number of bytes we expect to find on the page before we
 * give a score for the page.
 * @type {number}
 */
var MIN_UNCOMPILED_THRESHOLD = 4096;

/**
 * Name of the directory where we store minified JS.
 * @type {string}
 */
var MINIFIED_OUTPUT_DIR_NAME = 'page-speed-javascript';

/**
 * Permissions of the directory where we store minified JS.
 * @type {number}
 */
var MINIFIED_OUTPUT_DIR_PERMISSIONS = 0755;

/**
 * Write the minified version of the JS to a file on disk, so we can
 * provide a link to it in the UI, and so the user is able to get a
 * copy of the minified version.
 * @param {string} uncompiledSource The uncompiled JS.
 * @param {string} compiledSource The minified version of uncompiledSource.
 * @return {Object} The minified file on success, null on failure.
 */
var writeMinifiedFile = function(uncompiledSource, compiledSource) {
  // minifiedFile starts as a directory.  The call to minifiedFile.append()
  // below makes it a file.
  var minifiedFile = PAGESPEED.Utils.getOutputDir(MINIFIED_OUTPUT_DIR_NAME);
  if (!minifiedFile) {
    return null;
  }

  // Get the md5sum of the uncompiled source.
  var inputStream = PAGESPEED.Utils.wrapWithInputStream(uncompiledSource);
  var hash = PAGESPEED.Utils.getMd5HashForInputStream(inputStream);
  inputStream.close();

  // Create the output file.
  inputStream = PAGESPEED.Utils.wrapWithInputStream(compiledSource);
  var fileName = hash + '.js';
  minifiedFile.append(fileName);
  if (minifiedFile.exists()) {
    // If the file exists, remove it.
    minifiedFile.remove(true);
  }

  // Write the compiled source to a file.
  PAGESPEED.Utils.copyCompleteInputToOutput(
      inputStream,
      PAGESPEED.Utils.openFileForWriting(minifiedFile.path));

  return minifiedFile;
};

// Sorts the minified JS by most bytes saved.
function sortByBytesSaved(a, b) {
  var aSaved = a.savings;
  var bSaved = b.savings;
  return (aSaved > bSaved) ? -1 : (aSaved == bSaved) ? 0 : 1;
}

// Build a human-readable results string from the input data.
function buildResultString(data) {
  var resultArr = [
      'Minifying ', data.name, ' could save ',
      PAGESPEED.Utils.formatBytes(data.savings), ' (',
      PAGESPEED.Utils.formatPercent(data.savings / data.origSize),
      ' reduction).'];

  if (data.minifiedUrl) {
    resultArr = resultArr.concat(['  See ', data.minifiedUrl, '.']);
  }

  return resultArr.join('');
}

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var minifyJsLint = function(resourceAccessor) {
  // TODO: Update this rule to use |resourceAccessor|.

  var allScripts = PAGESPEED.Utils.getContentsOfAllScriptsOrStyles('script');
  if (allScripts.length == 0) {
    this.score = 'n/a';
    return;
  }

  var storage = {
    // How many bytes of javascript can be saved by minimization?
    totalPossibleSavings: 0,

    // How many bytes of javascript in the page?
    totalUncompiledBytes: 0,

    // How many external JS files?
    totalExternalFiles: 0,

    // How many files can be minimized?
    totalExternalMinifyableFiles: 0,

    aResults: [],
    aErrors: [],
    addStatistics: function(obj) { self.addStatistics_(obj); }
  };

  for (var i = 0, len = allScripts.length; i < len; i++) {
    var script = allScripts[i];
    if (!script.name || !script.content || script.content.length == 0) {
      continue;
    }
    this.addContinuation(buildMinifyJsCallback(storage, script));
  }
  var self = this;
  this.addContinuation(function() { generateOutput.call(self, storage); });
  this.doneAddingContinuations();
};

/*
 * Return a function that will minify the given script.
 * @param {Object} storage The data storage object.
 * @param {string} script The script contents.
 * @return {Function} A function that will try to minify the script.
 */
var buildMinifyJsCallback = function(storage, script) {
  return function() { doMinify(storage, script); return script.name; };
};

/*
 * Minify the given script.
 * @param {Object} storage The data storage object.
 * @param {string} script The script contents.
 */
var doMinify = function(storage, script) {
  var uncompiledSource = script.content;
  var uncompiledSourceLength = uncompiledSource.length;
  storage.totalUncompiledBytes += uncompiledSourceLength;

  var isInline = /inline block \#/.test(script.name);

  var compiledSource;
  try {
    compiledSource = JSMIN.compile(uncompiledSource);
  } catch (e) {
    storage.aErrors.push([
      'Minification of ', script.name, ' failed (',
      PAGESPEED.Utils.formatException(e), ').'
    ].join(''));
    return;
  }
  var compiledSourceLength = compiledSource.length;

  if (!isInline) {
    storage.totalExternalFiles++;
  }

  var possibleSavings = uncompiledSourceLength - compiledSourceLength;
  if (possibleSavings <= MIN_BYTES_SAVED) {
    return;
  }

  if (!isInline) {
    storage.totalExternalMinifyableFiles++;
  }

  storage.totalPossibleSavings += possibleSavings;

  var minifiedFile = writeMinifiedFile(uncompiledSource, compiledSource);

  var minifiedFileUrl;
  if (minifiedFile) {
    minifiedFileUrl = PAGESPEED.Utils.getUrlForFile(minifiedFile);

    // Make sure the temp file gets deleted when Firefox exits.
    PAGESPEED.Utils.deleteTemporaryFileOnExit(minifiedFile);
  }

  storage.aResults.push({
      name: script.name,
      origSize: uncompiledSourceLength,
      minifiedUrl: minifiedFileUrl || '',
      savings: possibleSavings});
};

/**
 * Generate the output.
 * @this PAGESPEED.LintRule
 * @param {Object} storage The data storage object.
 */
var generateOutput = function(storage) {
  if (storage.aErrors.length > 0) {
    // Tell the user about any errors we encountered.
    this.information = PAGESPEED.Utils.formatWarnings(storage.aErrors);
  }

  storage.addStatistics({
    totalPossibleSavings: storage.totalPossibleSavings,
    totalUncompiledBytes: storage.totalUncompiledBytes,
    totalExternalFiles: storage.totalExternalFiles,
    totalExternalMinifyableFiles: storage.totalExternalMinifyableFiles
  });

  if (storage.totalUncompiledBytes < MIN_UNCOMPILED_THRESHOLD) {
    this.score = 'n/a';
    return;
  }

  if (storage.aResults.length == 0 || storage.totalPossibleSavings == 0) {
    this.score = 100;
    return;
  }

  storage.aResults.sort(sortByBytesSaved);
  var aWarnings = [];
  for (var i = 0, len = storage.aResults.length; i < len; i++) {
    aWarnings.push(buildResultString(storage.aResults[i]));
  }

  var messages = PAGESPEED.Utils.formatWarnings(aWarnings);

  // Remove the path from the text of the link to the minified js, to
  // make it more readable.
  var jsDir = PAGESPEED.Utils.getOutputDir();
  if (jsDir) {
    var jsDirUrl = PAGESPEED.Utils.getUrlForFile(jsDir);
    var jsDirRegexp = new RegExp(
        ['>', jsDirUrl, '[^<]*'].join(''), 'g');
    messages = messages.replace(jsDirRegexp, '>minified version');
  }

  var percentPossibleSavings =
      storage.totalPossibleSavings / storage.totalUncompiledBytes;
  this.warnings = [
      'There is ', PAGESPEED.Utils.formatBytes(storage.totalUncompiledBytes),
      ' worth of JavaScript. Minifying could save ',
      PAGESPEED.Utils.formatBytes(storage.totalPossibleSavings), ' (',
      PAGESPEED.Utils.formatPercent(percentPossibleSavings),
      ' reduction).'].join('');

  if (!PAGESPEED.Utils.getOutputDir(MINIFIED_OUTPUT_DIR_NAME)) {
    this.warnings = [
        this.warnings,
        '<p> <b>Warning:</b> Can not save minified results because the ',
        'directory where optimized javascript is saved can not be ',
        'accessed.  Click the black triangle in the Page Speed ',
        'tab and select a different path under "Save Optimized ',
        'Files To:".'].join('');
  }

  this.score -= 2 * (percentPossibleSavings * 100);
  this.warnings += messages;
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minify JavaScript',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#MinifyJS',
    minifyJsLint,
    3.0,
    'MinifyJS'
  )
);

})();  // End closure
