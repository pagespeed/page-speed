#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Utility functions."""

import math

D_S = 1  # Single
D_A = 2  # Aggregate
D_N = 4  # Numeric
D_AN = D_A | D_N


def IsAggregate(val):
  """Checks if the value is an aggregate type."""
  return isinstance(val, list)


def IsNumeric(val):
  """Checks if the value is a numeric type."""
  if isinstance(val, (float, int, long)):
    return True
  return IsAggregate(val) and val and IsNumeric(val[0])


def GetMetrics(data, data_type=0, withtypes=False, names=None):
  """Gets a list of metrics in a data set."""

  # Find the test variation with the largest set of metrics
  maxlength = -1
  metrics = []
  for test in data.values():
    items = test.values()[0][0].items()
    if len(items) > maxlength:
      metrics = items
      maxlength = len(items)

  if names is not None:
    metrics = filter(lambda (name, val): (name in names), metrics)
  if data_type & D_N:
    metrics = filter(lambda (name, val): IsNumeric(val), metrics)
  metrics.sort()
  if withtypes:
    # Used for Google chart Tools columns.
    result = []
    for item in metrics:
      result.append(("number" if IsNumeric(item[1]) else "string", item[0]))
    return result
  return map(lambda (name, val): name, metrics)


def GetVariations(data):
  """Gets a list of variations in a data set."""
  values = set()
  for test in data.values():
    values.update(test.keys())
  return list(values)


def GetRuns(data):
  """Gets the number of runs for each test."""
  maxlength = 0
  for test in data.values():
    val = len(test.values()[0][0].values()[0])
    if val > maxlength:
      maxlength = val
      break
  return maxlength


def GetTests(data):
  """Gets a list of tests in a data set."""
  keys = []
  runs = GetRuns(data)
  for test_name in data:
    if len(data[test_name].values()[0][0].values()[0]) >= runs:
      keys.append(test_name)
  return keys


def Mean(lst):
  if not lst:
    return 0
  return sum(lst) / len(lst)


def Median(lst):
  if not lst:
    return 0
  length = len(lst)
  lstcpy = lst[:]
  lstcpy.sort()
  if length % 2 == 1:
    return lstcpy[(length - 1) / 2]
  else:
    return (lstcpy[length / 2] + lstcpy[length / 2 - 1]) / 2


def Quartiles(values):
  """Calculates the 0, 25, 75 and 100 percentiles for a list of numbers.

  Args:
    values: The list of numbers

  Returns:
    A list of the [min, 25%, 75%, max] of the values if there are atleast
    four values, [min, 50%, max] otherwise.
  """
  if not isinstance(values, list):
    return [values, values, values, values]
  valcpy = values[:]
  valcpy.sort()
  length = len(valcpy)
  p0 = valcpy[0]
  p4 = valcpy[-1]
  if length < 4:
    return [p0, Median(valcpy), Median(valcpy), p4]
  left = valcpy[0:int(math.floor(length / 2))]
  right = valcpy[int(math.ceil(length / 2)):]
  p1 = Median(left)
  p3 = Median(right)
  return [p0, p1, p3, p4]


def Correlation(values):
  """Calculates the correlation coefficient (r) for a list of pairs.

  http://www.socialresearchmethods.net/kb/statcorr.php

  Args:
    values: A list of pairs.

  Returns:
    The correlation coefficient (r) of the data set.
  """
  n = len(values)
  exy = 0
  ex = ey = 0
  ex2 = ey2 = 0
  for x, y in values:
    exy += x * y
    ex += x
    ey += y
    ex2 += x ** 2
    ey2 += y ** 2
  num = n * exy - ex * ey
  den = math.sqrt((n * ex2 - (ex ** 2)) * (n * ey2 - (ey ** 2)))
  if not den:
    return 0
  return num / den


def StdDev(values):
  """Calculates the standard deviation of a set of values."""
  mean = Mean(values)
  variance = sum(map(lambda x: math.pow(mean - x, 2), values))
  variance /= (len(values) - 1)
  return math.sqrt(variance)


def ConfInterval(values):
  """Computes a 95% confidence interval for a set of data.

  One of these assumptions must apply:
    The population distribution is normal.
    The sampling distribution is symmetric, unimodal, without outliers, and
      the sample size is 15 or less.
    The sampling distribution is moderately skewed, unimodal, without
      outliers, and the sample size is between 16 and 40.
    The sample size is greater than 40, without outliers.

  Arguments:
    values: The list of values

  Returns:
    Triple (Lower bound, mean, Upper bound)
  """
  if not IsAggregate(values):
    return values, values, values
  stat = Mean(values)
  if len(values) < 2:
    return 0, stat, 0
  stddev = StdDev(values)
  stderr = stddev / math.sqrt(len(values))
  me = 1.96 * stderr
  return stat - me, stat, stat + me
