// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.TabConnection;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

/**
 * The TestBatchRunner class takes and runs a list of TestRequests that must be run in sequence in
 * the same Chrome tab (e.g. a first view of a page followed by a repeat view, or perhaps a
 * sequence of pages representing a user session).  If one of the tests in the batch fails, the
 * whole batch needs to retried from the beginning, which is also taken care of by the
 * TestBatchRunner.
 */
public abstract class TestBatchRunner {

  /**
   * A callback interface for reporting on the TestBatchRunner's progress.
   */
  public static interface Listener {
    /**
     * Called each time an individual test completes.  Note that this may be called multiple times
     * for a given test if the test fails and must be retried.
     * @param testData the data collected from this run of the test
     */
    void onTestCompleted(TestData testData);

    /**
     * Called once when the whole batch either finishes successfully or fails for the last time.
     * @param connection the TabConnection that was used by the TestBatchRunner
     * @param batchData the final data collected from tests in the batch
     */
    void onBatchCompleted(TabConnection connection, List<TestData> batchData);
  }

  /**
   * Get a concrete instance of a TestBatchRunner.
   */
  public static TestBatchRunner getInstance() {
    // TODO(mdsteele): Allow the caller to supply a custom implementation of TestRunner, so that we
    //   can mock it out and test the TestBatchRunner class.
    return new TestBatchRunner() {
      public void startBatch(TabConnection connection,
                             List<TestRequest> batchRequests,
                             int maxBatchRetries,
                             long testTimeoutMillis,
                             Listener listener) {
        RunnerImpl runner = new RunnerImpl(connection, batchRequests, maxBatchRetries,
                                           testTimeoutMillis, listener);
        runner.startNextTest();
      }
    };
  }

  /**
   * Start running a batch of tests over the given TabConnection.  This method returns quickly; the
   * data collected from the tests is sent asynchronously to the given Listener.
   * @param connection the connection to the Chrome tab in which to run the test
   * @param batchRequests the tests to run
   * @param maxBatchRetries the max number of times to retry the whole batch if a test fails
   * @param testTimeoutMillis the max time to run each test before timing out and failing
   * @param listener the listener to inform asynchronously as tests finish
   */
  public abstract void startBatch(TabConnection connection,
                                  List<TestRequest> batchRequests,
                                  int maxBatchRetries,
                                  long testTimeoutMillis,
                                  Listener listener);

  private static class RunnerImpl implements TestRunner.Listener {

    private final TabConnection connection;
    private final List<TestRequest> batchRequests;
    private final int maxBatchRetries;
    private final long testTimeoutMillis;
    private final Listener listener;

    private final List<TestData> batchData = new ArrayList<TestData>();
    private Queue<TestRequest> requestQueue = new LinkedList<TestRequest>();
    private int numBatchFailures = 0;

    private RunnerImpl(TabConnection connection,
                       List<TestRequest> batchRequests,
                       int maxBatchRetries,
                       long testTimeoutMillis,
                       Listener listener) {
      this.connection = connection;
      this.batchRequests = new ArrayList<TestRequest>(batchRequests);
      this.maxBatchRetries = maxBatchRetries;
      this.testTimeoutMillis = testTimeoutMillis;
      this.listener = listener;
    }

    // Start the next test in the batch; or, if we've done them all, then declare ourselves done.
    private synchronized void startNextTest() {
      if (this.requestQueue.isEmpty()) {
        this.done();
      } else {
        TestRequest testRequest = this.requestQueue.remove();
        TestRunner.startTest(this.connection, testRequest, this.testTimeoutMillis, this);
      }
    }

    // Called each time a test succeeds or fails.
    @Override
    public synchronized void onTestCompleted(TestData testData) {
      this.listener.onTestCompleted(testData);
      if (testData.isFailure()) {
        // The test failed, which means the batch failed.  Let's see if we have any retries left.
        ++this.numBatchFailures;
        if (this.numBatchFailures <= this.maxBatchRetries) {
          // Retry; start the batch over from the beginning.
          this.batchData.clear();
          this.requestQueue.clear();
          this.requestQueue.addAll(this.batchRequests);
          this.startNextTest();
        } else {
          // We're out of retries.  Fail all remaining tests in the batch and call it quits.
          this.batchData.add(testData);
          while (!this.requestQueue.isEmpty()) {
            TestData data = new TestData(this.requestQueue.remove());
            data.setFailure("batch failed");
            this.batchData.add(data);
          }
          this.done();
        }
      } else {
        // The test succeeded; save the data and keep going.
        this.batchData.add(testData);
        this.startNextTest();
      }
    }

    // We are done, so inform the listener.
    private synchronized void done() {
      assert this.batchData.size() == this.batchRequests.size();
      this.listener.onBatchCompleted(this.connection, this.batchData);
    }

  }

}
