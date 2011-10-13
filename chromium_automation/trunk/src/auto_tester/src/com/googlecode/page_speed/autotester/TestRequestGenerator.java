package com.googlecode.page_speed.autotester;
import com.googlecode.page_speed.autotester.util.FileUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Generates a list of tests to perform.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class TestRequestGenerator {

  private final List<String> urls;
  private final Map<String, String> variations;

  private final int runs;
  private final boolean repeat;

  /**
   * Create a new configuration for the test runner.
   * @param urlFilename The filename of the URL list.
   * @param variationFilename The filename of the variation list.  If null, or
   *   if the file is empty, the TestRequestGenerator will instead use a
   *   single, default variation.
   * @param doRepeat Whether to do a repeat view (in addition to first view).
   * @param numRuns How many runs to do for each URL/variation/view combination
   */
  public TestRequestGenerator(String urlFilename,
                              String variationFilename,
                              boolean doRepeat,
                              int numRuns) {
    if (numRuns < 1) {
      throw new IllegalArgumentException("numRuns must be positive");
    }
    this.runs = numRuns;
    this.repeat = doRepeat;

    this.urls = FileUtils.getLines(new File(urlFilename));
    this.urls.removeAll(Arrays.asList(""));

    // Read the list of variations from the variations file, if any.
    this.variations = new HashMap<String, String>();
    if (variationFilename != null) {
      for (String variation : FileUtils.getLines(new File(variationFilename))) {
        String[] pair = variation.split("=", 2);
        if (pair.length == 2) {
          this.variations.put(pair[0], pair[1]);
        } else {
          this.variations.put(pair[0], pair[0]);
        }
      }
    }
    // If we weren't given a variations file (or if the file didn't specify any
    // variations at all), then use a single, default variation.
    if (this.variations.size() < 1) {
      this.variations.put("Default", "");
    }
  }

  /**
   * Gets the number of runs per test URL.
   * @return The number of runs per test URL.
   */
  public int getRuns() {
    return runs;
  }

  /**
   * Gets whether repeat views are tested.
   * @return True if repeat views are tested, false otherwise.
   */
  public boolean getRepeat() {
    return repeat;
  }

  /**
   * Gets a list of iterables of TestRun storage objects.
   * Tests are grouped by base URL.
   * @return A list of lists of all TestRuns to perform.
   */
  public List<List<TestRequest>> getTestIterable() {
    List<List<TestRequest>> testLists =
        new ArrayList<List<TestRequest>>(urls.size());
    for (String url : urls) {
      List<TestRequest> testList = new ArrayList<TestRequest>(
          runs * variations.size() * (repeat ? 2 : 1)
      );
      for (int run = 0; run < runs; run++) {
        for (Entry<String, String> var : variations.entrySet()) {
          testList.add(new TestRequest(url, var.getKey(), var.getValue(), run, 0));
          if (repeat) {
            testList.add(new TestRequest(url, var.getKey(), var.getValue(), run, 1));
          }
        }
      }
      testLists.add(testList);
    }
    return testLists;
  }

  /**
   * Calculates the total number of tests that will be run.
   * @return the total number of tests that will be run.
   */
  public long getTotalTests() {
    return ((long)urls.size() * (long)runs * (long)variations.size() *
            (repeat ? 2L : 1L));
  }
}
