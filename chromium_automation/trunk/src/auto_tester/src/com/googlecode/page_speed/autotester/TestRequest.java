// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * Represents the state of a single test (page load).
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class TestRequest {

  public final String url;
  public final String varName;
  public final String varQS;
  public final Integer run;
  public final Integer first;

  /**
   * Creates a new test run storage object.
   * 
   * @param aURL The base url to load (without variation)
   * @param aVN The variation name used in this test.
   * @param aVQS The variation query string.
   * @param aRun The run # of this test.
   * @param aView 0 for first view, 1 for repeat view
   */
  public TestRequest(String aURL, String aVN, String aVQS, Integer aRun,
      Integer aView) {
    url = aURL;
    varName = aVN;
    varQS = aVQS;
    run = aRun;
    first = aView == 0 ? 0 : 1;
  }

  /**
   * Get the full URL of the test (with variation).
   * 
   * @return A string of the full URL to load.
   */
  public String getFullURL() {
    if (!varQS.isEmpty()) {
      try {
        URI urlParts = new URI(url);
        String qs = urlParts.getQuery();
        if (qs != null) {
          qs += "&" + varQS;
        } else {
          qs = varQS;
        }
        return new URI(urlParts.getScheme(), urlParts.getUserInfo(), urlParts.getAuthority(),
          urlParts.getPort(), urlParts.getPath(), qs, urlParts.getFragment()).toString();
      } catch (URISyntaxException e) {
        // Fallback
        System.err.println("Failed parsing URL: " + url);
        String resURL = url;
        if (resURL.contains("?")) {
          resURL.replace("?", "?" + varQS);
        } else if (resURL.contains("#")) {
          resURL.replace("#", "?" + varQS + "#");
        } else {
          resURL += "?" + varQS;
        }
        return resURL;
      }
    }
    return url;
  }
  
}
