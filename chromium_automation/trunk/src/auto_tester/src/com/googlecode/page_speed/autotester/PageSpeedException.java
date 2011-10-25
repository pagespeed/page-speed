// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

/**
 * Indicates an error while invoking the Page Speed library: either an error starting and
 * connecting to the Page Speed subprocess, or else an error within the Page Speed library itself.
 */
@SuppressWarnings("serial")  // don't bother us about missing serialVersionUID
public class PageSpeedException extends Exception {

  public PageSpeedException() {
    super();
  }

  public PageSpeedException(String message) {
    super(message);
  }

  public PageSpeedException(Throwable cause) {
    super(cause);
  }

  public PageSpeedException(String message, Throwable cause) {
    super(message, cause);
  }

}
