// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.DataUtils;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Holds result data for a single test.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class TestResult {

  public final TestRequest request; // The request for this result.

  private Date startTime = null;
  private Date endTime = null;
  private boolean error = false;

  private List<JSONObject> data = null;

  public TestResult(TestRequest aRequest) {
    request = aRequest;
    startTime = new Date();
    data = new ArrayList<JSONObject>();
  }

  /**
   * Adds response data to the result.
   * @param value
   */
  public void addData(JSONObject value) {
    data.add(value);
  }

  /**
   * Gets all the response data.
   * @return A List of JSON response data.
   */
  public List<JSONObject> getData() {
    return data;
  }

  /**
   * Clears all the response data to expedite GC.
   */
  public void clearData() {
    data.clear();
  }

  /**
   * Sets the completion time to now.
   */
  public void setEnd() {
    endTime = new Date();
  }

  /**
   * Returns whether the test has been completed.
   * @return True is the test is complete, false otherwise.
   */
  public boolean isCompleted() {
    return endTime != null;
  }

  /**
   * Returns whether the test failed.
   * @return whether the test failed.
   */
  public boolean failed() {
    return error;
  }

  /**
   * Marks the test as failed.
   */
  public void setFailed() {
    clearData();
    error = true;
  }

  /**
   * Calculates how long the test took to run.
   * @return The length of the test (in seconds) if completed, time since test start otherwise.
   */
  public Long getTestDuration() {
    if (!isCompleted()) {
      return new Date().getTime() - startTime.getTime();
    }
    return endTime.getTime() - startTime.getTime();
  }

  /**
   * Generates metrics based on the raw data collected from chrome.
   *
   * @return A metric->value JSONObject
   */
  @SuppressWarnings("unchecked")
  public JSONObject getMetrics() {
    JSONObject metrics = new JSONObject();
    Object mainId = null;
    Double start = null;
    Object method;
    Object id;
    for (JSONObject obj : data) {
      if (obj != null && obj.containsKey("method")) {
        method = obj.get("method");
        id = DataUtils.getId(obj);
        if (mainId == null && method.equals("Network.requestWillBeSent")) {
          mainId = id;
        } else if (start == null && mainId != null
            && method.equals("Network.responseReceived") && id.equals(mainId)) {
          start = (Double) DataUtils.getByPath(obj, "params.response.timing.requestTime") * 1000;
          metrics.put("time_to_first_byte_ms",
            (Double) DataUtils.getByPath(obj, "params.timestamp") * 1000 - start);
        } else if (method.equals("Page.domContentEventFired") && start != null
            && !metrics.containsKey("time_to_base_page_complete_ms")) {
          metrics.put("time_to_base_page_complete_ms",
            (Double) DataUtils.getByPath(obj, "params.timestamp") * 1000 - start);
        } else if (method.equals("Page.loadEventFired") && start != null
            && !metrics.containsKey("load_time_ms")) {
          metrics.put("load_time_ms",
            (Double) DataUtils.getByPath(obj, "params.timestamp") * 1000 - start);
        }
      }
    }
    return metrics;
  }

  /**
   * Get the timeline instrumentation JSON used by pagespeed.
   *
   * @return A JSONArray of timeline event objects.
   */
  @SuppressWarnings("unchecked")
  public JSONArray getTimeline() {
    if (data.isEmpty()) {
      return null;
    }
    JSONArray timeline = new JSONArray();
    for (JSONObject obj : data) {
      if (DataUtils.isMethod(obj, "Timeline.eventRecorded")) {
        timeline.add(DataUtils.getByPath(obj, "params.record"));
      }
    }
    return timeline;
  }

  /**
   * Get the DOM JSON used by pagespeed.
   *
   * @return A json representation of the DOM.
   */
  public JSONObject getDOM() {
    if (data.isEmpty()) {
      return null;
    }
    for (JSONObject obj : data) {
      if (obj != null && obj.containsKey("documentUrl")) {
        return obj;
      }
    }
    return null;
  }

  /**
   * Get the HAR (HTTP Archive) used by pagespeed.
   *
   * @return The HTTP Archive JSONObject based on data collected from chrome.
   */
  public JSONObject getHAR() {
    if (data.isEmpty()) {
      return null;
    }
    return new HARCreator(this).build();
  }

}
