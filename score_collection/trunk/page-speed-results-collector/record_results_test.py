#!/usr/bin/python2.5
#
# Copyright 2009 Google Inc.
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

"""Unit tests for record_results.py."""

# This must be imported this first because it gives easy-to-read error
# messages if mox is not installed or app engine classes are not found.
import setup_app_engine_unittests

import unittest
import sys
import os

import mox
from google.appengine.ext.webapp import template

import record_results


class TestFullBeaconReceiver(unittest.TestCase):

  def setUp(self):
    self.mox = mox.Mox()

    # Mock all datastore access calls.
    self.mox.StubOutWithMock(record_results, 'BuildFullResultRecord')

  def tearDown(self):
    self.mox.UnsetStubs()

  def testPostInvalidData(self):
    """Bad data POSTed to FullBeaconReceiver creates a record marked invalid."""

    receiver = record_results.FullBeaconReceiver()

    # Have BuildFullResultRecord() return a mock datastore object.
    mock_page_speed_result = self.mox.CreateMock(
        record_results.FullResultRecord)
    record_results.BuildFullResultRecord().AndReturn(mock_page_speed_result)

    # Mock out receiver.request
    receiver.request = self.mox.CreateMockAnything()
    receiver.request.get('content').AndReturn('this is not valid JSON')

    mock_page_speed_result.put()

    self.mox.ReplayAll()
    receiver.post()
    self.mox.VerifyAll()

    self.assertFalse(mock_page_speed_result.is_valid)

  def testPostValidData(self):
    """POST to FullBeaconReceiver with good data should generate a valid record.
    """

    receiver = record_results.FullBeaconReceiver()

    # Have BuildFullResultRecord() return a mock datastore object.
    mock_page_speed_result = self.mox.CreateMock(
        record_results.FullResultRecord)
    record_results.BuildFullResultRecord().AndReturn(mock_page_speed_result)

    # Mock out receiver.request
    receiver.request = self.mox.CreateMockAnything()
    receiver.request.get('content').AndReturn(
        '{'
        '  "pageStats": {'
        '    "initialUrl": "http://a.com/b",'
        '    "transferSize": 12345,'
        '    "overallScore": 100'
        '  }'
        '}')

    mock_page_speed_result.put()

    self.mox.ReplayAll()
    receiver.post()
    self.mox.VerifyAll()

    self.assertTrue(mock_page_speed_result.is_valid)
    self.assertEquals('http://a.com/b', mock_page_speed_result.initial_url)
    self.assertEquals(12345, mock_page_speed_result.transfer_size)
    self.assertEquals(100, mock_page_speed_result.overall_score)

  def SetupRequestHandlerObjs(self, request_handler):
    """Add mocks for the web app objects set on a request handler."""
    request_handler.response = self.mox.CreateMockAnything()
    request_handler.response.out = self.mox.CreateMockAnything()

  def testFullBeaconShowerNoData(self):
    """Test that if there are no results, FullBeaconShower shows nothing."""

    self.mox.StubOutWithMock(record_results.FullResultRecord, 'all')

    # Object under test:
    results_show_handler = record_results.FullBeaconShower()
    self.SetupRequestHandlerObjs(results_show_handler)

    # Ask for all FullResultRecord datastore records:
    ps_result_query = self.mox.CreateMockAnything()
    record_results.FullResultRecord.all().AndReturn(ps_result_query)

    ps_result_all = self.mox.CreateMockAnything()
    ps_result_query.order('-time_received').AndReturn(ps_result_all)

    # The fetch returns no results.
    ps_result_all.fetch(mox.IsA(int)).AndReturn([])

    # Check that the template is told that there are no results.
    self.mox.StubOutWithMock(template, 'render')
    template.render(mox.IsA(str),  # The path to the template.
                    # Test that the template dictionary has {numresults = 0}
                    mox.And(mox.IsA(dict),
                            mox.In('num_results'),
                            mox.Func(lambda d: d['num_results'] == 0)
                            ),
                    ).AndReturn('Render This String')

    results_show_handler.response.out.write('Render This String')

    self.mox.ReplayAll()
    results_show_handler.get()
    self.mox.VerifyAll()

  def testFullBeaconShowerShowsAllAvailable(self):
    """Test that if there are a few results, FullBeaconShower shows them."""
    self.mox.StubOutWithMock(record_results.FullResultRecord, 'all')

    # Object under test:
    results_show_handler = record_results.FullBeaconShower()
    self.SetupRequestHandlerObjs(results_show_handler)

    # Ask for all FullResultRecord datastore records:
    ps_result_query = self.mox.CreateMockAnything()
    record_results.FullResultRecord.all().AndReturn(ps_result_query)

    ps_result_all = self.mox.CreateMockAnything()
    ps_result_query.order('-time_received').AndReturn(ps_result_all)

    # The fetch returns two objects.
    ps_result_all.fetch(mox.IsA(int)).AndReturn([{}, {}])

    # Check that the template is told that there are no results.
    self.mox.StubOutWithMock(template, 'render')
    template.render(mox.IsA(str),  # The path to the template.
                    # Test that the template dictionary has {numresults = 0}
                    mox.And(mox.IsA(dict),
                            mox.In('num_results'),
                            mox.Func(lambda d: d['num_results'] == 2)
                            ),
                    ).AndReturn('Render This String')

    results_show_handler.response.out.write('Render This String')

    self.mox.ReplayAll()
    results_show_handler.get()
    self.mox.VerifyAll()


class TestMinimalBeaconReceiver(unittest.TestCase):

  def setUp(self):
    self.mox = mox.Mox()

  def tearDown(self):
    self.mox.UnsetStubs()

  def testStoreMinimalBeacon(self):
    """Test that a minimal beacon is stored properly."""

    minimal_beacon_receiver = record_results.MinimalBeaconReceiver()

    # Mock out minimal_beacon_receiver.request.params .
    minimal_beacon_receiver.request = self.mox.CreateMockAnything()
    minimal_beacon_receiver.request.params = self.mox.CreateMockAnything()

    # Create a realistic beacon input by taking a set of key value
    # pairs and turning it into a param string.  Also create the
    # params that we expect.  This test almost re-implements the
    # function under test, but it is useful because it ensures that
    # a sample input will not throw an exception.
    param_dict = {
        'v': '1.1',
        'u': 'http://www.example.com/path/to/data.html#label',
        'w': '55975',
        'pBrowserCache': '87.5'
    }
    # Send in the dict as a sequence of key-value pairs, just as the
    # webapp framework will.
    param_key_value_pairs = list(param_dict.iteritems())

    minimal_beacon_receiver.request.params.items().AndReturn(
        param_key_value_pairs)


    # Expect that the datastore object has those params (with a prefix).
    param_prefix_dict = dict(
        [('param_%s' % k, param_dict[k]) for k in param_dict])

    self.mox.StubOutWithMock(
        record_results, 'MinimalBeaconRecord', use_mock_anything=True)
    mock_min_beacon_record = self.mox.CreateMockAnything()

    record_results.MinimalBeaconRecord(**param_prefix_dict).AndReturn(
        mock_min_beacon_record)

    mock_min_beacon_record.put()

    self.mox.ReplayAll()
    minimal_beacon_receiver.get()
    self.mox.VerifyAll()

if __name__ == '__main__':
    unittest.main()
