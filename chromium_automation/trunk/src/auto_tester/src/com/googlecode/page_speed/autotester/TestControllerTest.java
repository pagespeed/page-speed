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
 * Unit tests for <code>TestController</code>.
 */
public class TestControllerTest extends TestCase {

  // Simple listener that counts how many times each method is called.
  private static class Listener implements TestController.Listener {
    public int testCompletedOnce = 0;
    public int testTotallyDone = 0;
    public int allTestsCompleted = 0;
    public synchronized void onTestCompletedOnce(TestData testData) {
      ++this.testCompletedOnce;
    }
    public synchronized void onTestTotallyDone(TestData testData) {
      ++this.testTotallyDone;
    }
    public synchronized void onAllTestsCompleted() {
      ++this.allTestsCompleted;
    }
  }

  private static class MockBatchRunner extends TestBatchRunner {
    private final boolean failFirst;

    public MockBatchRunner(boolean failFirst) {
      this.failFirst = failFirst;
    }

    @Override
    public void startBatch(final TabConnection connection,
                           final List<TestRequest> batchRequests,
                           int maxBatchRetries,
                           long testTimeoutMillis,
                           final TestBatchRunner.Listener listener) {
      (new Thread() {
          public void run() {
            // If failFirst is set, simulate failing the first test of the batch.
            if (MockBatchRunner.this.failFirst) {
              TestData testData = new TestData(batchRequests.get(0));
              try { Thread.sleep(100); } catch (InterruptedException e) { return; }
              testData.setFailure("Oh no!");
              testData.setCompleted();
              listener.onTestCompleted(testData);
            }

            // Now simulate a successful run through the batch.
            List<TestData> results = Lists.newArrayList();
            for (TestRequest testRequest : batchRequests) {
              TestData testData = new TestData(testRequest);
              try { Thread.sleep(100); } catch (InterruptedException e) { return; }
              testData.setCompleted();
              results.add(testData);
              listener.onTestCompleted(testData);
            }
            listener.onBatchCompleted(connection, results);
          }
        }).start();
    }
  }

  public void testSuccess() throws Exception {
    TestController controller = TestController.newInstance(new MockBatchRunner(false));
    List<MockTabConnection> connections = ImmutableList.of(new MockTabConnection(),
                                                           new MockTabConnection());

    List<TestRequest> batch1 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.com/"), "Default", "", 0, 0));
    List<TestRequest> batch2 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 1));
    List<TestRequest> batch3 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.org/"), "Default", "", 0, 0));
    List<List<TestRequest>> testBatches = ImmutableList.of(batch1, batch2, batch3);

    Listener listener = new Listener();
    controller.runTests(connections, testBatches, 1, 1000, listener);
    assertEquals(1, listener.allTestsCompleted);
    assertEquals(4, listener.testTotallyDone);
    assertEquals(4, listener.testCompletedOnce);
  }

  public void testBatchFailures() throws Exception {
    TestController controller = TestController.newInstance(new MockBatchRunner(true));
    List<MockTabConnection> connections = ImmutableList.of(new MockTabConnection(),
                                                           new MockTabConnection());

    List<TestRequest> batch1 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.com/"), "Default", "", 0, 0));
    List<TestRequest> batch2 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 0),
        new TestRequest(new URL("http://www.example.net/"), "Default", "", 0, 1));
    List<TestRequest> batch3 = ImmutableList.of(
        new TestRequest(new URL("http://www.example.org/"), "Default", "", 0, 0));
    List<List<TestRequest>> testBatches = ImmutableList.of(batch1, batch2, batch3);

    Listener listener = new Listener();
    controller.runTests(connections, testBatches, 1, 1000, listener);
    assertEquals(1, listener.allTestsCompleted);
    assertEquals(4, listener.testTotallyDone);
    assertEquals(7, listener.testCompletedOnce);  // 3 tests had to be redone (one per batch)
  }

  // TODO(mdsteele): Add more tests:
  //   * Verify that we shard across multiple tab connections concurrently.
  //   * Verity that the results we get back match up with the test requests we made.

}
