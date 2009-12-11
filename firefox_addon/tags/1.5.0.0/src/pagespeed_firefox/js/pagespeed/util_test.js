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
 * @fileoverview Tests features of util.js.
 *
 * @author Kyle Scholz
 */

// First stub out all necessary objects here.

Components = {};
Components.classes = {};
Components.classes['@mozilla.org/preferences-service;1'] = {};
Components.interfaces = {};
Components.interfaces.nsIPrefBranch = {
  getCharPref: function(x) {return 'PREF';}
};
Components.classes['@mozilla.org/preferences-service;1'].getService =
  function(x) {return Components.interfaces.nsIPrefBranch;};

/**
 * Asserts that two arrays are equal.
 * @param {Array} a First array.
 * @param {Array} b Second array.
 */
function assertArrayEquals(a, b) {
  assertEquals(a.length, b.length);
  for (var i = 0; i < a.length; i++) {
    assertEquals(a[i], b[i]);
  }
}

/**
 * Count the number of properties in an object.
 * @param {Object} obj The object whose properties need to be counted.
 * @return {number} The number of properties of obj.
 */
function countNumProperties(obj) {
  var result = 0;
  for (var x in obj) {
    result++;
  }
  return result;
}

// scanningRegexMatch

function gethtmlWhitespaceMatches(str) {
  return PAGESPEED.Utils.scanningRegexMatch(PAGESPEED.Utils.HTML_WHITESPACE,
                                        str,
                                        [1, 2, 3]);
}

/**
 * This function compares two strings.  Strings are broken
 * into lines, and only the lines that differ are shown.
 * This makes differences in long strings much easier to
 * see.
 * @param {string} description Describes what is being tested.
 * @param {string} expected The expected result of a test.
 * @param {string} actual The actual result of a test.
 */
function assertStringsEq(description, expected, actual) {
  var LINE_LENGTH = 80;
  if (expected !== actual) {
    var len = Math.max(expected.length, actual.length);

    dump('\n\nDiff found: \n');
    for (var i = 0; i < len; i += LINE_LENGTH) {
      var expectedPart = expected.substr(i, LINE_LENGTH);
      var actualPart = actual.substr(i, LINE_LENGTH);

      if (expectedPart !== actualPart) {
        dump('Expect: ' + expectedPart + '\n');
        dump('Actual: ' + actualPart + '\n');
      }
    }
  }
  assertEquals(description, expected, actual);
}

function testHtmlWhitespaceRegexBeginningString() {
  var whitespaceMatches = gethtmlWhitespaceMatches('  <p>Text </p>');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(2, whitespace.length);
}

function testHtmlWhitespaceRegexTabbed() {
  var whitespaceMatches = gethtmlWhitespaceMatches('\t<p>Text </p>');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(1, whitespace.length);
}

function testHtmlWhitespaceRegexEndString() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p>Text </p>  ');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(2, whitespace.length);
}

function testHtmlWhitespaceRegexBetweenTags() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p>Text</p> <p></p>');
  assertEquals(0, whitespaceMatches.length);
}

function testHtmlWhitespaceRegexMultipleBetweenTags() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p>Text</p>  <p></p>');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(1, whitespace.length);
}

function testHtmlWhitespaceRegexAttributeSpaces() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p  class="a">Text</p>');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(1, whitespace.length);
}

function testHtmlWhitespaceRegexNewLineAndSpace() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p>\n </p>');
  var whitespace = whitespaceMatches.join('');
  assertEquals(1, whitespaceMatches.length);
  assertEquals(1, whitespace.length);
}

function testHtmlWhitespaceRegexAllowTrailingNewLine() {
  var whitespaceMatches = gethtmlWhitespaceMatches('<p></p>\n');
  assertEquals(0, whitespaceMatches.length);
}

// getDomainFromUrl

function testGetDomainFromUrl() {
  assertEquals('', PAGESPEED.Utils.getDomainFromUrl());
  assertEquals('', PAGESPEED.Utils.getDomainFromUrl(''));
  assertEquals('s', PAGESPEED.Utils.getDomainFromUrl('http://s/'));
  assertEquals('www.example.com',
               PAGESPEED.Utils.getDomainFromUrl('http://www.example.com'));
  assertEquals('www.example.com',
               PAGESPEED.Utils.getDomainFromUrl(
                   'http://www.example.com/a/b.png'));
  assertEquals('www.example.co.uk',
               PAGESPEED.Utils.getDomainFromUrl('http://www.example.co.uk/'));
  assertEquals('mail.example.com',
               PAGESPEED.Utils.getDomainFromUrl('https://mail.example.com/'));
  assertEquals('www.foo.com',
               PAGESPEED.Utils.getDomainFromUrl(
                 'http://www.foo.com/?url=http://www.bar.com'));
  assertEquals('www.foo.com',
               PAGESPEED.Utils.getDomainFromUrl(
                 'http://www.foo.com/?url=http%3A//www.bar.com'));
}

// getPathFromUrl

