#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Displays the main page for the app."""

from google.appengine.dist import use_library
use_library("django", "1.2")

from google.appengine.ext import webapp
from google.appengine.ext.webapp import template
from google.appengine.ext.webapp.util import run_wsgi_app

import config


class MainPage(webapp.RequestHandler):
  """Displays an upload screen."""

  def get(self):  # pylint: disable-msg=C6409
    template_values = {
    }
    path = "templates/index.html"
    self.response.out.write(template.render(path, template_values))


def main():
  application = webapp.WSGIApplication([
      ("/", MainPage),
  ], debug=config.DEBUG)

  run_wsgi_app(application)


if __name__ == "__main__":
  main()
