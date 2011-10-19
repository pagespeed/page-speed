// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.chrome;

import org.json.simple.JSONObject;

/**
 * A callback for an asynchronous call to a Chrome tab.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public interface AsyncCallback {

  /**
   * Called by <code>ChromeTab.asyncCall</code> with the JSON result object if and when the call
   * succeeds.
   */
  void onSuccess(JSONObject result);

  /**
   * Called by <code>ChromeTab.asyncCall</code> with an error message if the call fails for any
   * reason (including I/O errors).
   */
  void onError(String error);

}