function testGetPathFromUrl() {
  assertEquals('', PAGESPEED.Utils.getPathFromUrl());
  assertEquals('', PAGESPEED.Utils.getPathFromUrl(''));
  assertEquals('', PAGESPEED.Utils.getPathFromUrl('http://s/'));
  assertEquals('', PAGESPEED.Utils.getPathFromUrl('http://www.example.com'));
  assertEquals('a/b.png',
               PAGESPEED.Utils.getPathFromUrl(
                   'http://www.example.com/a/b.png'));
  assertEquals('a',
               PAGESPEED.Utils.getPathFromUrl('http://www.example.co.uk/a'));
  assertEquals('x_y_z',
               PAGESPEED.Utils.getPathFromUrl(
                   'https://mail.example.com/x_y_z'));
  assertEquals('?url=http://www.bar.com',
               PAGESPEED.Utils.getPathFromUrl(
                 'http://www.foo.com/?url=http://www.bar.com'));
  assertEquals('?url=http%3A//www.bar.com',
               PAGESPEED.Utils.getPathFromUrl(
                 'http://www.foo.com/?url=http%3A//www.bar.com'));
}

// htmlEscape

function testHtmlEscape() {
  assertEquals('', PAGESPEED.Utils.htmlEscape());
  assertEquals('&lt;&gt;&amp;&lt;&gt;&amp;',
               PAGESPEED.Utils.htmlEscape('<>&<>&'));
}

// getContentsOfAllTags

function testGetContentsOfAllTags() {
  assertArrayEquals(['CONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<html><head></head><body>CONTENT</body></html>',
                      'body'));
  assertArrayEquals(['CONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<html><head></head><body\n>CONTENT</body></html>',
                      'body'));
  assertArrayEquals(['CONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<html><head></head><body>CONTENT',
                      'body'));
  assertArrayEquals(['CONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<body foo="bar">CONTENT</body>',
                      'body'));
  assertArrayEquals(['CONTENT<a></a>CONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<body>CONTENT<a></a>CONTENT</body>',
                      'body'));
  assertArrayEquals(['CONTENT\nCONTENT'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<body>CONTENT\nCONTENT</body>',
                      'body'));
  assertArrayEquals(['CONTENT1', 'CONTENT2'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<span>CONTENT1</span><span>CONTENT2</span>',
                      'span'));
  assertArrayEquals(['<span>CONTENT</span>'],
                    PAGESPEED.Utils.getContentsOfAllTags(
                      '<div><span>CONTENT</span></div>',
                      'div'));
}

// formatWarnings

function testFormatWarnings() {
  assertEquals('', PAGESPEED.Utils.formatWarnings([]));

  assertEquals('<ul><li>a</li><li>b</li></ul>',
               PAGESPEED.Utils.formatWarnings(['a', 'b']));

  assertEquals('<ul><li>a</li></ul>',
               PAGESPEED.Utils.formatWarnings(['a', '']));

  assertEquals('<ul><li>&lt; <a href="http://www.foo.com" ' +
               'onclick="document.openLink(this);' +
               'return false;">' +
               'http://www.foo.com</a> &gt;</li></ul>',
               PAGESPEED.Utils.formatWarnings(['< http://www.foo.com >']));

  assertEquals('<ul><li><a href="http://www.foo.com?a=1&amp;b=2" ' +
               'onclick="document.openLink(this);' +
               'return false;">' +
               'http://www.foo.com?a=1&amp;b=2</a></li></ul>',
               PAGESPEED.Utils.formatWarnings(['http://www.foo.com?a=1&b=2']));

  assertEquals('<ul><li>http://www.foo.com?a=1&b=2</li></ul>',
               PAGESPEED.Utils.formatWarnings(['http://www.foo.com?a=1&b=2'],
                                              true));
}

// formatBytes

function testFormatBytes() {
  assertEquals('abc', PAGESPEED.Utils.formatBytes('abc').valueOf());
  assertEquals('123 bytes', PAGESPEED.Utils.formatBytes('123'));
  assertEquals('123 bytes', PAGESPEED.Utils.formatBytes(123));
  assertEquals('1.2kB', PAGESPEED.Utils.formatBytes('1234'));
  assertEquals('1.2kB', PAGESPEED.Utils.formatBytes(1234));
  assertEquals('1.18MB', PAGESPEED.Utils.formatBytes('1234567'));
  assertEquals('1.18MB', PAGESPEED.Utils.formatBytes(1234567));
}

// formatNumber

function testFormatNumber() {
  assertEquals('abc', PAGESPEED.Utils.formatNumber('abc').valueOf());
  assertEquals('123', PAGESPEED.Utils.formatNumber(123));
  assertEquals('1,234', PAGESPEED.Utils.formatNumber(1234));
}

// formatPercent

function testFormatPercent() {
  assertEquals('undefined', PAGESPEED.Utils.formatPercent());
  assertEquals('abc', PAGESPEED.Utils.formatPercent('abc'));
  assertEquals('0%', PAGESPEED.Utils.formatPercent(0));
  assertEquals('5%', PAGESPEED.Utils.formatPercent(0.05));
  assertEquals('5%', PAGESPEED.Utils.formatPercent(0.0500));
  assertEquals('5.1%', PAGESPEED.Utils.formatPercent(0.051));
  assertEquals('5.1%', PAGESPEED.Utils.formatPercent(0.0512345));
  assertEquals('5.2%', PAGESPEED.Utils.formatPercent(0.0519));
  assertEquals('50%', PAGESPEED.Utils.formatPercent(0.5));
  assertEquals('150%', PAGESPEED.Utils.formatPercent(1.5));
}

