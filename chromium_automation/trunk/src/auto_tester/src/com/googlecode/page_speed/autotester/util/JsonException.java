// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

/**
 * Indicates an error while using JSON values; either an error while parsing a string into a JSON
 * value, or a dynamic type error when extracting a member of a JSON object or array.
 */
@SuppressWarnings("serial")  // don't bother us about missing serialVersionUID
public class JsonException extends Exception {

  public JsonException() {
    super();
  }

  public JsonException(String message) {
    super(message);
  }

}
