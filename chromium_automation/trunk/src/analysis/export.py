#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Exports data to other formats for download."""

import csv

from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app

import config
import models
import result_iterator
import util


class ExportCSV(webapp.RequestHandler):
  """Exports results to a CSV file."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    if result:
      data = result.GetResultData()
      data_iter = CSVIterator(data)
      results = data_iter.Iterate(tests=data.keys())

      columns = results[0].keys()
      columns.sort()

      self.response.headers["Content-Type"] = "text/csv"
      content_disposition = "attachment;filename=result_%d.csv" % result_id
      self.response.headers["Content-Disposition"] = content_disposition
      self.response.out.write(",".join(columns) + "\n")

      csvout = csv.DictWriter(self.response.out, columns, extrasaction="ignore")
      csvout.writerows(results)
    else:
      self.error(404)


class CSVIterator(result_iterator.MetricIterator):
  """Converts interval nexted CSV structure to dict record list."""

  def __init__(self, data):
    self.csvdict = []
    result_iterator.MetricIterator.__init__(self, data)

  def PreVariation(self, var_name, test_name):
    # Each test/variation has a row
    self.cur_row = {"test": test_name, "variation": var_name}

  def Metric(self, metric_name, metric_value,
             unused_cached, unused_var_name, unused_test_name):
    # Each row has median/mean of aggregate values
    # or actual values for non-aggregate fields.
    if util.IsNumeric(metric_value) and util.IsAggregate(metric_value):
      self.cur_row[metric_name + "_median"] = util.Median(metric_value)
      self.cur_row[metric_name + "_mean"] = util.Mean(metric_value)
    else:
      self.cur_row[metric_name] = metric_value

  def PostVariation(self, unused_var_name, unused_test_name):
    self.csvdict.append(self.cur_row)

  def GetData(self):
    return self.csvdict


def main():
  application = webapp.WSGIApplication([
      ("/export/csv", ExportCSV),
  ], debug=config.DEBUG)

  run_wsgi_app(application)

if __name__ == "__main__":
  main()
