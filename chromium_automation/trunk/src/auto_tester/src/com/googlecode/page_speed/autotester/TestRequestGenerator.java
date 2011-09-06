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

  private List<String> urls;
  private Map<String, String> variations;

  private int runs = 1;
  private boolean repeat = true;

  /**
   * Create a new configuration for the test runner.
   * @param urlFilename The filename of the URL list.
   * @param variationFilename The filename of the variation list.
   */
  public TestRequestGenerator(String urlFilename,
                    String variationFilename) {

    urls = FileUtils.getLines(new File(urlFilename));
    urls.removeAll(Arrays.asList(""));

    variations = new HashMap<String, String>();
    if (variationFilename != null) {
      for (String variation : FileUtils.getLines(new File(variationFilename))) {
        String[] pair = variation.split("=", 2);
        if (pair.length == 2) {
          variations.put(pair[0], pair[1]);
        } else {
          variations.put(pair[0], pair[0]);
        }
      }
    }
    variations.put("Default", "");
  }

  /**
   * Gets the number of runs per test URL.
   * @return The number of runs per test URL.
   */
  public int getRuns() {
    return runs;
  }

  /**
   * Sets the number of runs per test URL.
   * @param newRuns The new number of runs per test URL.
   */
  public void setRuns(int newRuns) {
    if (runs <= 0) {
      System.err.println(String.format(
        "Warning: Ignoring non-positive runs %d.", newRuns));
      return;
    }
    runs = newRuns;
  }

  /**
   * Gets whether repeat views are tested.
   * @return True if repeat views are tested, false otherwise.
   */
  public boolean getRepeat() {
    return repeat;
  }

  /**
   * Sets whether repeat views are tested.
   * @param newRepeat True if repeat views should be tested, false otherwise.
   */
  public void setRepeat(boolean newRepeat) {
    repeat = newRepeat;
  }

  /**
   * Gets a list of iterables of TestRun storage objects.
   * Tests are grouped by base URL.
   * @return A list of lists of all TestRuns to perform.
   */
  public List<List<TestRequest>> getTestIterable() {
    List<List<TestRequest>> testLists = new ArrayList<List<TestRequest>>(urls.size());
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
  public Long getTotalTests() {
    return (long) (urls.size() * runs * variations.size() * (repeat ? 2 : 1));
  }
}