// isCompressed

function testIsCompressed_Gzip() {
  var headers = {'Content-Encoding': 'gzip'};
  assertTrue(PAGESPEED.Utils.isCompressed(headers));
}

function testIsCompressed_Deflate() {
  var headers = {'Content-Encoding': 'deflate'};
  assertTrue(PAGESPEED.Utils.isCompressed(headers));
}

function testIsCompressed_GzipSdchWithoutSpace() {
  var headers = {'Content-Encoding': 'gzip,sdch'};
  assertTrue(PAGESPEED.Utils.isCompressed(headers));
}

function testIsCompressed_SdchDeflateWithSpace() {
  var headers = {'Content-Encoding': 'sdch, deflate'};
  assertTrue(PAGESPEED.Utils.isCompressed(headers));
}

function testIsCompressed_Sdch() {
  var headers = {'Content-Encoding': 'sdch'};
  assertFalse(PAGESPEED.Utils.isCompressed(headers));
}

function testIsCompressed_Missing() {
  var headers = {};
  assertFalse(PAGESPEED.Utils.isCompressed(headers));
}

// cleanUpJsdScriptSource

// Verify that an already cleaned up src string doesn't get modified.
function testCleanUpJsdScriptSource_AlreadyCleanedUp() {
  var cleanSrc = 'function () {}';
  assertEquals(cleanSrc, PAGESPEED.Utils.cleanUpJsdScriptSource(cleanSrc));
}

function testCleanUpJsdScriptSource_SingleLine() {
  var origSrc = '    function () {}';
  var cleanedSrc = 'function () {}';
  assertEquals(cleanedSrc, PAGESPEED.Utils.cleanUpJsdScriptSource(origSrc));
}

function testCleanUpJsdScriptSource_MultiLine() {
  var origSrc = '    function () {\n        doSomething();\n    }';
  var cleanedSrc = 'function () {\n    doSomething();\n}';
  assertEquals(cleanedSrc, PAGESPEED.Utils.cleanUpJsdScriptSource(origSrc));
}

// copyAllPrimitiveProperties

// Copying an empty object should not add any new properties.
function testCopyAllPrimitiveProperties_EmptyObj() {
  var result = {};
  PAGESPEED.Utils.copyAllPrimitiveProperties(result, {});
  assertEquals(0, countNumProperties(result));

  PAGESPEED.Utils.copyAllPrimitiveProperties(result, {}, 'prefix');
  assertEquals(0, countNumProperties(result));
}

// Don't copy non-basic types.
function testCopyAllPrimitiveProperties_ExcludeNonBasicTypes() {
  var dontCopyTheseTypes = {
    'object': {'foo': 'bar'},
    'function': function(x) { return x + 1; }
  };
  var num = 1.2345;
  var result = {'num': num};

  PAGESPEED.Utils.copyAllPrimitiveProperties(result, {});
  assertEquals(1, countNumProperties(result));
  assertEquals(num, result['num']);
}

// Do copy basic types.
function testCopyAllPrimitiveProperties_CopyBasicTypes() {
  var copyTheseTypes = {
    'string': 'A string...',
    'number': 1.234,
    'bool': true,
    'is_null': null,
    'is_undefined': undefined
  };
  var result = {};

  PAGESPEED.Utils.copyAllPrimitiveProperties(result, copyTheseTypes);

  for (var x in copyTheseTypes) {
    assertEquals(copyTheseTypes[x], result[x]);
  }
  assertEquals(5, countNumProperties(result));
}

// Do overwrite basic types with the same property name.
function testCopyAllPrimitiveProperties_OverwriteNonBasicTypes() {
  var copyTheseTypes = {
    'string': 'A string...',
    'number': 1.234,
    'bool': true,
    'null': null,
    'undefined': undefined
  };

  var result = {
    '_string': 'Overwrite me.',
    '_bool': false,
    '_null': null
  };

  PAGESPEED.Utils.copyAllPrimitiveProperties(result, copyTheseTypes, '_');
  for (var x in copyTheseTypes) {
    assertEquals(copyTheseTypes[x], result['_' + x]);
  }

  assertEquals(5, countNumProperties(result));
}

// Tests the getFunctionSize method with a basic function.
function testGetFunctionSizeBasic() {
  testString = ['  function x() {',
                '    bar();',
                '    bas();',
                '  }'].join('\n');
  assertEquals(testString.length, PAGESPEED.Utils.getFunctionSize(testString));
}

// Tests the getFunctionSize method with nested functions.
function testGetFunctionSizeWithNestedFunctions() {
  functionParts = new Array();
  functionParts[0] =            // counted
      '  function x() {\n' +
      '    bar();\n' +
      '    ';
  functionParts[1] =            // not counted
          'function x() {\n' +
      '      function y() {\n' +
      '        z();\n' +
      '      }\n' +
      '    }';
  functionParts[2] =            // counted
           '\n' +
      '    bas();\n' +
      '  }';
  var expectedLength = functionParts[0].length + functionParts[2].length;
  assertEquals(expectedLength,
               PAGESPEED.Utils.getFunctionSize(functionParts.join('')));
}

