#!/usr/bin/python2.6
#
# Copyright 2010 Google Inc. All Rights Reserved.
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

"""Tests for poc."""

__author__ = 'aoates@google.com (Andrew Oates)'

import os
import unittest

import poc

# paths for test data
TEST_SRC_BASE = 'build/poc/testdata'
TEST_TMP_BASE = '/tmp'


class PocTest(unittest.TestCase):
  """Test cases for the poc .po compiler."""

  def setUp(self):
    self.out_dir = os.path.join(TEST_TMP_BASE, 'poc_test_files')

    self.assertFalse(os.path.exists(self.out_dir))
    os.mkdir(self.out_dir)

  def tearDown(self):
    os.system('rm -rf %s' % self.out_dir)

  def testUnicodeToCLiteral(self):
    """Tests the UnicodeToCLiteral function."""
    ustr = u'\uc810abc\uc218'
    self.assertEqual(r'"\xec" "\xa0" "\x90" "abc\xec" "\x88" "\x98" ""',
                     poc.UnicodeToCLiteral(ustr))

  def DoWriteStringTableTest(self, strings, comments, golden_data):
    """Runs poc.WriteStringTable and tests the generated string table.

    Args:
      strings: List of strings.
      comments: List of associated comments.
      golden_data: The "golden" string table we expect.
    """
    self.assertEqual([], os.listdir(self.out_dir))
    poc.WriteStringTable(strings, 'test_table', 'locale', self.out_dir,
                         'test_table.po.cc', comments)
    self.assertEqual(['test_table.po.cc'], os.listdir(self.out_dir))

    golden = poc.STRING_TABLE_TEMPLATE % {
        'table_name': 'test_table',
        'locale_str': '"locale"',
        'locale': 'locale',
        'table_data': golden_data,
        }

    file_contents = open(os.path.join(self.out_dir, 'test_table.po.cc')).read()
    self.assertEqual(golden, file_contents)

    os.unlink(os.path.join(self.out_dir, 'test_table.po.cc'))

  def testWriteStringTable(self):
    """Tests calling WriteStringTable with normal arguments."""
    strings = ['string1', 'string2']
    comments = ['comment1', 'comment2']
    golden_data = ('  "string1", // comment1\n'
                   '  "string2", // comment2\n'
                   '  0x0')
    self.DoWriteStringTableTest(strings, comments, golden_data)

  def testWriteStringTableNoComments(self):
    """Tests calling WriteStringTable with normal arguments."""
    strings = ['string1', 'string2']
    golden_data = ('  "string1",\n'
                   '  "string2",\n'
                   '  0x0')
    self.DoWriteStringTableTest(strings, None, golden_data)

  def testWriteStringTableNoLocale(self):
    """Tests calling WriteStringTable with locale=None."""
    strings = ['string1', 'string2']
    comments = ['comment1', 'comment2']

    self.assertEqual([], os.listdir(self.out_dir))
    poc.WriteStringTable(strings, 'test_table', None, self.out_dir,
                         'test_table.po.cc', comments)
    self.assertEqual(['test_table.po.cc'], os.listdir(self.out_dir))

    golden_data = ('  "string1", // comment1\n'
                   '  "string2", // comment2\n'
                   '  0x0')
    golden = poc.STRING_TABLE_TEMPLATE % {
        'table_name': 'test_table',
        'locale_str': 'NULL',
        'locale': 'master',
        'table_data': golden_data,
        }

    file_contents = open(os.path.join(self.out_dir, 'test_table.po.cc')).read()
    self.assertEqual(golden, file_contents)

    os.unlink(os.path.join(self.out_dir, 'test_table.po.cc'))

  def testGenerateStringTablesMaster(self):
    """Tests generating the master string table."""

    self.assertEqual([], os.listdir(self.out_dir))
    pot_file = os.path.join(TEST_SRC_BASE, 'test.pot')
    poc.GenerateStringTables(self.out_dir, pot_file, [])

    self.assertEqual(['master.po.cc'], os.listdir(self.out_dir))

    generated_file = os.path.join(self.out_dir, 'master.po.cc')
    golden_file = os.path.join(TEST_SRC_BASE, 'master.po.cc.golden')
    self.assertEqual(open(generated_file).read(), open(golden_file).read())

    os.unlink(generated_file)

  def testGenerateStringTables(self):
    """Tests generating the locale-specific string tables."""

    self.assertEqual([], os.listdir(self.out_dir))
    pot_file = os.path.join(TEST_SRC_BASE, 'test.pot')
    po_file = os.path.join(TEST_SRC_BASE, 'testA.po')
    poc.GenerateStringTables(self.out_dir, pot_file, [po_file])

    self.assertEqual(['testA.po.cc'], os.listdir(self.out_dir))

    generated_file = os.path.join(self.out_dir, 'testA.po.cc')
    golden_file = os.path.join(TEST_SRC_BASE, 'testA.po.cc.golden')
    self.assertEqual(open(generated_file).read(), open(golden_file).read())

    os.unlink(generated_file)


if __name__ == '__main__':
  unittest.main()
