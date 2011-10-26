// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.AsyncCallback;
import com.googlecode.page_speed.autotester.chrome.CallFailureException;
import com.googlecode.page_speed.autotester.chrome.NotificationListener;
import com.googlecode.page_speed.autotester.chrome.TabConnection;
import com.googlecode.page_speed.autotester.util.FileUtils;
import com.googlecode.page_speed.autotester.util.Json;
import com.googlecode.page_speed.autotester.util.JsonException;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

/**
 * The TestRunner class takes a single given TestRequest and asynchronously runs it to completion
 * (success or failure) over a specified TabConnection.
 */
public abstract class TestRunner {

  /**
   * A callback interface for when the TestRunner finishes its test.
   */
  public static interface Listener {
    /**
     * Called when the TestRunner finishes running its TestRequest.
     * @param testData the data collected from the test (whether successful or not)
     */
    void onTestCompleted(TestData testData);
  }

  /**
   * Start running a TestRequest over the given TabConnection.  This method returns quickly; the
   * data from the test is sent asynchronously to the given Listener.
   * @param connection the connection to the Chrome tab in which to run the test
   * @param testRequest the test to run
   * @param testTimeoutMillis the max time to run the test before timing out and failing
   * @param listener the listener to inform asynchronously when the test has finished
   */
  public abstract void startTest(TabConnection connection, TestRequest testRequest,
                                 long testTimeoutMillis, Listener listener);

  public static TestRunner newInstance() {
    return new TestRunner() {
      public void startTest(TabConnection connection, TestRequest testRequest,
                            long testTimeoutMillis, Listener listener) {
        RunnerImpl runner = new RunnerImpl(connection, testRequest, listener);
        runner.start(testTimeoutMillis);
      }
    };
  }

  private static class RunnerImpl implements NotificationListener {

    // Internal enum type for tracking the progress of the test.
    private static enum Phase {
      ABOUT_BLANK,   // 1) initializing by loading about:blank
          LOADING_PAGE,  // 2) loading page to be tested
          FETCHING_DOM,  // 3) page is loaded; fetching DOM tree
          FINISHING;     // 4) received DOM tree; waiting for remaining resources to be collected
    }

    private static final String REQUEST_ID_KEY = "requestId";

    private static final String DOM_COLLECTOR_SCRIPT;
    static {
      // We're trying to load a static resource file (domCollector.js).  First, try getting it
      // using the class loader.  This should work if we're running from a JAR file.
      ClassLoader classLoader = TestRunner.class.getClassLoader();
      InputStream stream = classLoader.getResourceAsStream("domCollector.js");
      if (stream != null) {
        DOM_COLLECTOR_SCRIPT = FileUtils.getContents(stream);
      } else {
        // The class loader approach won't work in some contexts, such as running `ant test` from
        // the command line; this fallback seems to work in such cases.
        DOM_COLLECTOR_SCRIPT = FileUtils.getContents(new File("res/domCollector.js"));
      }
    }

    private final TabConnection tab;
    private final TestRequest testRequest;
    private final TestData testData;
    private final Listener listener;
    private final Timer watchDogTimer = new Timer(true);  // true -> uses a daemon thread

    private Phase phase = Phase.ABOUT_BLANK;
    private String mainRequestId = null;  // requestId for the primary URL
    // The timestamp (in milliseconds) when we started the main page request (NaN is used as a
    // sentinel value until we fill this in):
    private double startTimestampMillis = Double.NaN;

    private RunnerImpl(TabConnection connection, TestRequest testRequest, Listener listener) {
      this.tab = connection;
      this.testRequest = testRequest;
      this.testData = new TestData(testRequest);
      this.listener = listener;
    }

    // Start the test running with the given timeout.
    private synchronized void start(long testTimeoutMillis) {
      this.tab.setNotificationListener(this);
      this.watchDogTimer.schedule(new TimerTask() {
          public void run() {
            RunnerImpl.this.expireTest();
          }
        }, testTimeoutMillis);
      // We start by loading about:blank, just to be sure we load the test URL in a predictable
      // way.
      this.startLoadingUrl("about:blank");
    }

    // Receive a notification message from the Chrome tab.
    @Override
    public synchronized void handleNotification(String method, JSONObject params) {
      try {
        if (this.phase == Phase.ABOUT_BLANK) {
          // Once we've loaded about:blank, we can begin the real test.
          if (method.equals("Page.loadEventFired")) {
            this.beginLoadingPage();
          }
        } else if (method.equals("Network.requestWillBeSent")) {
          if (this.mainRequestId == null) {
            this.mainRequestId = Json.getString(params, REQUEST_ID_KEY);
          }
          this.testData.addDataForHar(method, params);
        } else if (method.equals("Network.loadingFinished") ||
                   method.equals("Network.resourceLoadedFromMemoryCache")) {
          this.fetchResourceContent(Json.getString(params, REQUEST_ID_KEY));
        } else if (method.equals("Network.responseReceived")) {
          if (this.mainRequestId != null &&
              this.mainRequestId.equals(Json.getString(params, REQUEST_ID_KEY))) {
            this.startTimestampMillis = 1000.0 * Json.getDouble(Json.getObject(Json.getObject(
                params, "response"), "timing"), "requestTime");
            this.testData.setTimeToFirstByteMillis(
                1000.0 * Json.getDouble(params, "timestamp") - this.startTimestampMillis);
          }
        } else if (method.equals("Page.domContentEventFired")) {
          if (!Double.isNaN(this.startTimestampMillis)) {
            this.testData.setTimeToBasePageCompleteMillis(
                1000.0 * Json.getDouble(params, "timestamp") - this.startTimestampMillis);
          } else {
            this.abortTest("Saw Page.domContentEventFired before receiving main response");
          }
        } else if (method.equals("Page.loadEventFired")) {
          if (!Double.isNaN(this.startTimestampMillis)) {
            this.testData.setLoadTimeMillis(
                1000.0 * Json.getDouble(params, "timestamp") - this.startTimestampMillis);
            this.fetchDomTree();
          } else {
            this.abortTest("Saw Page.loadEventFired before receiving main response");
          }
        } else if (method.equals("Timeline.eventRecorded")) {
          this.testData.addTimelineEvent(Json.getObject(params, "record"));
        } else {
          this.testData.addDataForHar(method, params);
        }
      } catch (JsonException e) {
        this.abortTest("Bad JSON from Chrome: " + e.getMessage());
      }
    }

