#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Iterates over the result data to produce specific data structures.

Many of the current iterators rely on the iteration being in a specific order.
This is not ideal and at some point a better method should be devised.
"""

__author__ = "azlatin@google.com (Alexander Zlatin)"


import util


class MetricIterator(object):
  """Iterates over a result data structure."""

  def __init__(self, data):
    self.data = data
    self.metrics = None

  def PreTest(self, test_name):
    pass

  def PreVariation(self, var_name, test_name):
    pass

  def PreCached(self, cached, var_name, test_name):
    pass

  def Metric(self, metric_name, metric_value, cached, var_name, test_name):
    pass

  def PostCached(self, cached, var_name, test_name):
    pass

  def PostVariation(self, var_name, test_name):
    pass

  def PostTest(self, test_name):
    pass

  def GetData(self):
    return None

  def Iterate(self, metric_names=None, variation_names=None,
              view_list=None, tests=None):
    """Iterates over the result data structure.

    Iterates over the result data structure, calling the overridden methods
    at each step of the iteration.

    Args:
      metric_names: The names of the metrics to iterate over.
      variation_names: The names of variations to iterate over.

    Returns
      The result of GetData()
    """
    if tests is None:
      tests = util.GetTests(self.data)
    
    if metric_names is None:
      metric_names = util.GetMetrics(self.data)
    self.metrics = metric_names

    if variation_names is None:
      variation_names = util.GetVariations(self.data)
      
    if view_list is None:
      view_list = (0, 1)

    for test_name in tests:
      self.PreTest(test_name)
      test_data = self.data[test_name]
      for var_name in variation_names:
        self.PreVariation(var_name, test_name)
        variation_data = test_data.get(var_name, test_data.values()[0])
        for cached in view_list:
          self.PreCached(cached, var_name, test_name)
          metrics = variation_data[cached]
          if not metrics:
            continue;
          for metric_name in metric_names:
            if metric_name not in metrics:
              value = []
            else:
              value = metrics[metric_name]
            self.Metric(metric_name, value, cached, var_name, test_name)
          self.PostCached(cached, var_name, test_name)
        self.PostVariation(var_name, test_name)
      self.PostTest(test_name)
      
    self.metrics = None
    return self.GetData()


class AggregateIterator(MetricIterator):
  """Creates an aggregate data set."""

  def __init__(self, data, filter):
    self.filter = filter
    self.variations = {}
    MetricIterator.__init__(self, data)

  def PreVariation(self, var_name, test_name):
    if var_name not in self.variations:
      self.variations[var_name] = [{}, {}]

  def Metric(self, metric_name, metric_value, cached, var_name,
             unused_test_name):
    if metric_name not in self.variations[var_name][cached]:
      self.variations[var_name][cached][metric_name] = []
    if not isinstance(metric_value, list):
      metric_value = [metric_value] # We use extend() which only works on lists
    metric_value = self.filter(metric_value)
    self.variations[var_name][cached][metric_name].extend(metric_value)

  def GetData(self):
    """Returns the aggregate data set.
    
    The data set is equivalent to the original except the only test is
    "Aggregate"
    """
    return {"Aggregate": self.variations}


class ViewDataIterator(MetricIterator):
  """Creates an aggregate data set."""

  def __init__(self, data, reprstat):
    self.reprstat = reprstat
    self.result = {}
    MetricIterator.__init__(self, data)

  def PreTest(self, unused_test_name):
    self.cur_test = [[], [], [], []]

  def PreCached(self, unused_cached, var_name, unused_test_name):
    self.cur_var = [str(var_name)]

  def Metric(self, metric_name, metric_value, cached, var_name,
             unused_test_name):
    value = self.reprstat(metric_value)
    if util.IsNumeric(metric_value):
      self.cur_var.append(value)
    else:
      value = (var_name, metric_name, self.reprstat(value))
      self.cur_test[cached + 2].append(value)

  def PostCached(self, cached, unused_var_name, unused_test_name):
    self.cur_test[cached].append(self.cur_var)

  def PostTest(self, test_name):
    self.result[str(test_name)] = self.cur_test

  def GetData(self):
    """
    Returns a dict {test: list of four lists}:
      0: List of First View Variations Numeric Metrics
          [variation name, *metric values]
      1: List of Repeat View Variations Numeric Metrics
          [variation name, *metric values]
      2: List of First View Variations String Metrics
          (variation name, metric name, value)
      3: List of Repeat View Variations String Metrics
          (variation name, metric name, value)
    """
    return self.result


class BasicStatIterator(MetricIterator):
  """Creates an aggregate data set."""

  def __init__(self, data, delta):
    self.delta = delta
    self.chartdata = [[], []]
    self.quartiles = {}
    MetricIterator.__init__(self, data)

  def PreTest(self, test_name):
    self.cur_test = [[str(test_name)], [str(test_name)]]
    self.quartiles[str(test_name)] = [[], []]

  def Metric(self, metric_name, metric_value, cached, var_name, test_name):
    reprvalue = self.delta(self.data, test_name, cached, metric_name, var_name)
    self.cur_test[cached].append(reprvalue)
    quartiles = [str(var_name)] + util.Quartiles(metric_value)
    self.quartiles[test_name][cached].append(quartiles)

  def PostTest(self, test_name):
    variations = len(self.data[test_name]) + 1
    if len(self.cur_test[0]) == variations:
      self.chartdata[0].append(self.cur_test[0])
    if len(self.cur_test[1]) == variations:
      self.chartdata[1].append(self.cur_test[1])

  def GetData(self):
    """Returns bar chart and quartile data.
    
    Bar Chart Data:
      list[0=fv,1=rv] = list of [test_name, *variation values]
    
    Quartile Data:
      dict[test_name][cached] = list of [*quartiles]
    """
    return self.chartdata, self.quartiles


class CorrelationIterator(MetricIterator):
  """Creates an aggregate data set."""

  def __init__(self, data, idelta, ddelta, imetric):
    self.idelta = idelta
    self.ddelta = ddelta
    self.imetric = imetric
    self.chartdata = [[], []]
    self.correlation = [{}, {}]
    MetricIterator.__init__(self, data)

  def PreCached(self, cached, unused_var_name, test_name):
    try:
      self.cur_var = [self.idelta(self.data, test_name, cached, self.imetric)]
    except KeyError:
      self.cur_var = [0]

  def Metric(self, metric_name, unused_metric_value, cached, unused_var_name,
             test_name):
    try:
      val = self.ddelta(self.data, test_name, cached, metric_name)
    except KeyError:
      return
    self.cur_var.append(val)
    if metric_name not in self.correlation[cached]:
      self.correlation[cached][metric_name] = []
    self.correlation[cached][metric_name].append((self.cur_var[0], val))

  def PostCached(self, cached, unused_var_name, test_name):
    if len(self.metrics) + 1 == len(self.cur_var):
      self.cur_var.append(test_name)
      self.chartdata[cached].append(self.cur_var)

  def GetData(self):
    """
    Returns scatter plot and correlation data.
    
    Scatter Plot Data:
      list[0=fv,1=rv] = list of [dependant metric, *independant metrics]
    
    Correlation Data:
      list[0=fv,1=rv][metric name] = (correlation coefficient (r))
    """
    correlation = [{}, {}]
    for cached in (0, 1):
      for metric in self.correlation[cached]:
        value = util.Correlation(self.correlation[cached][metric])
        correlation[cached][metric] = value
    return self.chartdata, correlation


class SignificanceIterator(MetricIterator):
  """Creates an aggregate data set."""

  def __init__(self, data, basevar, filter):
    self.basevar = basevar
    self.tabledata = [{}, {}]
    self.filter = filter
    MetricIterator.__init__(self, data)

  def Metric(self, metric_name, metric_value, cached, var_name, test_name):
    try:
      baseval = self.data[test_name][self.basevar][cached][metric_name]
    except KeyError:
      self.tabledata[cached][metric_name].append((test_name,
                               (-1,-1,-1), (-1,-1,-1)))
      return
    baseint = util.ConfInterval(self.filter(baseval))
    mainint = util.ConfInterval(self.filter(metric_value))
    if metric_name not in self.tabledata[cached]:
      self.tabledata[cached][metric_name] = []
    self.tabledata[cached][metric_name].append((test_name,
                                   baseint, mainint))

  def GetData(self):
    """
    Returns a set of confidence intervals.
    
    Table Data:
      list[0=fv,1=rv][metric] = list of (test, base ci, main ci)
    
    Chart Data:
      list[0=fv,1=rv] = list of [test, *|lowerbound, upperbound|]
    """
    chartdata = [[], []]
    tmpdata = [{}, {}]
    for cached in (0, 1):
      for metric in self.tabledata[cached]:
        for ci in self.tabledata[cached][metric]:
          if ci[0] not in tmpdata[cached]:
            tmpdata[cached][ci[0]] = [str(ci[0])]
          tmpdata[cached][ci[0]].extend([ci[1][0], ci[1][2]])
          tmpdata[cached][ci[0]].extend([ci[2][0], ci[2][2]])
    for cached in (0, 1):
      for test in tmpdata[cached]:
        chartdata[cached].append(tmpdata[cached][test])
    return self.tabledata, chartdata


class RunsIterator(MetricIterator):
  """"""

  def __init__(self, data):
    self.result = [{}, {}]
    self.runs = util.GetRuns(data)
    MetricIterator.__init__(self, data)

  def PreCached(self, cached, var_name, test_name):
    if var_name not in self.result[cached]:
      self.result[cached][var_name] = [["Run %d" %x] for x in range(self.runs)]

  def Metric(self, unused_metric_name, metric_value, cached, var_name,
             unused_test_name):
    for x in range(self.runs):
      if x >= len(metric_value):
        self.result[cached][var_name][x].append(-1)
        continue
      self.result[cached][var_name][x].append(metric_value[x])

  def GetData(self):
    """
    list[0=fv,1=rv][variation] = list of [label, *values]
    """
    return self.result