package com.googlecode.page_speed.autotester.iface;

import com.googlecode.page_speed.autotester.TestRequest;

import org.json.simple.JSONObject;

// Copyright 2011 Google Inc. All Rights Reserved.

/**
 * Interface for output data builders.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public interface OutputBuilder {

  /**
   * Add a test run to the output.
   * @param run The TestRun object to add.
   * @param metrics The metrics on TestRun.getMetrics()
   * @param pagespeed The json output of pagespeed for the test run.
   */
  public void buildPart(TestRequest run,
                        JSONObject metrics, JSONObject pagespeed);
  
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
