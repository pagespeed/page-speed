#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Uploads files to the app data store."""

from google.appengine.dist import use_library
use_library('django', '1.2')

import config
import models

import urllib

from google.appengine.api import users
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app


class UploadResult(webapp.RequestHandler):
  """Uploads the result file to the WPTResult data store."""
  def post(self):  # pylint: disable-msg=C6409
    result_file = self.request.get("result_file")

    try:
      result = models.WPTResult(content=result_file)
      if users.get_current_user():
        result.user = users.get_current_user()
      result.put()
      result_id = result.key().id()

      self.redirect("/results?%s" % urllib.urlencode({"id": result_id}))
    except (db.BadValueError):
      self.response.out.write("Invalid Data")


def main():
  application = webapp.WSGIApplication([
      ("/upload/results", UploadResult),
  ], debug=config.DEBUG)

  run_wsgi_app(application)


if __name__ == "__main__":
  main()
