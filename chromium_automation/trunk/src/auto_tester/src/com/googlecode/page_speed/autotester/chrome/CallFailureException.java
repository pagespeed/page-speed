// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.chrome;

/**
 * This checked exception indicates that a synchronous call to a Chrome tab failed for some reason.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class CallFailureException extends Exception {

  public static final long serialVersionUID = 9823498234987L;

  public CallFailureException() {
    super();
  }

  public CallFailureException(String message) {
    super(message);
  }

}
