package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.iface.TestObserver;

import org.chromium.sdk.internal.websocket.WsConnection;
import org.json.simple.JSONObject;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Coordinates running of tests and loading of URLs across multiple tabs.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class URLTester extends TestObservable implements TestObserver {


  private ListIterator<List<TestRequest>> pendingTests;
  private Map<TabController, ListIterator<TestRequest>> assignedTests;
  private Map<TestResult, TabController> currentTests; // In progress

  private Map<String, Integer> failedUrls;
  private int maxFailsBeforeAbort = 4;
  private int retriesOnFailure = 1;

  private BlockingQueue<TabController> tabQueue = null;

  /**
   * Creates a new URLTester object.
   *
   * @param aTests An iterator over TestRuns.
   */
  public URLTester(List<List<TestRequest>> aTests) {
    this.pendingTests = aTests.listIterator();
    this.assignedTests = new HashMap<TabController, ListIterator<TestRequest>>();
    this.currentTests = new HashMap<TestResult, TabController>();
    this.failedUrls = new HashMap<String, Integer>();
    this.tabQueue = new LinkedBlockingQueue<TabController>();
  }

  /**
   * Adds a tab to be used for testing.
   *
   * @param tabInfo The JSON tab description from getTabList()
   * @param aTimeoutConnect How long to wait to connect to the tab before
   *        aborting.
   * @param aTimeoutTest How long to wait for test completion before aborting.
   * @return True on success, false on failure.
   */
  public boolean addTabConnection(JSONObject tabInfo, int aTimeoutConnect, Long aTimeoutTest) {
    if (tabInfo.containsKey("webSocketDebuggerUrl")) {
      try {
        URI uri = new URI((String) tabInfo.get("webSocketDebuggerUrl"));
        InetSocketAddress addr = new InetSocketAddress(uri.getHost(), uri.getPort());

        WsConnection tab = WsConnection.connect(addr, aTimeoutConnect, uri.getPath(), "none", null);

        TabController socketListener = new TabController(tab, aTimeoutTest);
        socketListener.addObserver(this);
        tab.startListening(socketListener);
        tabQueue.add(socketListener);
        return true;
      } catch (URISyntaxException e) {
        System.err.println("Chrome returned an invalid URL.");
      } catch (IOException e) {
        System.err.println("Failed connecting to " + tabInfo.get("webSocketDebuggerUrl"));
      }
    } else {
      System.err.println("Tab is not debuggable. Another client is likely already debugging it.");
    }
    return false;
  }

  /**
   * Sets the maximum number of failures (excluding retries) before aborting the URL.
   * @param maxURLFailsBeforeAbort the maxURLFailsBeforeAbort to set
   */
  public void setMaxURLFailsBeforeAbort(int maxURLFailsBeforeAbort) {
    this.maxFailsBeforeAbort = maxURLFailsBeforeAbort;
  }

  /**
   * Sets the maximum number of retries before assuming a failure.
   * @param retriesOnFailure the retriesOnFailure to set
   */
  public void setRetriesOnFailure(int retriesOnFailure) {
    this.retriesOnFailure = retriesOnFailure;
  }

  /**
   * Starts loading and testing URLs. Attempts to load repeat views directly
   * after first views on the same server.
   */
  public void start() {
    if (tabQueue.isEmpty()) {
      System.err.println("No tabs connected.");
      return;
    }

    List<TabController> idleTabs = new ArrayList<TabController>();
    while (pendingTests.hasNext() || !assignedTests.isEmpty()) {
      TabController tc;
      try {
        tc = tabQueue.take();
      } catch (InterruptedException e) {
        e.printStackTrace();
        continue;
      }
      ListIterator<TestRequest> testIterator = null;

      // Check if tab already has assigned tests
      if (assignedTests.containsKey(tc)) {
        testIterator = assignedTests.get(tc);
        if (!testIterator.hasNext()) {
          assignedTests.remove(tc);
          testIterator = null;
        }
      }

      // If not, attempt to assign it some
      if (testIterator == null) {
        if (pendingTests.hasNext()) {
          testIterator = pendingTests.next().listIterator();
          assignedTests.put(tc, testIterator);
        } else {
          // This tab has no more tests it can run.
          // Move it to the idle list and go onto the next tab.
          idleTabs.add(tc);
          continue;
        }
      }

      // Run the test
      TestRequest test = testIterator.next();
      System.out.println(String
        .format("Starting Test: %s - %s #%d-%d", test.url, test.varName, test.run, test.first));
      TestResult testResult = new TestResult(test);
      currentTests.put(testResult, tc); // Map TabController -> TestRun
      tc.setTest(testResult);
    }

    // Return all idle TabControllers to the queue
    for (TabController tc : idleTabs) {
      tabQueue.offer(tc);
    }
  }

  /**
   * Closes all chrome debugging connections.
   */
  public void stop() {
    TabController tc = null;
    while (tabQueue.size() > 0) {
      try {
        tc = tabQueue.take();
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      tc.getTab().close();
    }
  }

  /**
   * Gets notified when a URL has completed loading and being tested.
   */
  @Override
  public void onTestCompleted(TestResult result) {
    TabController tc = currentTests.remove(result);
    try {
      notifyTestCompleted(result);
    } catch (NullPointerException e) {
      e.printStackTrace();
    }

    if (result.failed()) {
      // Increase the fail count for this base URL
      Integer failures = 1;
      if (failedUrls.containsKey(result.request.url)) {
        failures = failedUrls.get(result.request.url) + 1;
      }
      failedUrls.put(result.request.url, failures);

      if (failures <= retriesOnFailure) {
        // We will retry a URL incase it is a rare occurrence.
        // This goes in front so we catch tests which are run once and run last.
        assignedTests.get(tc).previous();
        result.clearData();
      } else if (failures > maxFailsBeforeAbort + retriesOnFailure) {
        // If we get > maxURLFailsBeforeAbort failures (excluding retries),
        // we abort all those tests.
        assignedTests.remove(tc);
        failedUrls.remove(result.request.url);
      }
    }

    if (assignedTests.get(tc) != null && !assignedTests.get(tc).hasNext()) {
      // If the last test succeeded, free this worker.
      assignedTests.remove(tc);
    }

    // Give back the worker to the pool.
    tabQueue.offer(tc);

    // We want to wait until all tests are fully complete, so we need to check
    // for any currently running tests or any assigned/scheduled tests.
    if (!pendingTests.hasNext() && assignedTests.isEmpty() && currentTests.isEmpty()) {
      notifyAllTestsCompleted();
      URLTester.this.stop();
    }

  }

  @Override
  public void onAllTestsCompleted() {
    // Worker threads can't know if all tests are complete.
  }

}
