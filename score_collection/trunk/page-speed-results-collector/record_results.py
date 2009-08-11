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

"""Record Page Speed results to the datastore."""

__author__ = 'skerner@google.com (Sam Kerner)'

import logging
import os.path
import sys

from django.utils import simplejson
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp import template
from google.appengine.ext.webapp.util import run_wsgi_app

PAGE_SPEED_URL = 'http://code.google.com/p/page-speed'

class MainPage(webapp.RequestHandler):
  """A page to allow a user to see that they set up the server correctly."""

  def get(self):
    self.response.headers['Content-Type'] = 'text/plain'
    self.response.out.write(
        'Congratulations, Page Speed Results Collector is running.')


# Objects that inherit from db.Model represent entries in the app
# engine datastore.  See app engine docs for details:
# http://code.google.com/appengine/docs/python/datastore/entitiesandmodels.html
class FullResultRecord(db.Model):
  """Datastore record which holds a single set of Page Speed results."""

  # Store the raw JSON encoded results sent to us.  This ensures that
  # we always see what the client realy sent when diagnosing issues.
  content = db.TextProperty()

  # Were we able to decode |content|?  If not, than properties read
  # from |context| will not be set.
  is_valid = db.BooleanProperty()

  # Properties read from |content|.
  initial_url = db.TextProperty()  # Use text because strings have 500b limit.
  transfer_size = db.IntegerProperty()
  overall_score = db.IntegerProperty()

  # TODO: Parse out the fields of the content and index records by
  # date, url, etc.

  # Time we received the data.  The time sent by the client is in
  # self.content, but we record our own time to avoid issues with mis-set
  # clocks, time zones, and stored results that are sent in batches.
  time_received = db.DateTimeProperty(auto_now_add=True)


def BuildFullResultRecord():
  """Call this to build a FullResultRecord object.  Makes unit tests simpler."""
  return FullResultRecord()


class FullBeaconReceiver(webapp.RequestHandler):
  """A JSON-encoded results object is decoded and stored in the data store."""

  def post(self):
    page_speed_results = BuildFullResultRecord()

    # Save the raw data we receive.  We only parse and store parts of
    # it, and having the original content ensures that we never lose
    # any information.

    content = self.request.get('content')
    is_valid = False
    try:
      results_obj = simplejson.loads(content)
      is_valid = True

    except ValueError:
      is_valid = False
      logging.error('There was an error decoding this data: %s' % content)

    page_speed_results.content = content
    page_speed_results.is_valid = is_valid

    # At this point, |page_speed_results| holds the information we got
    # from the network.  It is .put() in the finally: clause so that
    # any exception raised while processing it does not keep us from
    # storing it.

    try:
      if is_valid:
        page_stats = results_obj['pageStats']

        page_speed_results.initial_url = page_stats['initialUrl']
        page_speed_results.transfer_size = page_stats['transferSize']
        page_speed_results.overall_score = page_stats['overallScore']

    finally:
      # Even if the input is invalid, it gets added to the data store.
      page_speed_results.put()


class FullBeaconShower(webapp.RequestHandler):
  """Produce a simple list of recorded results."""

  MAX_RESULTS_TO_DISPLAY = 100

  def get(self):
    # Get the most recent results, ordered by the time they were recorded.
    ps_results_query = FullResultRecord.all().order('-time_received')
    ps_results = ps_results_query.fetch(self.MAX_RESULTS_TO_DISPLAY)

    template_values = {
      'num_results': len(ps_results),
      'ps_results': ps_results,
      'page_speed_url': PAGE_SPEED_URL
    }
    template_path = os.path.join(os.path.dirname(__file__),
                                 'templates',
                                 'showResults.tmpl')
    self.response.out.write(template.render(template_path, template_values))


class MinimalBeaconRecord(db.Expando):
  """Store data from a minimal beacon."""

  # Time we received the beacon.
  time_received = db.DateTimeProperty(auto_now_add=True)

  # This is an expando object, meaning that other properties
  # will be added by users.  These properties will always
  # start with 'param_' to ensure a malicious beacon can
  # not overwrite methods of this object.  See MinimalBeacon.get().
  # Docs on expando class:
  # http://code.google.com/appengine/docs/python/datastore/expandoclass.html


class MinimalBeaconReceiver(webapp.RequestHandler):
  """Recieve and store results in the minimal beacon format."""

  def get(self):
    parms_dict = {}

    for item, value in self.request.params.items():
      # item and value are unicode strings.  Represent them as UTF8,
      # so that the use of **params_dict below is legal python.
      parms_dict['param_%s' % item.encode('utf-8')] = value.encode('utf-8')

    record = MinimalBeaconRecord(**parms_dict)
    record.put()


class MinimalBeaconShower(webapp.RequestHandler):
  """Produce a simple list of minimal beacon results."""

  MAX_RESULTS_TO_DISPLAY = 100

  def get(self):
    # Get the most recent results, ordered by the time they were recorded.
    query = MinimalBeaconRecord.all().order('-time_received')
    query_results = query.fetch(self.MAX_RESULTS_TO_DISPLAY)

    template_values = {
      'num_results': len(query_results),
      'min_beacon_data': query_results,
      'page_speed_url': PAGE_SPEED_URL
    }
    template_path = os.path.join(os.path.dirname(__file__),
                                 'templates',
                                 'showMinResults.tmpl')
    self.response.out.write(template.render(template_path, template_values))


application = webapp.WSGIApplication(
    [('/', MainPage),

     # Recieve the full page speed results object.
     ('/beacon/full/receive', FullBeaconReceiver),

     # Show the last few page speed results objects received.
     ('/beacon/full/show', FullBeaconShower),

     # Recieve the minimal page speed results object.
     ('/beacon/minimal/receive', MinimalBeaconReceiver),

     # Show the last few  minimal page speed results received.
     ('/beacon/minimal/show', MinimalBeaconShower)],

     debug=True)

def main():
  run_wsgi_app(application)

if __name__ == "__main__":
  main()
