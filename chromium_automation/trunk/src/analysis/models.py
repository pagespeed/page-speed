#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.
"""Data models used in the analysis GAE app."""

import zlib

from django.utils import simplejson as json
from google.appengine.ext import db

import util


class WPTResultData(db.BlobProperty):
  """Represents an uploaded test data set."""

  def make_value_from_datastore(self, value):  # pylint: disable-msg=C6409
    data = zlib.decompress(value)
    return data

  def get_value_for_datastore(self, model_instance):
    # pylint: disable-msg=C6409
    """Converts data to an internal format. Compresses the data."""
    data = super(WPTResultData, self).get_value_for_datastore(model_instance)
    data = self.ConvertData(data)
    return db.Blob(zlib.compress(data, 9))

  def ConvertData(self, value):
    """Validates data, making sure it is a valid result data structure."""
    try:
      data = json.loads(value)
    except ValueError:
      raise db.BadValueError("Not Valid JSON")
    if not util.GetVariations(data) or not util.GetMetrics(data):
      raise db.BadValueError("Invalid Data")
    return json.dumps(data, separators=(",", ":"))


class WPTResult(db.Model):
  user = db.UserProperty(indexed=True)
  date = db.DateTimeProperty(auto_now_add=True)
  content = WPTResultData(required=True)

  def GetResultData(self):
    return json.loads(self.content)
