// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.MockTabConnection;
import com.googlecode.page_speed.autotester.util.Json;

import junit.framework.TestCase;
import org.json.simple.JSONObject;

import java.net.URL;

/**
 * Unit tests for <code>TestRunner</code>.
 */
public class TestRunnerTest extends TestCase {

  private static class Listener implements TestRunner.Listener {
    public TestData testData = null;
    public void onTestCompleted(TestData testData) {
      this.testData = testData;
    }
  }

  public void testFirstView() throws Exception {
    TestRequest testRequest = new TestRequest(new URL("http://www.example.com/"),
                                              "Default", "", 0, 0);
    Listener listener = new Listener();

    MockTabConnection connection = new MockTabConnection()
        .expectCall("Runtime.evaluate",
                    "{\"expression\":\"window.location.href=\\\"about:blank\\\"\"," +
                    "\"returnByValue\":true}").willRespond("{}")
        .willNotify("Page.loadEventFired")
        .expectCall("Network.clearBrowserCache").willRespond("{}")
        .expectCall("Network.clearBrowserCookies").willRespond("{}")
        .expectCall("Timeline.start").willRespond("{}")
        .expectCall("Network.enable").willRespond("{}")
        .expectCall("Runtime.evaluate",
                    "{\"expression\":\"window.location.href=\\\""+
                    "http:\\\\\\/\\\\\\/www.example.com\\\\\\/\\\"\"," +
                    "\"returnByValue\":true}")
        .willNotify("Network.requestWillBeSent", "{\"requestId\":\"mainpage\"}")
        .willNotify("Network.responseReceived",
                    "{\"requestId\":\"mainpage\",\"timestamp\":1.75," +
                    "\"response\":{\"timing\":{\"requestTime\":1.0}}}")
        .willNotify("Timeline.eventRecorded", "{\"record\":{\"timeline\":true}}")
        .willRespond("{}")
        .willNotify("Page.domContentEventFired", "{\"timestamp\":2.0}")
        .willNotify("Page.loadEventFired", "{\"timestamp\":2.125}")
        .expectCall("Runtime.evaluate")  // fetching DOM
        .willNotify("Timeline.eventRecorded", "{\"record\":{\"answer\":42}}")
        .willRespond("{\"result\":{\"description\":\"{\\\"dom\\\":true}\"}}");

    TestRunner.newInstance().startTest(connection, testRequest, 5000, listener);

    assertNotNull("Test data not recorded", listener.testData);
    if (listener.testData.isFailure()) {
      fail("testData is failure: " + listener.testData.getFailureMessage());
    }
    connection.doneMakingCalls();
    assertTrue(listener.testData.isCompleted());
    assertEquals("{\"dom\":true}", listener.testData.getDomString());
    assertEquals("[{\"timeline\":true},{\"answer\":42}]", listener.testData.getTimelineString());

    JSONObject metrics = listener.testData.getMetrics();
    assertEquals(750.0, Json.getDouble(metrics, "time_to_first_byte_ms"));
    assertEquals(1000.0, Json.getDouble(metrics, "time_to_base_page_complete_ms"));
    assertEquals(1125.0, Json.getDouble(metrics, "load_time_ms"));
  }

  public void testFailure() throws Exception {
    TestRequest testRequest = new TestRequest(new URL("http://www.example.com/"),
                                              "Default", "", 0, 1);
    Listener listener = new Listener();

    MockTabConnection connection = new MockTabConnection()
        .expectCall("Runtime.evaluate",
                    "{\"expression\":\"window.location.href=\\\"about:blank\\\"\"," +
                    "\"returnByValue\":true}").willRespond("{}")
        .willNotify("Page.loadEventFired")
        .expectCall("Timeline.start").willRespond("{}")
        .expectCall("Network.enable").willRespond("{}")
        .expectCall("Runtime.evaluate",
                    "{\"expression\":\"window.location.href=\\\""+
                    "http:\\\\\\/\\\\\\/www.example.com\\\\\\/\\\"\"," +
                    "\"returnByValue\":true}").willRespond("{}")
        .willNotify("Network.requestWillBeSent", "{\"requestId\":\"mainpage\"}")
        .willNotify("Network.responseReceived",
                    "{\"requestId\":\"mainpage\",\"timestamp\":1.75," +
                    "\"response\":{\"timing\":{\"requestTime\":1.0}}}")
        .willNotify("Page.domContentEventFired", "{\"timestamp\":2.0}")
        .willNotify("Page.loadEventFired", "{\"timestamp\":2.125}")
        .expectCall("Runtime.evaluate")  // fetching DOM
        .willError("No DOM for you!");

    TestRunner.newInstance().startTest(connection, testRequest, 5000, listener);
    connection.doneMakingCalls();

    assertNotNull("Test data not recorded", listener.testData);
    assertTrue(listener.testData.isCompleted());
    assertTrue(listener.testData.isFailure());
    assertEquals("No DOM for you!", listener.testData.getFailureMessage());
  }

}
