// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.chrome.MockTabConnection;
import com.googlecode.page_speed.autotester.chrome.TabConnection;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import junit.framework.TestCase;

import java.net.URL;
import java.util.List;

/**
 * Unit tests for <code>TestBatchRunner</code>.
 */
public class TestBatchRunnerTest extends TestCase {

  // Simple listener that saves all arguments passed to it.
  private static class MockListener implements TestBatchRunner.Listener {
    public List<TestData> testsData = Lists.newArrayList();
    private boolean batchWasCompleted = false;
    public TabConnection tabConnection = null;
    public List<TestData> batchData = null;

    @Override
    public synchronized void onTestBeginning(TestRequest testRequest) {}

    @Override
    public synchronized void onTestCompleted(TestData testData) {
      if (this.batchWasCompleted) {
        throw new RuntimeException("Batch was already completed");
      }
      this.testsData.add(testData);
    }

    @Override
    public synchronized void onBatchCompleted(TabConnection connection, List<TestData> testData) {
      if (this.batchWasCompleted) {
        throw new RuntimeException("Batch was already completed");
      }
      this.batchWasCompleted = true;
      this.tabConnection = connection;
      this.batchData = testData;
      this.notifyAll();
    }

    // Waits until onBatchCompleted() has been called.
    public synchronized void waitUntilDone() throws InterruptedException {
      while (!this.batchWasCompleted) {
        this.wait();
      }
    }
  }

  private static class MockTestRunner extends TestRunner {
    public static final String FAILURE_MESSAGE = "Oh no!  This test failed!";

    private final int failNth;
    private int numTestsRun = 0;

    // Creates a MockTestRunner that will simulate success on all tests run on it.
    public MockTestRunner() {
      this.failNth = -1;  // don't fail anything
    }

    // Creates a MockTestRunner that will simulate failing the nth test run on it.
    public MockTestRunner(int failNth) {
      this.failNth = failNth;
    }

    @Override
    public synchronized void startTest(TabConnection connection,
                                       final TestRequest testRequest,
                                       long testTimeoutMillis,
                                       final TestRunner.Listener listener) {
      final boolean shouldFail = this.numTestsRun == this.failNth;
      ++this.numTestsRun;
      (new Thread() {
          public void run() {
            final TestData testData = new TestData(testRequest);
            try { Thread.sleep(100); } catch (InterruptedException e) { return; }
            if (shouldFail) {
              testData.setFailure(MockTestRunner.FAILURE_MESSAGE);
            }
            testData.setCompleted();
            listener.onTestCompleted(testData);
          }
        }).start();
    }
  }

  public void testSuccess() throws Exception {
    TestBatchRunner batchRunner = TestBatchRunner.newInstance(new MockTestRunner());
    MockTabConnection connection = new MockTabConnection();

    List<TestRequest> testBatch = ImmutableList.of(
        new TestRequest(new URL("http://www.example.com/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.org/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.xyz/"), "Default", "", 0, 0));

    MockListener listener = new MockListener();
    batchRunner.startBatch(connection, testBatch, 0, 1000, listener);
    listener.waitUntilDone();

    synchronized (listener) {
      // We should have one test result for each test request, all successful:
      assertEquals(testBatch.size(), listener.testsData.size());
      for (int i = 0; i < testBatch.size(); ++i) {
        assertEquals(testBatch.get(i), listener.testsData.get(i).getTestRequest());
        assertFalse(listener.testsData.get(i).isFailure());
      }
      // There were no failures/retries, so the final batch results should be the same as all test
      // results:
      assertEquals(listener.testsData, listener.batchData);
      // We should get the same TabConnection back:
      assertEquals(connection, listener.tabConnection);
    }
  }

  public void testFailureRetry() throws Exception {
    // Use a MockTestRunner that will fail (only) the third test to be run.
    TestBatchRunner batchRunner = TestBatchRunner.newInstance(new MockTestRunner(2));
    MockTabConnection connection = new MockTabConnection();

    List<TestRequest> testBatch = ImmutableList.of(
        new TestRequest(new URL("http://www.example.com/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.org/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.xyz/"), "Default", "", 0, 0));

    MockListener listener = new MockListener();
    batchRunner.startBatch(connection, testBatch,
                           1,  // Allow one retry for this batch.
                           1000, listener);
    listener.waitUntilDone();

    synchronized (listener) {
      // We had to redo three tests:
      assertEquals(testBatch.size() + 3, listener.testsData.size());
      // Only the third test failed:
      for (int i = 0; i < listener.testsData.size(); ++i) {
        assertEquals((i == 2), listener.testsData.get(i).isFailure());
      }
      assertEquals(MockTestRunner.FAILURE_MESSAGE, listener.testsData.get(2).getFailureMessage());
      // The final batch data is all successful and matches with the test requests:
      assertEquals(testBatch.size(), listener.batchData.size());
      for (int i = 0; i < testBatch.size(); ++i) {
        assertEquals(testBatch.get(i), listener.batchData.get(i).getTestRequest());
        assertFalse(listener.batchData.get(i).isFailure());
      }
    }
  }

  public void testFailureNoRetries() throws Exception {
    // Use a MockTestRunner that will fail (only) the second test to be run.
    TestBatchRunner batchRunner = TestBatchRunner.newInstance(new MockTestRunner(1));
    MockTabConnection connection = new MockTabConnection();

    List<TestRequest> testBatch = ImmutableList.of(
        new TestRequest(new URL("http://www.example.com/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.org/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.xyz/"), "Default", "", 0, 0));

    MockListener listener = new MockListener();
    batchRunner.startBatch(connection, testBatch,
                           0,  // Do not allow retries for this batch.
                           1000, listener);
    listener.waitUntilDone();

    synchronized (listener) {
      // We failed after the second test, with no retries, so later tests were never even run:
      assertEquals(2, listener.testsData.size());
      // However, we should still be given a full set of batch data, matching the test requests:
      assertEquals(testBatch.size(), listener.batchData.size());
      for (int i = 0; i < testBatch.size(); ++i) {
        assertEquals(testBatch.get(i), listener.batchData.get(i).getTestRequest());
      }
      // All tests from the second onward should have failed:
      for (int i = 0; i < listener.batchData.size(); ++i) {
        assertEquals((i >= 1), listener.batchData.get(i).isFailure());
      }
      assertEquals(MockTestRunner.FAILURE_MESSAGE, listener.batchData.get(1).getFailureMessage());
      assertEquals("batch failed", listener.batchData.get(2).getFailureMessage());
      assertEquals("batch failed", listener.batchData.get(3).getFailureMessage());
    }
  }

}
