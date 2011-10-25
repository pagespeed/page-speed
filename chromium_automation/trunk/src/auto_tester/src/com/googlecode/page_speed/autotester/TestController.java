// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.ChromeTab;
import com.googlecode.page_speed.autotester.chrome.TabConnection;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * The TestController class takes and runs a list of batches of tests, sharding them out onto one
 * or more Chrome tabs.
 */
public abstract class TestController {

  /**
   * A callback interface for reporting on the TestController's progress.  Implementations are
   * expected to be thread-safe.
   */
  public static interface Listener {
    /**
     * Called each time an individual test completes.  Note that this may be called multiple times
     * for a given test if the test fails and must be retried.
     * @param testData the data collected from this run of the test
     */
    void onTestCompletedOnce(TestData testData);

    /**
     * Called once for each test when it completes for the last time (no more retries).
     * @param testData the final data collected from the test
     */
    void onTestTotallyDone(TestData testData);

    /**
     * Called once, at the end, when all tests are completely done running.
     */
    void onAllTestsCompleted();
  }

  /**
   * Start running a suite of tests over the given pool of TabConnections.  The
   * data collected from the tests is sent asynchronously to the given
   * Listener, but this method blocks until all tests have been completed and
   * the data sent to the listener.  This will close all the connections before returning.
   * @param connections the connections to the Chrome tabs in which to run the tests
   * @param testBatches the batches of tests to run
   * @param maxBatchRetries the max number of times to retry each batch if a test fails
   * @param testTimeoutMillis the max time to run each test before timing out and failing
   * @param listener the listener to inform asynchronously as tests finish
   */
  public abstract void runTests(List<? extends TabConnection> connections,
                                List<? extends List<TestRequest>> testBatches,
                                int maxBatchRetries,
                                long testTimeoutMillis,
                                Listener listener) throws InterruptedException;

  /**
   * Get a concrete instance of a TestController that uses a standard TestBatchRunner for running
   * test batches.
   */
  public static TestController newInstance() {
    return TestController.newInstance(TestBatchRunner.newInstance());
  }

  /**
   * Get a concrete instance of a TestController that uses the given TestBatchRunner for running
   * test batches.
   */
  public static TestController newInstance(final TestBatchRunner batchRunner) {
    return new TestController() {
      public void runTests(List<? extends TabConnection> connections,
                           List<? extends List<TestRequest>> testBatches,
                           int maxBatchRetries,
                           long testTimeoutMillis,
                           Listener listener) throws InterruptedException {
        ControllerImpl controller =
            new ControllerImpl(batchRunner, connections, testBatches, listener);
        controller.run(maxBatchRetries, testTimeoutMillis);
      }
    };
  }

  private static class ControllerImpl implements TestBatchRunner.Listener {

    // Note that we use thread-safe classes here rather than making the methods of this class
    // synchronized; we can't make e.g. both run() and onBatchCompleted() synchronized, because the
    // latter must be called (a number of times) while the former is being called.
    private final TestBatchRunner batchRunner;
    private final BlockingQueue<TabConnection> tabPool;
    private final ConcurrentLinkedQueue<List<TestRequest>> testBatchQueue;
    private final Listener listener;
    private final CountDownLatch numBatchesRemaining;

    private ControllerImpl(TestBatchRunner batchRunner,
                           List<? extends TabConnection> connections,
                           List<? extends List<TestRequest>> testBatches,
                           Listener listener) {
      this.batchRunner = batchRunner;
      if (connections.isEmpty()) {
        throw new IllegalArgumentException("must provide at least one TabConnection");
      }
      this.tabPool = new LinkedBlockingQueue<TabConnection>(connections);
      this.testBatchQueue = new ConcurrentLinkedQueue<List<TestRequest>>(testBatches);
      this.listener = listener;
      this.numBatchesRemaining = new CountDownLatch(this.testBatchQueue.size());
    }

    private void run(int maxBatchRetries, long testTimeoutMillis) throws InterruptedException {
      // Farm out all test batches to tabs.
      List<TestRequest> batchRequests;
      while ((batchRequests = testBatchQueue.poll()) != null) {
        TabConnection connection = this.tabPool.take();
        batchRunner.startBatch(connection, batchRequests, maxBatchRetries,
                               testTimeoutMillis, this);
      }

      // Wait for all test batches to come back.
      this.numBatchesRemaining.await();

      // Close all TabConnections.
      TabConnection connection;
      while ((connection = tabPool.poll()) != null) {
        connection.close();
      }

      // And, we're done!
      this.listener.onAllTestsCompleted();
    }

    @Override
    public void onTestCompleted(TestData testData) {
      this.listener.onTestCompletedOnce(testData);
    }

    @Override
    public void onBatchCompleted(TabConnection connection, List<TestData> batchData) {
      // Get the connection back into the pool right away, so the run() method can get started on
      // the next test.
      this.tabPool.add(connection);

      // Report the final data from this batch of tests.
      for (TestData testData : batchData) {
        this.listener.onTestTotallyDone(testData);
      }

      // Only after the data has been recorded do we declare this batch done, potentially allowing
      // the run() method to return.
      this.numBatchesRemaining.countDown();
    }

  }

}
