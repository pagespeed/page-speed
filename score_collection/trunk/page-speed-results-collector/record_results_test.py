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


class TestPostResults(unittest.TestCase):

  def setUp(self):
    self.mox = mox.Mox()

    # Mock all datastore access calls.
    self.mox.StubOutWithMock(record_results, 'BuildPageSpeedResult')

  def tearDown(self):
    self.mox.UnsetStubs()

  def testPostInvalidData(self):
    """POST to PostResults with bad data should generate an invalid record."""

    post_results = record_results.PostResults()

    # Create a mock datastore object, and make BuildPageSpeedResult() return it.
    mock_page_speed_result = self.mox.CreateMock(record_results.PageSpeedResult)
    record_results.BuildPageSpeedResult().AndReturn(mock_page_speed_result)

    # Mock out post_results.request
    post_results.request = self.mox.CreateMockAnything()
    post_results.request.get('content').AndReturn('this is not valid JSON')

    mock_page_speed_result.put()

    self.mox.ReplayAll()
    post_results.post()
    self.mox.VerifyAll()

    self.assertFalse(mock_page_speed_result.is_valid)

  def testPostValidData(self):
    """POST to PostResults with good data should generate a valid record."""

    post_results = record_results.PostResults()

    # Create a mock datastore object, and make BuildPageSpeedResult() return it.
    mock_page_speed_result = self.mox.CreateMock(record_results.PageSpeedResult)
    record_results.BuildPageSpeedResult().AndReturn(mock_page_speed_result)

    # Mock out post_results.request
    post_results.request = self.mox.CreateMockAnything()
    post_results.request.get('content').AndReturn(
        '{'
        '  "pageStats": {'
        '    "initialUrl": "http://a.com/b",'
        '    "transferSize": 12345,'
        '    "overallScore": 100'
        '  }'
        '}')

    mock_page_speed_result.put()

    self.mox.ReplayAll()
    post_results.post()
    self.mox.VerifyAll()

    self.assertTrue(mock_page_speed_result.is_valid)
    self.assertEquals('http://a.com/b', mock_page_speed_result.initial_url)
    self.assertEquals(12345, mock_page_speed_result.transfer_size)
    self.assertEquals(100, mock_page_speed_result.overall_score)

  def setupRequestHandlerObjs(self, request_handler):
    """Add mocks for the web app objects set on a request handler."""
    request_handler.response = self.mox.CreateMockAnything()
    request_handler.response.out = self.mox.CreateMockAnything()

  def testShowResultsNoData(self):
    """Test that if there are no results, ShowResults shows nothing."""

    self.mox.StubOutWithMock(record_results.PageSpeedResult, 'all')

    # Object under test:
    show_results = record_results.ShowResults()
    self.setupRequestHandlerObjs(show_results)

    # Ask for all PageSpeedResult datastore records:
    ps_result_query = self.mox.CreateMockAnything()
    record_results.PageSpeedResult.all().AndReturn(ps_result_query)

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

    show_results.response.out.write('Render This String')

    self.mox.ReplayAll()
    show_results.get()
    self.mox.VerifyAll()

  def testShowResultsShowsAllAvailable(self):
    """Test that if there are a few results, ShowResults shows them."""
    self.mox.StubOutWithMock(record_results.PageSpeedResult, 'all')

    # Object under test:
    show_results = record_results.ShowResults()
    self.setupRequestHandlerObjs(show_results)

    # Ask for all PageSpeedResult datastore records:
    ps_result_query = self.mox.CreateMockAnything()
    record_results.PageSpeedResult.all().AndReturn(ps_result_query)

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

    show_results.response.out.write('Render This String')

    self.mox.ReplayAll()
    show_results.get()
    self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