// Tests the getFunctionSize with { } in the string
function testGetFunctionSizeWithBraces() {
  functionParts = new Array();
  functionParts[0] =            // counted
      'function y() {\n' +
      '  bar(\'some random string with some { and some }\');\n' +
      '  bar(\'in it and also the word function \');\n' +
      '  ';
  functionParts[1] =            // not counted
        'function x() {\n' +
      '    alert("again the same random string with { and } }}}} ");\n' +
      '  }';
  functionParts[2] =            // counted
         '\n' +
      '  bas();\n' +
      '}';
  var expectedLength = functionParts[0].length + functionParts[2].length;
  assertEquals(expectedLength,
               PAGESPEED.Utils.getFunctionSize(functionParts.join('')));
}

// Tests the getFunctionSize with a nested function in comments.
function testGetFunctionSizeWithNestedFunctionInComment() {
  testString = ['function z() {',
                '  // commented function () { here }',
                '}'].join('\n');
  assertEquals(testString.length, PAGESPEED.Utils.getFunctionSize(testString));
}

// Tests the getFunctionSize with a nested function in comments.
function testGetFunctionSizeWithNestedFunctionInRegex() {
  testString = ['function z() {',
                '  /regex around function () { here }/',
                '}'].join('\n');
  assertEquals(testString.length, PAGESPEED.Utils.getFunctionSize(testString));
}

// Tests the getFunctionSize with a nested function in comments.
function testGetFunctionSizeWithNestedFunctionInBlockComment() {
  testString = ['function z() {',
                '  /* commented function () { here }*/',
                '}'].join('\n');
  assertEquals(testString.length, PAGESPEED.Utils.getFunctionSize(testString));
}

// Tests the getFunctionSize with a nested function in comments.
function testGetFunctionSizeWithNestedFunctionInString() {
  testString = ['function z() {',
                '  "quoted function () { here }"',
                '}'].join('\n');
  assertEquals(testString.length, PAGESPEED.Utils.getFunctionSize(testString));
}

// Tests the getFunctionSize method with a variable with the word
// function as a prefix.
function testGetFunctionSizeWithVarWithFunctionPrefix() {
  functionParts = new Array();
  functionParts[0] =
      'function y() {\n' +
      '  bar(\'some random string with some { and some }\');\n' +
      '  var functionFoo;\n' +
      '  bar(\'in it and also the word function \');\n' +
      '  '
  functionParts[1] =
        'function x() {\n' +
      '    alert("again the same random string with { and } }}}} ");\n' +
      '  }';
  functionParts[2] =
         '\n' +
      '  bas();\n' +
      '}';
  var expectedLength = functionParts[0].length + functionParts[2].length;
  assertEquals(expectedLength,
               PAGESPEED.Utils.getFunctionSize(functionParts.join('')));
}

// Tests the getFunctionSize method with a variable with the word
// function as a suffix.
function testGetFunctionSizeWithVarWithFunctionSuffix() {
  functionParts = new Array();
  functionParts[0] =
      'function y() {\n' +
      '  bar(\'some random string with some { and some }\');\n' +
      '  var my_function;\n' +
      '  bar(\'in it and also the word function \');\n' +
      '  '
  functionParts[1] =
        'function x() {\n' +
      '    alert("again the same random string with { and } }}}} ");\n' +
      '  }';
  functionParts[2] =
         '\n' +
      '  bas();\n' +
      '}';
  var expectedLength = functionParts[0].length + functionParts[2].length;
  assertEquals(expectedLength,
               PAGESPEED.Utils.getFunctionSize(functionParts.join('')));
}

