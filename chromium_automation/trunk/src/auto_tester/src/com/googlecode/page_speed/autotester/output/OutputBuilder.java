// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.output;

import com.googlecode.page_speed.autotester.TestData;

import org.json.simple.JSONObject;

/**
 * Interface for output data builders.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public interface OutputBuilder {

  /**
   * Add a test result to the output.
   * @param testData The data collected from the test.
   * @param pagespeedResults The JSON output of Page Speed for the test run.
   */
  public void addTestResult(TestData testData, JSONObject pagespeedResults);

  /**
   * The file extension for the output format.
   * @return A file extension without the leading "."
   */
  public String getResultExtension();

  /**
   * The output of this builder.
   * @return The contents for the output file.
   */
  public String getResult();

}
