#!/usr/bin/python2.6
#
# Copyright 2011 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

__author__ = 'bmcquade@google.com (Bryan McQuade)'

import sys

FILE_HEADER = (
"""// This filter set is based on the filterset database distributed by
// adblockrules.org.
//
// License information for the filterset database is available at
// http://adblockrules.org/wiki.php?Wiki:About

#include "adblockrules.h"

namespace adblockrules {
""")

FILE_FOOTER = """
}  // namespace adblockrules
"""


# Takes in a list of rule strings from an ABP rule file, and generates
# a list of equivalent regular expressions, as well as comment strings
# from the beginning of the input file.
def GenerateRegexp(rule_lines, rules, beginning_comments):
  for rule in rule_lines:
    rule = rule.strip()
    if not rule:
      continue

    if rule.startswith(('!','[Adblock')):
      if not rules:
        beginning_comments.append(rule)
      continue

    if len(rule) >= 2 and rule[0] == '/' and rule[-1] == '/':
      # A regular expression rule. Pass it through unmodified.
      rules.append(rule)
      continue

    # For now we do not support exception rules (those that start with
    # '@@') or filter options (denoted by a '$' within the rule), so
    # skip over any that are encountered.
    if rule.startswith('@@') or '$' in rule:
      print >> sys.stderr, 'Ignoring rule ', rule
      continue

    # Rules that contain '##' are used to hide certain DOM nodes in
    # the UI. They are not relevant for URL matching, so ignore them.
    if '##' in rule:
      continue;

    # Properly escape dots and question marks so they are parsed as
    # literals instead of regular expression characters with special
    # meaning.
    rule = rule.replace('.', '\\\.').replace('?', '\\\?')

    # Convert the adblock wildcard to its regular expression
    # equivalent.
    rule = rule.replace('*', '.*')

    # Convert the adblock separator character to its regular
    # expression equivalent.
    rule = rule.replace('^', '[^a-zA-Z0-9_-.%]')

    # Convert the adblock beginning of string character to its regular
    # expresson equivalent.
    match_http_and_https_prefix = False
    if rule[0] is '|':
      # '||' is special and indicates either http or https prefix.
      if rule.startswith('||'):
        rule = rule[2:]
        match_http_and_https_prefix = True
      else:
        rule = '^' + rule[1:]

    # Convert the adblock end of string character to its regular
    # expression equivalent.
    if rule[-1] is '|':
      rule = rule[:-1] + '$'

    # Properly escape the pipe so it is parsed as a literal instead of
    # a regular expression character with special meaning.
    rule = rule.replace('|', '\\\|')

    if match_http_and_https_prefix:
      rule = '^http://' + rule + '|' + '^https://' + rule

    rules.append(rule)


def PrintRegexpFile(variable_name, rules, beginning_comments):
  print FILE_HEADER
  for comment in beginning_comments:
    print '// ' + comment
  print ''
  print ('const char* ' +
         variable_name +
         ' =\n      "(' +
         '"\n      "|'.join(rules) +
         ')";')
  print FILE_FOOTER


def main():
  if len(sys.argv) != 2:
    print >> sys.stderr, 'Usage: regexp_from_abp_ruleset variable_name'
    sys.exit(1)

  rules = []
  beginning_comments = []
  GenerateRegexp(sys.stdin.readlines(), rules, beginning_comments)
  PrintRegexpFile(sys.argv[1], rules, beginning_comments)


if __name__ == '__main__':
  main()
