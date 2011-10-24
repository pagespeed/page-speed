// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.Json;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.util.Date;

/**
 * Holds data collected for a single test, before that data has been run through Page Speed to
 * produce results.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class TestData {

  private final TestRequest testRequest;
  private final Date startTime;
  private Date endTime = null;
  private String failureMessage = null;

  private String domString = null;
  private JSONArray timelineData = new JSONArray();

  public TestData(TestRequest testRequest) {
    this.testRequest = testRequest;
    this.startTime = new Date();
  }

  ////// TEST STATUS /////

  /**
   * Return true if this test has been completed (whether successful or not).
   */
  public boolean isCompleted() {
    return this.endTime != null;
  }

  /**
   * Mark the test as having been completed.
   */
  public void setCompleted() {
    if (this.endTime != null) {
      throw new RuntimeException("must not call setCompleted() more than once");
    }
    this.endTime = new Date();
  }

  /**
   * Get the time taken, in milliseconds, to run this test.  The test must be marked completed
   * before calling this method.
   */
  public long getTestDurationMillis() {
    if (this.endTime != null) {
      throw new RuntimeException("setCompleted() has not yet been called");
    }
    return this.endTime.getTime() - this.startTime.getTime();
  }

  /**
   * Return whether the test failed.
   * @return true if the test failed, false if it was successful or is not done yet.
   */
  public boolean isFailure() {
    return this.failureMessage != null;
  }

  /**
   * Get the message explaining why the test failed, or null if the test hasn't failed.
   */
  public String getFailureMessage() {
    return this.failureMessage;
  }

  /**
   * Mark the test as having failed.  Note that this doesn't also mark the test as completed; for
   * that, call <code>setCompleted()</code>.
   * @param message the error message to use
   */
  public void setFailure(String message) {
    this.failureMessage = message;
  }

  ///// DOM /////

  public void setDomString(String domString) {
    this.domString = domString;
  }

  /**
   * Get the DOM data as a string, ready to be written to a file and passed into Page Speed.
   */
  public String getDomString() {
    return this.domString;
  }

  ///// TIMELINE /////

  /**
   * Append a single event record to the timeline data.
   */
  public void addTimelineEvent(JSONObject record) {
    Json.add(this.timelineData, record);
  }

  /**
   * Get the timeline data as a string, ready to be written to a file and passed into Page Speed.
   */
  public String getTimelineString() {
    return this.timelineData.toJSONString();
  }

  ///// HAR /////

  public void addResourceContent(String requestId, String body, boolean isBase64Encoded) {
    // TODO(mdsteele): Implement this.
  }

  // TODO(mdsteele): Add methods for recording other relevant network events needed for generating
  //   HAR data.

}