// Tests to see if findNestedFunctionLength returns zero with no match
function testFindNestedFunctionLengthNoMatch() {
  testString = 'not it';
  assertEquals(0, PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if findNestedFunctionLength matches an empty function
function testFindNestedFunctionLengthEmpty() {
  testString = ' function(){}';
  assertEquals(testString.length - 1,
               PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if findNestedFunctionLength matches an empty function
function testFindNestedFunctionLengthDoublyNested() {
  testString = ' function(){ function {} }';
  assertEquals(testString.length - 1,
               PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if findNestedFunctionLength skips a commented curly
function testFindNestedFunctionLengthCommentedCurly() {
  testString = ' function(){ /*}*/ }1234';
  assertEquals(testString.length - 1 - 4,
               PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if findNestedFunctionLength skips a quoted curly
function testFindNestedFunctionLengthQuotedCurly() {
  testString = ' function(){ "}" }1234';
  assertEquals(testString.length - 1 - 4,
               PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if findNestedFunctionLength skips a regex curly
function testFindNestedFunctionLengthRegexCurly() {
  testString = ' function(){ /}/ }1234';
  assertEquals(testString.length - 1 - 4,
               PAGESPEED.Utils.findNestedFunctionLength(testString, 1));
}

// Tests to see if zero is returned on no match.
function testFindStringLiteralLengthNoMatch() {
  testString = 'X"string does not start a beginning."\n';
  assertEquals(0, PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Tests the findStringLiteralLength method with a single quoted string
function testFindStringLiteralLengthSingleQuotedString() {
  testStringLiteral = '\'single\'';
  testString = 'abc' + testStringLiteral + '12345';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 3));
}

// Tests the findStringLiteralLength method with a double quoted string
function testFindStringLiteralLengthDoubleQuotedString() {
  testStringLiteral = '"double"';
  testString = testStringLiteral + '12345';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Tests the findStringLiteralLength method for a double quoted string
//  with a single quoted string embedded in it
function testFindStringLiteralLengthSingleQuotedWithinDoubleQuoted() {
  testStringLiteral = '"\'single-quoted\'"';
  testString = testStringLiteral + ' string within double-quoted string';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Tests the findStringLiteralLength method for a single quoted string
// with a double quoted string embedded in it
function testFindStringEndDoubleQuotedWithinSingleQuoted() {
  testStringLiteral = '"\'single-quoted\'"';
  testString = testStringLiteral + ' string within double-quoted string';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Tests the findStringLiteralLength method with a double quoted string
// with an escaped double quote in it
function testFindStringEndWithEscapedDoubleQuote() {
  testStringLiteral = '"double-quoted\\""';
  testString = testStringLiteral + ' string with an escaped string in it';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Tests the findStringLiteralLength method with a single quoted string
// with an escaped single quote in it
function testFindStringEndWithEscapedSingleQuote() {
  testStringLiteral = '\'single-quoted\\\'\'';
  testString = testStringLiteral + ' string with an escaped string in it';
  assertEquals(testStringLiteral.length,
               PAGESPEED.Utils.findStringLiteralLength(testString, 0));
}

// Test to see if zero is returned on no match.
function testFindCommentLengthNoMatch() {
  testString = 'X//Comment starts after first character\n';
  assertEquals(0, PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Test to see if the end of comments are found correctly.
function testFindCommentLengthSingleLineComment() {
  testComment = '// single line comment\n';
  testString = testComment + '//foo\n';
  assertEquals(testComment.length,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Test to see if second comment is found correctly.
function testFindCommentLengthSingleLineCommentSecondComment() {
  testComment = '// single line comment\n';
  testString = '//foo\n' + testComment;
  startIndex = testString.length - testComment.length;
  assertEquals(testComment.length,
               PAGESPEED.Utils.findCommentLength(testString, startIndex));
}

// Test to see if an empty comment length is found correctly.
function testFindCommentLengthEmptySingleLineComment() {
  testString = '//\n// foo\n';
  assertEquals(3, PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Tests the findCommentLength method for a single-line comment.
function testFindCommentLengthLineCommentExtraSlashes() {
  testString = '//////////////////';
  assertEquals(testString.length,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Tests the findCommentLength method for a block comment
function testFindCommentLengthBlockComment() {
  testString = 'abcd/* block comment \nsecond line of block comment */123';
  assertEquals(testString.length - 4 - 3,
               PAGESPEED.Utils.findCommentLength(testString, 4));
}

// Tests the findCommentLength method for an empty block comment
function testFindCommentLengthEmptyBlockComment() {
  testString = '/**/\n this = that;';
  assertEquals(4, PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Tests the findCommentLength method for a block comment with an extra slash.
function testFindCommentLengthBlockCommentExtraSlash() {
  testString = '/** /*/123456';
  assertEquals(testString.length - 6,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Tests the findCommentLength method for a block comment without an end.
function testFindCommentLengthBlockCommentMissingEnd() {
  testString = '/**********';
  assertEquals(testString.length,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}

// Tests the findCommentLength method for a block comment
// with a single line comment in it
function testFindCommentLengthBlockCommentWithNestedSingleLineComment() {
  testString = '/* block comment \n with a single line // comment in it \n*/';
  assertEquals(testString.length,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}


// Tests the findCommentLength method with a single line method with
// a block comment in it
function testFindCommentLengthSingleLineCommentWithNestedBlockComment() {
  testComment = '// single line comment with a /* block */ in it \n';
  testString = testComment + '//foo\n';
  assertEquals(testComment.length,
               PAGESPEED.Utils.findCommentLength(testString, 0));
}

function testGetResponseCode() {
  var realGetCacheEntry = PAGESPEED.Utils.getCacheEntry;
  var realGetResourceProperty = PAGESPEED.Utils.getResourceProperty;
  var realSetResourceProperty = PAGESPEED.Utils.setResourceProperty;

  var mockStatusLine;
  var MockDescriptor = {
    getMetaDataElement: function(name) {
      return mockStatusLine + '\nHeader1: foo\nHeader2: value\n';
    },
    close: function() {}
  };
  PAGESPEED.Utils.getCacheEntry = function() { return MockDescriptor; };
  PAGESPEED.Utils.getResourceProperty = function() { return null; };
  PAGESPEED.Utils.setResourceProperty = function() { return null; };

  mockStatusLine = 'HTTP/1.1 200 OK';
  assertEquals(200, PAGESPEED.Utils.getResponseCode('http://no/such/url'));
  mockStatusLine = 'HTTP/1.1 404 Not Found';
  assertEquals(404, PAGESPEED.Utils.getResponseCode('http://no/such/url'));
  mockStatusLine = 'HTTP/1.1 301 Moved Permanently';
  assertEquals(301, PAGESPEED.Utils.getResponseCode('http://no/such/url'));
  mockStatusLine = 'HTTP/1.1 302 Moved Temporarily';
  assertEquals(302, PAGESPEED.Utils.getResponseCode('http://no/such/url'));
  mockStatusLine = 'HTTP/1.0 204 No Content';
  assertEquals(204, PAGESPEED.Utils.getResponseCode('http://no/such/url'));

  PAGESPEED.Utils.getCacheEntry = realGetCacheEntry;
  PAGESPEED.Utils.getResourceProperty = realGetResourceProperty;
  PAGESPEED.Utils.setResourceProperty = realSetResourceProperty;
}

function testLinkify() {
  var stringWithUrls =
    ['http://123.45.67.89:80/hellohellohellohellohellohellohellohello',
     'hellohellotruncated another link https://foo.com/bar and ',
     'file://foo.com/bar finally http://foo.com/ and http://bar.com.'].join('');
  var linkified = PAGESPEED.Utils.linkify(stringWithUrls);
  assertNotEquals(stringWithUrls, linkified);

  assertEquals(
      ['<a href="http://123.45.67.89:80/',
       'hellohellohellohellohellohellohellohellohellohellotruncated" ',
       'onclick="document.openLink(this);return false;">',
       '/hellohellohellohellohellohellohellohellohellohell...</a> ',
       'another link <a href="https://foo.com/bar" ',
       'onclick="document.openLink(this);return false;">',
       'https://foo.com/bar</a> and ',
       '<a href="file://foo.com/bar" ',
       'onclick="document.openLink(this);return false;">file://foo.com/bar</a>',
       ' finally <a href="http://foo.com" ',
       'onclick="document.openLink(this);return false;">',
       'http://foo.com</a> and ',
       '<a href="http://bar.com" ',
       'onclick="document.openLink(this);return false;">http://bar.com</a>.'
       ].join(''), linkified);

  var stringWithUrls =
    ['Hello there http://foo.com/asdf?q=hi and here is another link ',
     'http://123.45.67.89:80/hellohellohellohellohellohellohellohello',
     'hellohellotruncated another link https://foo.com/bar and ',
     'file://foo.com/bar finally http://foo.com/ and http://bar.com.'].join('');
  var linkified = PAGESPEED.Utils.linkify(stringWithUrls);
  assertNotEquals(stringWithUrls, linkified);

  assertEquals(
      ['Hello there <a href="http://foo.com/asdf?q=hi" ',
       'onclick="document.openLink(this);return false;">',
       'http://foo.com/asdf?q=hi</a> ',
       'and here is another link <a href="http://123.45.67.89:80/',
       'hellohellohellohellohellohellohellohellohellohellotruncated" ',
       'onclick="document.openLink(this);return false;">',
       '/hellohellohellohellohellohellohellohellohellohell...</a> ',
       'another link <a href="https://foo.com/bar" ',
       'onclick="document.openLink(this);return false;">',
       'https://foo.com/bar</a> and ',
       '<a href="file://foo.com/bar" ',
       'onclick="document.openLink(this);return false;">file://foo.com/bar</a>',
       ' finally <a href="http://foo.com" ',
       'onclick="document.openLink(this);return false;">',
       'http://foo.com</a> and ',
       '<a href="http://bar.com" ',
       'onclick="document.openLink(this);return false;">http://bar.com</a>.'
       ].join(''), linkified);
}

function testLinkifyJustLinks() {
  assertEquals(
      'Just a host:',
      ['<a href="http://www.example.com" ',
       'onclick="document.openLink(this);',
       'return false;">http://www.example.com</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://www.example.com'));

  assertEquals(
      'An IP address:',
      ['<a href="http://127.0.0.1" ',
       'onclick="document.openLink(this);',
       'return false;">http://127.0.0.1</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://127.0.0.1'));

  assertEquals(
      'An IP address with a port:',
      ['<a href="http://127.0.0.1:40" ',
       'onclick="document.openLink(this);',
       'return false;">http://127.0.0.1:40</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://127.0.0.1:40'));

  assertEquals(
      'An IP address with a path:',
      ['<a href="http://123.45.67.89/foo" ',
       'onclick="document.openLink(this);',
       'return false;">http://123.45.67.89/foo</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://123.45.67.89/foo'));

  assertEquals(
      'An IP address with a port and path:',
      ['<a href="http://123.45.67.89:80/foo" ',
       'onclick="document.openLink(this);',
       'return false;">http://123.45.67.89:80/foo</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://123.45.67.89:80/foo'));

  assertEquals(
      'Host with slash:',
      ['<a href="http://www.example.com" ',
       'onclick="document.openLink(this);',
       'return false;">http://www.example.com</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('http://www.example.com/'));

   assertEquals(
       'Host and port:',
       ['<a href="http://www.example.com:50" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com:50</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com:50'));

   assertEquals(
       'Host, port, slash:',
       ['<a href="http://www.example.com:50" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com:50</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com:50/'));

   assertEquals(
       'Host and path:',
       ['<a href="http://www.example.com/foo/bar" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com/foo/bar</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com/foo/bar'));

   assertEquals(
       'Host, port, path:',
       ['<a href="http://www.example.com:50/foo/bar" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com:50/foo/bar</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com:50/foo/bar'));

   assertEquals(
       'Host, cgi parms:',
       ['<a href="http://www.example.com?bar=1&thud=2" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com?bar=1&thud=2</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com?bar=1&thud=2'));

   assertEquals(
       'Host, path, cgi parms:',
       ['<a href="http://www.example.com/foo?bar=1&thud=2" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com/foo?bar=1&thud=2</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com/foo?bar=1&thud=2'));

   assertEquals(
       'Host, port, path, cgi parms:',
       ['<a href="http://www.example.com:50/foo?bar=1&thud=2" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com:50/foo?bar=1&thud=2</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com:50/foo?bar=1&thud=2'));

   assertEquals(
       'Host, path, #',
       ['<a href="http://www.example.com/foo#bar" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com/foo#bar</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com/foo#bar'));

   assertEquals(
       'Host, path, port, #',
       ['<a href="http://www.example.com:50/foo#bar" ',
        'onclick="document.openLink(this);',
        'return false;">http://www.example.com:50/foo#bar</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://www.example.com:50/foo#bar'));

   assertEquals(
       'Hpst, path, nonalphanumeric chars',
       ['<a href="http://foo-bar.com/foo-bar_baz+%.jpg" ',
        'onclick="document.openLink(this);',
        'return false;">http://foo-bar.com/foo-bar_baz+%.jpg</a>'
        ].join(''),
       PAGESPEED.Utils.linkify('http://foo-bar.com/foo-bar_baz+%.jpg'));
}

function testLinkifyTrickyContext() {
  assertEquals(
      'Text after link',
      ['<a href="http://123.45.67.89:80/hello" ',
       'onclick="document.openLink(this);return false;">',
       'http://123.45.67.89:80/hello</a> ',
       'post'
       ].join(''),
      PAGESPEED.Utils.linkify('http://123.45.67.89:80/hello post'));

  assertEquals(
      'Text before link',
      ['pre <a href="http://123.45.67.89:80/hello" ',
       'onclick="document.openLink(this);return false;">',
       'http://123.45.67.89:80/hello</a>'
       ].join(''),
      PAGESPEED.Utils.linkify('pre http://123.45.67.89:80/hello'));

  assertEquals(
      'Text before and after link',
      ['pre <a href="http://123.45.67.89:80/',
       'hello" ',
       'onclick="document.openLink(this);return false;">',
       'http://123.45.67.89:80/hello</a> post'
       ].join(''),
      PAGESPEED.Utils.linkify('pre http://123.45.67.89:80/hello post'));

  assertEquals(
      'Period after host',
      ['Trailing .: <a href="http://www.example.com" ',
       'onclick="document.openLink(this);return false;">',
       'http://www.example.com</a>.'
        ].join(''),
       PAGESPEED.Utils.linkify('Trailing .: http://www.example.com/.'));
}

function testLinkifyWithExtraEqualsInParams() {
  var stringWithUrls =
      ['http://www.example.com/foo/bar/default.jpg',
       '?h=1&w=2&hash=_abc_def_123='
       ].join('');

  var linkified = PAGESPEED.Utils.linkify(stringWithUrls)

  assertEquals(
      ['<a href="http://www.example.com/foo/bar/default.jpg',
       '?h=1&w=2&hash=_abc_def_123="',
       ' onclick="document.openLink(this);return false;">',
       '/foo/bar/default.jpg?h=1&w=2&hash=_abc_def_123=</a>'
       ].join(''),
      linkified);

  // Now try the same url emberded in some text.
  var stringWithUrls =
      ['Here is a URL: http://img.youtube.com/vi/1hy8pCXqhzc/default.jpg',
       '?h=75&w=100&sigh=__uPqi_oRsm_32jTYrKNbKt4D0FNY= with an = on the end.'
       ].join('');

  var linkified = PAGESPEED.Utils.linkify(stringWithUrls)

  assertEquals(
      ['Here is a URL: ',
       '<a href="http://img.youtube.com/vi/1hy8pCXqhzc/default.jpg?',
       'h=75&w=100&sigh=__uPqi_oRsm_32jTYrKNbKt4D0FNY=" ',
       'onclick="document.openLink(this);return false;">',
       'default.jpg?h=75&w=100&sigh=__uPqi_oRsm_32jTYrKNbK...</a>',
       ' with an = on the end.'].join(''),
      linkified);
}

function getMockInputStream(chunks) {
  var mockInputStream = {
    chunks: chunks,
    init: function() {},
    available: function() {
      if (!chunks || chunks.length == 0) return 0;
      return chunks[0].length;
    },
    read: function(numBytes) {
      assertEquals(chunks[0].length, numBytes);
      return chunks.shift();
    },
    close: function() {}
  };

  return mockInputStream;
}

function testReadInputStream() {
  var realCCIN = PAGESPEED.Utils.CCIN;

  var inputStreamChunks = ['here', 'is', 'some', 'text'];
  var mockInputStream = getMockInputStream(inputStreamChunks);

  PAGESPEED.Utils.CCIN = function() { return mockInputStream; };

  assertEquals(null, PAGESPEED.Utils.readInputStream(null));

  var myInputStream = getMockInputStream(null);
  var expectedOut = inputStreamChunks.join('');
  var actualOut = PAGESPEED.Utils.readInputStream(myInputStream);
  assertEquals('Not all input chunks were consumed',
               0, inputStreamChunks.length);
  assertEquals(expectedOut, actualOut);

  PAGESPEED.Utils.CCIN = realCCIN;
}

// findPreRedirectUrl:

function testFindPreRedirectUrl_noRedirects() {
  // When there are no redirects, getComponents().redirect is undefined.
  PAGESPEED.Utils.getComponents = function() {
    return {};
  }

  assertEquals(
      'http://www.example.com',
      PAGESPEED.Utils.findPreRedirectUrl('http://www.example.com'));

  // Strip fragments:
  assertEquals(
      'http://www.example.com/foo',
      PAGESPEED.Utils.findPreRedirectUrl('http://www.example.com/foo#bar'));
}

function testFindPreRedirectUrl_simpleRedirectChain() {
  // a.com -> b.com
  // b.com -> c.com
  PAGESPEED.Utils.getComponents = function() {
    return {
      redirect: {
        'http://a.com/': {
          'elements': ['http://b.com/']
        },
        'http://b.com/': {
          'elements': ['http://c.com/']
        },
        'http://unrelated.junk.com/': {
          'elements': ['http://http://unrelated.junk.org']
        }
      }
    };
  };

  assertEquals(
      'a.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://a.com/'));

  assertEquals(
      'b.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://b.com/'));

  assertEquals(
      'c.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://c.com/'));

  assertEquals(
      'no.redirect.com -> no.redirect.com',
      'http://no.redirect.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://no.redirect.com/'));
}

function testFindPreRedirectUrl_complexRedirectChain() {
  // a.com -> b1.com
  // a.com -> b2.com
  // b1.com -> c.com
  // b2.com -> c.com
  PAGESPEED.Utils.getComponents = function() {
    return {
      redirect: {
        'http://a.com/': {
          'elements': ['http://b1.com/', 'http://b2.com/']
        },
        'http://b1.com/': {
          'elements': ['http://c.com/']
        },
        'http://b2.com/': {
          'elements': ['http://c.com/']
        },
        'http://unrelated.junk.com/': {
          'elements': ['http://http://unrelated.junk.org']
        }
      }
    };
  };

  assertEquals(
      'a.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://a.com/'));

  assertEquals(
      'b1.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://b1.com/'));

  assertEquals(
      'b2.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://b2.com/'));

  assertEquals(
      'c.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://c.com/'));
}

// Test that when there is more than one candidate source url,
// the shortest one lowest in the alphabet is returned.
function testFindPreRedirectUrl_preferShortUrls() {
  // aaaaaaa.com -> dest.com
  // longname.com -> b.com
  // b.com -> dest.com
  // c.com -> dest.com
  // d.com -> dest.com

  PAGESPEED.Utils.getComponents = function() {
    return {
      redirect: {
        'http://aaaaaaa.com/': {
          'elements': ['http://dest.com/']
        },
        'http://longname.com/': {
          'elements': ['http://b.com/']
        },
        'http://b.com/': {
          'elements': ['http://dest.com/']
        },
        'http://c.com/': {
          'elements': ['http://dest.com/']
        }
      }
    };
  };

  assertEquals(
      'c.com is shorter than aaaaaaa.com and before d.com.  ' +
      'b.com is redirected to.',
      'http://c.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://dest.com/'));
}

// Test that a cycle in the graph does not cause an infinite loop.
function testFindPreRedirectUrl_redirectCycle() {
  // a.com -> cycle1.com
  // cycle1.com -> cycle2.com
  // cycle2.com -> cycle1.com
  // cycle2.com -> dest.com
  PAGESPEED.Utils.getComponents = function() {
    return {
      redirect: {
        'http://a.com/': {
          'elements': ['http://cycle1.com/']
        },
        'http://cycle1.com/': {
          'elements': ['http://cycle2.com/']
        },
        'http://cycle2.com/': {
          'elements': ['http://cycle1.com/', 'http://dest.com/']
        }
      }
    };
  };

  assertEquals(
      'a.com -> a.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://a.com/'));
  assertEquals(
      'a.com -> cycle1.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://cycle1.com/'));
  assertEquals(
      'a.com -> cycle2.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://cycle2.com/'));
  assertEquals(
      'a.com -> dest.com',
      'http://a.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://dest.com/'));
}

// Test that when there are no urls which are not redirect targets,
// the initial url is returned.
function testFindPreRedirectUrl_noSourceUrlOutsiodeCycle() {
  // cycle1.com -> cycle2.com
  // cycle2.com -> cycle1.com
  // cycle2.com -> dest.com
  PAGESPEED.Utils.getComponents = function() {
    return {
      redirect: {
        'http://cycle1.com/': {
          'elements': ['http://cycle2.com/']
        },
        'http://cycle2.com/': {
          'elements': ['http://cycle1.com/', 'http://dest.com/']
        }
      }
    };
  };

  assertEquals(
      'dest.com is the best we can do',
      'http://dest.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://dest.com/'));

  assertEquals(
      'cycle1.com is in a clique',
      'http://cycle1.com/',
      PAGESPEED.Utils.findPreRedirectUrl('http://cycle1.com/'));
}