    @Override
    public void onConnectionError(String message) {
      this.abortTest("Tab connection error: " + message);
    }

    // Ask the tab to load the given URL.
    private synchronized void startLoadingUrl(String url) {
      // A previous version of Chrome had a "Page.load" method, but that has since been removed (at
      // least as of M16).  Simply evaluating "window.location.href=url" seems to work fine,
      // though.
      JSONObject params = new JSONObject();
      Json.put(params, "expression", "window.location.href=" + JSONValue.toJSONString(url));
      Json.put(params, "returnByValue", true);
      this.tab.asyncCall("Runtime.evaluate", params, new AsyncCallback() {
          public void onSuccess(JSONObject result) {}
          public void onError(String message) {
            RunnerImpl.this.abortTest(message);
          }
        });
    }

    // Enter the LOADING_PAGE phase; reset the state of the tab, and ask it to load the test URL.
    private synchronized void beginLoadingPage() {
      assert this.phase == Phase.ABOUT_BLANK;
      this.phase = Phase.LOADING_PAGE;

      try {
        if (!this.testRequest.isRepeatView()) {
          this.tab.syncCall("Network.clearBrowserCache");
          this.tab.syncCall("Network.clearBrowserCookies");
        }
        this.tab.syncCall("Timeline.start");
        this.tab.syncCall("Network.enable");
      } catch (CallFailureException e) {
        this.abortTest(e.getMessage());
        return;
      }

      this.startLoadingUrl(this.testRequest.getFullURL());
    }

    // Ask the Chrome tab to retrieve the contents of the specified resource.  The requestId object
    // is some key object given to us by the Chrome tab.
    private synchronized void fetchResourceContent(final String requestId) {
      JSONObject params = new JSONObject();
      Json.put(params, REQUEST_ID_KEY, requestId);
      this.tab.asyncCall("Network.getResourceContent", params, new AsyncCallback() {
          public void onSuccess(JSONObject response) {
            RunnerImpl.this.handleResourceContent(requestId, response);
          }
          public void onError(String message) {
            RunnerImpl.this.abortTest(message);
          }
        });
    }

    // Callback for fetchResourceContent().
    private synchronized void handleResourceContent(String requestId, JSONObject result) {
      try {
        this.testData.addResourceContent(requestId, Json.getString(result, "body"),
                                         Json.getBoolean(result, "base64Encoded"));
        this.checkIfDone();
      } catch (JsonException e) {
        this.abortTest("Bad JSON from Chrome: " + e.getMessage());
      }
    }

    // Enter the FETCHING_DOM phase; ask the Chrome tab to retrieve the DOM tree from the page.
    private synchronized void fetchDomTree() {
      assert this.phase == Phase.LOADING_PAGE;
      this.phase = Phase.FETCHING_DOM;

      JSONObject params = new JSONObject();
      Json.put(params, "expression", DOM_COLLECTOR_SCRIPT);
      Json.put(params, "returnByValue", true);

      this.tab.asyncCall("Runtime.evaluate", params, new AsyncCallback() {
          public void onSuccess(JSONObject response) {
            RunnerImpl.this.handleDomTree(response);
          }
          public void onError(String message) {
            RunnerImpl.this.abortTest(message);
          }
        });
    }

    // Callback for fetchDomTree().
    private synchronized void handleDomTree(JSONObject result) {
      assert this.phase == Phase.FETCHING_DOM;
      this.phase = Phase.FINISHING;
      try {
        this.testData.setDomString(
            Json.getString(Json.getObject(result, "result"), "description"));
        this.checkIfDone();
      } catch (JsonException e) {
        this.abortTest("Bad JSON from Chrome: " + e.getMessage());
      }
    }

    // Check whether we have collected all the data we need, and if so, end the test.
    private synchronized void checkIfDone() {
      if (this.phase == Phase.FINISHING && !this.tab.hasOutstandingCalls()) {
        this.endTest();
      }
    }

    // End this test as a failure, with the given error message.
    private synchronized void abortTest(String message) {
      this.testData.setFailure(message);
      this.endTest();
    }

    // Declare the test to be done, and inform the listener.
    private synchronized void endTest() {
      this.watchDogTimer.cancel();
      this.tab.setNotificationListener(null);
      this.testData.setCompleted();
      this.listener.onTestCompleted(this.testData);
    }

    // Called by the watchdog timer task if we fail to complete the test before reaching the
    // timeout.
    private synchronized void expireTest() {
      if (!this.testData.isCompleted()) {
        this.abortTest("timed out");
      }
    }

  }

}
