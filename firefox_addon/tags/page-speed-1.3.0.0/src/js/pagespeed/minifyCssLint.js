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
 * @fileoverview Runs Stoyan Stefanov's Javascript port of cssmin over all CSS
 * on the page and generates a score based on how much cssmin is able to
 * compress the sources.
 *
 * For more information on cssmin, see:
 * http://www.phpied.com/cssmin-js/
 *
 * @author Matthew Steele
 */

(function() {  // Begin closure

/**
 * Minimum number of bytes we expect to save when minifying CSS before we warn
 * the user about it.
 * @type {number}
 */
var MIN_BYTES_SAVED = 25;

/**
 * Minimum number of bytes we expect to find on the page before we give a score
 * for the page.
 * @type {number}
 */
var MIN_UNCOMPILED_THRESHOLD = 4096;

/**
 * Name of the directory where we store minified CSS.
 * @type {string}
 */
var MINIFIED_OUTPUT_DIR_NAME = 'page-speed-css';

/**
 * Permissions of the directory where we store minified CSS.
 * @type {number}
 */
var MINIFIED_OUTPUT_DIR_PERMISSIONS = 0755;

/**
 * Write the minified version of the CSS to a file on disk, so we can provide a
 * link to it in the UI, and so the user is able to get a copy of the minified
 * version.
 * @param {string} uncompiledSource The uncompiled CSS.
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
  var fileName = hash + '.css';
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

// Sorts the minified CSS by most bytes saved.
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
 */
var minifyCssLint = function() {
  var allStyles = PAGESPEED.Utils.getContentsOfAllScriptsOrStyles('style');
  if (allStyles.length == 0) {
    this.score = 'n/a';
    return;
  }

  var storage = {
    // How many bytes of CSS can be saved by minimization?
    totalPossibleSavings: 0,

    // How many bytes of CSS in the page?
    totalUncompiledBytes: 0,

    // How many external CSS files?
    totalExternalFiles: 0,

    // How many files can be minimized?
    totalExternalMinifyableFiles: 0,

    aResults: [],
    aErrors: [],
    addStatistics: function(obj) { self.addStatistics_(obj); }
  };

  for (var i = 0, len = allStyles.length; i < len; i++) {
    var style = allStyles[i];
    if (!style.name || !style.content || style.content.length == 0) {
      continue;
    }
    this.addContinuation(buildMinifyCssCallback(storage, style));
  }
  var self = this;
  this.addContinuation(function() { generateOutput.call(self, storage); });
  this.doneAddingContinuations();
};

/*
 * Return a function that will minify the given stylesheet.
 * @param {Object} storage The data storage object.
 * @param {string} style The stylesheet contents.
 * @return {Function} A function that will try to minify the stylesheet.
 */
var buildMinifyCssCallback = function(storage, style) {
  return function() { doMinify(storage, style); return style.name; };
};

/*
 * Minify the given stylesheet.
 * @param {Object} storage The data storage object.
 * @param {string} style The stylesheet contents.
 */
var doMinify = function(storage, style) {
  var uncompiledSource = style.content;
  var uncompiledSourceLength = uncompiledSource.length;
  storage.totalUncompiledBytes += uncompiledSourceLength;

  var isInline = /inline block \#/.test(style.name);

  var compiledSource;
  try {
    compiledSource = YAHOO.compressor.cssmin(uncompiledSource, 0);
  } catch (e) {
    storage.aErrors.push([
      'Minification of ', style.name, ' failed (',
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
      name: style.name,
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

  // Remove the path from the text of the link to the minified CSS, to
  // make it more readable.
  var cssDir = PAGESPEED.Utils.getOutputDir();
  if (cssDir) {
    var cssDirUrl = PAGESPEED.Utils.getUrlForFile(cssDir);
    var cssDirRegexp = new RegExp(
        ['>', cssDirUrl, '[^<]*'].join(''), 'g');
    messages = messages.replace(cssDirRegexp, '>minified version');
  }

  var percentPossibleSavings =
      storage.totalPossibleSavings / storage.totalUncompiledBytes;
  this.warnings = [
      'There is ', PAGESPEED.Utils.formatBytes(storage.totalUncompiledBytes),
      ' worth of CSS. Minifying could save ',
      PAGESPEED.Utils.formatBytes(storage.totalPossibleSavings), ' (',
      PAGESPEED.Utils.formatPercent(percentPossibleSavings),
      ' reduction).'].join('');

  if (!PAGESPEED.Utils.getOutputDir(MINIFIED_OUTPUT_DIR_NAME)) {
    this.warnings = [
        this.warnings,
        '<p> <b>Warning:</b> Can not save minified results because the ',
        'directory where optimized CSS is saved can not be ',
        'accessed.  Click the black triangle in the Page Speed ',
        'tab and select a different path under "Save Optimized ',
        'Files To:".'].join('');
  }

  this.score -= 2 * (percentPossibleSavings * 100);
  this.warnings += messages;
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minify CSS',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#MinifyCSS',
    minifyCssLint,
    3.0,
    'MinifyCSS'
  )
);

})();  // End closure
