#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""A collection of GAE django template filters."""

from django.utils import simplejson as json
from google.appengine.ext import webapp

register = webapp.template.create_template_register()
def jsonify(object):
  """A filter that converts an object to a json string."""
  return json.dumps(object)
register.filter(jsonify)
