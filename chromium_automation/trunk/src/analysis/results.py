#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Displays result data to the user in various ways.

Most complex code is in result_iterator.py.
"""

from google.appengine.dist import use_library
use_library("django", "1.2")

import itertools

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template
from google.appengine.ext.webapp.util import run_wsgi_app

import config
import models
import result_iterator
import util

webapp.template.register_template_library("templates.filters")


class ResultsIndex(webapp.RequestHandler):
  """Allows the user to set up reports/visualizations."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    if result:
      data = result.GetResultData()
      allmetrics = util.GetMetrics(data)
      metrics = util.GetMetrics(data, util.D_N)
      delta_funcs = GetDeltaFunctionList(data)

      template_values = {
          "id": result_id,
          "tests": util.GetTests(data),
          "variations": util.GetVariations(data),
          "metrics": metrics,
          "allmetrics": allmetrics,
          "delta_functions": delta_funcs,
          "runs": util.GetRuns(data),
          "reprstats": ["median", "mean"]
      }
      path = "templates/results_index.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)


class ResultsViewData(webapp.RequestHandler):
  """Displays the data to the user with no visualizations."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    reprstat = GetReprStat(self.request)
    metrics = self.request.get_all("metrics")
    if result:
      data = result.GetResultData()
      data_iter = result_iterator.ViewDataIterator(data, reprstat)
      tests = data_iter.Iterate(metric_names=metrics)
      columns = [("string", "Variation")]
      columns += util.GetMetrics(data, util.D_N, True, names=metrics)

      template_values = {
          "id": result_id,
          "data": data,
          "tests": tests,
          "columns": columns,
      }
      path = "templates/results_data.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)


class ResultsBasicStat(webapp.RequestHandler):
  """Displays single metric statistics to the user."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    metric = self.request.get("metric")
    delta_func = GetDeltaFunc(self.request, "delta_func")
    if result and metric:
      data = GetDataSet(result.GetResultData(), self.request)

      data_iter = result_iterator.BasicStatIterator(data, delta_func)
      chartdata, quartiles = data_iter.Iterate(metric_names=[metric])
      columns = [("string", "Variation")]
      columns += [("number", key) for key in util.GetVariations(data)]

      template_values = {
          "id": result_id,
          "metric": metric,
          "columns": columns,
          "data": chartdata,
          "quartiles": quartiles
      }
      path = "templates/results_basic.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)


class ResultsCorrelation(webapp.RequestHandler):
  """Displays two metric statistics to the user."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    imetric = self.request.get("imetric")
    dmetrics = self.request.get_all("dmetrics")
    metrics = [imetric] + dmetrics
    idelta = GetDeltaFunc(self.request, "idelta")
    ddelta = GetDeltaFunc(self.request, "ddelta")
    if result and dmetrics:
      data = result.GetResultData()
      data_iter = result_iterator.CorrelationIterator(data, idelta, ddelta,
                                                      imetric)
      tabledata, correlation = data_iter.Iterate(metric_names=dmetrics,
                                                 variation_names=[None])
      chartdata = [[x[:-1] for x in cached] for cached in tabledata]
      columns = [("number", key) for key in metrics]

      template_values = {
          "id": result_id,
          "imetric": imetric,
          "dmetrics": dmetrics,
          "metrics": metrics,
          "columns": columns,
          "data": chartdata,
          "tcolumns": columns + [("string", "URL")],
          "tdata": tabledata,
          "correlation": correlation,
          "variations": (self.request.get("idelta").split("|"),
                         self.request.get("ddelta").split("|"))
      }
      path = "templates/results_correlation.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)
      self.response.out.write("404: Please make sure you have selected ")
      self.response.out.write("dependent metric(s) and that the test ")
      self.response.out.write("is valid.")


class ResultsSignificance(webapp.RequestHandler):
  """Displays confidence interval statistics to the user."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    metrics = self.request.get_all("metrics")
    basevar = self.request.get("basevariation")
    mainvar = self.request.get("mainvariation")
    filter = GetFilterFunc(self.request, True)
    if result and metrics:
      data = result.GetResultData()
      data_iter = result_iterator.SignificanceIterator(data, basevar, filter)
      tabledata, chartdata = data_iter.Iterate(metric_names=metrics,
                                               variation_names=[mainvar])
      columns = [("string", "Test")]
      for x in tabledata[0]:
        columns.extend([("number", "%s %s LB" % (x, basevar)),
                        ("number", "%s %s UB" % (x, basevar)),
                        ("number", "%s %s LB" % (x, mainvar)),
                        ("number", "%s %s UB" % (x, mainvar))])

      template_values = {
          "id": result_id,
          "metrics": metrics,
          "data": tabledata,
          "columns": columns,
          "chart": chartdata,
          "variations": (basevar, mainvar)
      }
      path = "templates/results_significance.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)
      self.response.out.write("404: Please make sure you have selected ")
      self.response.out.write("some metrics(s) and that the test ")
      self.response.out.write("is valid.")


