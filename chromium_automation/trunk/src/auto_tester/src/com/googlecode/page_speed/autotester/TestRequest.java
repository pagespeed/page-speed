// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Represents a single test (page load) that we want to perform.  This class is immutable, and
 * therefore thread-safe.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public class TestRequest {

  /**
   * Generates a list of lists of TestRequest objects from a specification of
   * the set of tests to be performed.  Tests are grouped by base URL; that is,
   * each element of the return value is a list of tests whose base URL is the
   * same.
   *
   * @param urls The list of URLs to be tested.
   * @param variations A map from variation names to variation query strings.
   * @param doRepeatView Whether to test repeat views in addition to first
   *   views of each URL/variation combination.
   * @param numberOfRuns How many repetitions of each URL/variation/repeat
   *   combiation to perform.
   * @return A list of lists of all TestReqests to perform.
   */
  public static List<List<TestRequest>> generateRequests(
      List<URL> urls,
      Map<String,String> variations,
      boolean doRepeatView,
      int numberOfRuns) {
    List<List<TestRequest>> testLists = new ArrayList<List<TestRequest>>(urls.size());

    for (URL url : urls) {
      List<TestRequest> testList = new ArrayList<TestRequest>(
          numberOfRuns * variations.size() * (doRepeatView ? 2 : 1));

      for (int run = 0; run < numberOfRuns; ++run) {
        for (Map.Entry<String,String> variation : variations.entrySet()) {
          testList.add(new TestRequest(url, variation.getKey(),
                                       variation.getValue(), run, 0));
          if (doRepeatView) {
            testList.add(new TestRequest(url, variation.getKey(),
                                         variation.getValue(), run, 1));
          }
        }
      }

      testLists.add(testList);
    }

    return testLists;
  }

  // TODO(mdsteele): Choose nicer names for these fields; possibly make them
  //   private and provide accessors.
  public final String url;  // TODO(mdsteele): s/String/URL/
  public final String varName;
  public final String varQS;
  public final int run;
  public final int first;

  /**
   * Creates a new test run storage object.
   *
   * @param aURL The base url to load (without variation)
   * @param aVN The variation name used in this test.
   * @param aVQS The variation query string.
   * @param aRun The run # of this test.
   * @param aView 0 for first view, 1 for repeat view
   */
  public TestRequest(URL aURL, String aVN, String aVQS, int aRun,
                     int aView) {
    this.url = aURL.toString();
    this.varName = aVN;
    this.varQS = aVQS;
    this.run = aRun;
    this.first = aView == 0 ? 0 : 1;
  }

  public String getDescription() {
    return String.format("%s - %s %s #%d", this.url, this.varName,
                         (this.isRepeatView() ? "Repeat" : "First"), this.run);
  }

  public int getRunNumber() {
    return this.run;
  }

  public boolean isRepeatView() {
    return this.first != 0;
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
        return new URI(urlParts.getScheme(), urlParts.getUserInfo(),
                       urlParts.getAuthority(), urlParts.getPort(),
                       urlParts.getPath(), qs,
                       urlParts.getFragment()).toString();
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
