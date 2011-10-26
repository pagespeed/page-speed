// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.Json;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Holds data collected for a single test, before that data has been run through Page Speed to
 * produce results.  This class is thread-safe.
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
  private List<JSONObject> dataForHar = new ArrayList<JSONObject>();
  private double timeToFirstByteMillis = Double.POSITIVE_INFINITY;
  private double timeToBasePageCompleteMillis = Double.POSITIVE_INFINITY;
  private double loadTimeMillis = Double.POSITIVE_INFINITY;

  public TestData(TestRequest testRequest) {
    this.testRequest = testRequest;
    this.startTime = new Date();
  }

  public synchronized TestRequest getTestRequest() {
    return this.testRequest;
  }

  ////// TEST STATUS /////

  /**
   * Return true if this test has been completed (whether successful or not).
   */
  public synchronized boolean isCompleted() {
    return this.endTime != null;
  }

  /**
   * Mark the test as having been completed.
   */
  public synchronized void setCompleted() {
    if (this.endTime != null) {
      throw new RuntimeException("must not call setCompleted() more than once");
    }
    this.endTime = new Date();
  }

  /**
   * Get the time taken, in milliseconds, to run this test.  The test must be marked completed
   * before calling this method.
   */
  public synchronized long getTestDurationMillis() {
    if (this.endTime == null) {
      throw new RuntimeException("setCompleted() has not yet been called");
    }
    return this.endTime.getTime() - this.startTime.getTime();
  }

  /**
   * Return whether the test failed.
   * @return true if the test failed, false if it was successful or is not done yet.
   */
  public synchronized boolean isFailure() {
    return this.failureMessage != null;
  }

  /**
   * Get the message explaining why the test failed, or null if the test hasn't failed.
   */
  public synchronized String getFailureMessage() {
    return this.failureMessage;
  }

  /**
   * Mark the test as having failed.  Note that this doesn't also mark the test as completed; for
   * that, call <code>setCompleted()</code>.
   * @param message the error message to use
   */
  public synchronized void setFailure(String message) {
    this.failureMessage = message;
  }

  ///// METRICS /////

  // TODO(mdsteele): Make a cleaner API than this (will need to change some other classes).
  public synchronized JSONObject getMetrics() {
    JSONObject obj = new JSONObject();
    Json.put(obj, "time_to_first_byte_ms", this.timeToFirstByteMillis);
    Json.put(obj, "time_to_base_page_complete_ms", this.timeToBasePageCompleteMillis);
    Json.put(obj, "load_time_ms", this.loadTimeMillis);
    return obj;
  }

  public synchronized void setTimeToFirstByteMillis(double millis) {
    this.timeToFirstByteMillis = millis;
  }

  public synchronized void setTimeToBasePageCompleteMillis(double millis) {
    this.timeToBasePageCompleteMillis = millis;
  }

  public synchronized void setLoadTimeMillis(double millis) {
    this.loadTimeMillis = millis;
  }

  ///// DOM /////

  public synchronized void setDomString(String domString) {
    this.domString = domString;
  }

  /**
   * Get the DOM data as a string, ready to be written to a file and passed into Page Speed.
   */
  public synchronized String getDomString() {
    return this.domString;
  }

  ///// TIMELINE /////

  /**
   * Append a single event record to the timeline data.
   */
  public synchronized void addTimelineEvent(JSONObject record) {
    Json.add(this.timelineData, record);
  }

  /**
   * Get the timeline data as a string, ready to be written to a file and passed into Page Speed.
   */
  public synchronized String getTimelineString() {
    return this.timelineData.toJSONString();
  }

  ///// HAR /////

  public synchronized String getHarString() {
    return new HarGenerator(this).build().toJSONString();
  }

  // TODO(mdsteele): Make a cleaner API than this (will need to change HARCreator).
  public synchronized List<JSONObject> getDataForHar() {
    return this.dataForHar;
  }

  // TODO(mdsteele): Make a cleaner API than this (will need to change TestRunner).
  public synchronized void addDataForHar(String methodName, JSONObject params) {
    JSONObject obj = new JSONObject();
    Json.put(obj, "method", methodName);
    Json.put(obj, "params", params);
    this.dataForHar.add(obj);
  }

  public synchronized void addResourceContent(String requestId, String body,
                                              boolean isBase64Encoded) {
    JSONObject result = new JSONObject();
    Json.put(result, "content", body);
    Json.put(result, "base64encoded", isBase64Encoded);
    JSONObject obj = new JSONObject();
    Json.put(obj, "method", "__Internal.resourceContent");
    Json.put(obj, "result", result);
    this.dataForHar.add(obj);
  }

}