class ResultsRuns(webapp.RequestHandler):
  """Displays metric change between runs to the user."""

  def get(self):  # pylint: disable-msg=C6409
    result_id = int(self.request.get("id"))
    result = models.WPTResult.get_by_id(result_id)
    metric = self.request.get("metric")
    if result and metric:
      data = result.GetResultData()
      data_iter = result_iterator.RunsIterator(data)
      chartdata = data_iter.Iterate(metric_names=[metric])
      columns = [("string", "Test")] + [("number", x)
                                        for x in util.GetTests(data)]

      template_values = {
          "id": result_id,
          "columns": columns,
          "chart": chartdata,
      }
      path = "templates/results_runs.html"
      self.response.out.write(template.render(path, template_values))
    else:
      self.error(404)


def GetFilterFunc(req, enable):
  """Returns a function that filters out results."""
  if enable and req.get("filtervalues") == "basic":

    def FilterFunc(values):
      if len(values) > 10 and util.IsNumeric(values):
        quartiles = util.Quartiles(values)
        iqr = (quartiles[2] - quartiles[1]) * 1.5
        liqr = quartiles[1] - iqr
        hiqr = quartiles[2] + iqr
        values = filter(lambda x: x >= liqr and x <= hiqr, values)
      return values
    return FilterFunc
  else:
    return lambda x: x


def GetReprStat(req, filter_items=True):
  """Returns a representative statistic function based on the query string."""
  if req.get("reprstat") in ("", "median"):
    func = util.Median
  else:
    func = util.Mean
  filter_func = GetFilterFunc(req, filter_items)

  def ReprStatFunc(lst):
    if util.IsAggregate(lst):
      if not lst:
        return 0
      if not util.IsNumeric(lst):
        return str(",".join(lst))
    else:
      return lst if util.IsNumeric(lst) else str(lst)
    lst = filter_func(lst)
    return func(lst)
  return ReprStatFunc


def GetDeltaFunctionList(data):
  variations = util.GetVariations(data)
  none = [("|%s|" % x, "%s - 0" % x) for x in variations]
  delta = [("%s|%s|%s" % x, "(%s - %s) %s" % (x[1], x[2], x[0]))
           for x in itertools.product(("absolute", "percent"),
                                      variations, variations)]
  return none + delta


def GetDeltaFunc(req, name="delta_func"):
  """Creates a delta closure that subtracts a variation.

  Args:
    req: The request object.
    name: The name of the get argument specifying the delta function to use.

  Returns:
   A representative statistic.
  """
  delta, mv, bv = req.get(name).split("|")
  reprstat = GetReprStat(req, req.get("test") != "__aggregate")
  if delta == "percent":

    def DeltaFunc(data, test, cached, metric, mvar=None):
      basevar = reprstat(data[test][bv][cached][metric])
      mainvar = reprstat(data[test][mv or mvar][cached][metric])
      if not mainvar and not basevar:
        return 0
      return round((mainvar - basevar) / ((mainvar + basevar) / 2) * 100, 3)
  elif delta == "absolute":

    def DeltaFunc(data, test, cached, metric, mvar=None):
      baseval = reprstat(data[test][bv][cached][metric])
      return reprstat(data[test][mv or mvar][cached][metric]) - baseval
  else:

    def DeltaFunc(data, test, cached, metric, mvar=None):
      return reprstat(data[test][mv or mvar][cached][metric])
  return DeltaFunc


def GetDataSet(data, req):
  """Returns a data set based on the query string."""
  test = req.get("test") or "__all"
  if test == "__all":
    tests = data
  elif test == "__aggregate":
    reprstat = GetReprStat(req, True)
    filter_func = lambda x: [reprstat(x)]
    aggr_iter = result_iterator.AggregateIterator(data, filter_func)
    tests = aggr_iter.Iterate()
  else:
    tests = {test: data[test]}
  return tests


def main():
  application = webapp.WSGIApplication([
      ("/results/data", ResultsViewData),
      ("/results/basic", ResultsBasicStat),
      ("/results/correlation", ResultsCorrelation),
      ("/results/significance", ResultsSignificance),
      ("/results/runs", ResultsRuns),
      ("/results", ResultsIndex),
  ], debug=config.DEBUG)

  run_wsgi_app(application)

if __name__ == "__main__":
  main()
