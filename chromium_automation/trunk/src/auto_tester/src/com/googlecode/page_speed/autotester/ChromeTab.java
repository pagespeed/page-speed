// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import java.net.URI;

/**
 * Represents a debuggable Chrome tab that we will connect to to perform tests.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class ChromeTab {

  // TODO(mdsteele): Move more functionality into this class (probably from
  //   TabController).

  private final URI uri;

  public ChromeTab(URI uri) {
    if (uri == null) {
      throw new IllegalArgumentException("uri must be non-null");
    }
    this.uri = uri;
  }

  /**
   * @return The URI used to connect to this Chrome tab.
   */
  public URI getURI() {
    return this.uri;
  }

  @Override
  public String toString() {
    return this.uri.toString();
  }

}
