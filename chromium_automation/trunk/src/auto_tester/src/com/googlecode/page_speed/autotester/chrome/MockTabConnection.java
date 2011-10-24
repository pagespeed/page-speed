// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.chrome;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

/**
 * A mocked-out implementation of <code>TabConnection</code>, for unit tests.  See
 * <code>TestRunnerTest.java</code> for usage examples.
 */
public class MockTabConnection extends TabConnection {

  private static class Expectation {
    public final String methodName;
    public final JSONObject params;
    public final List<Action> actions = new ArrayList<Action>();
    public Expectation(String methodName, JSONObject params) {
      this.methodName = methodName;
      this.params = params;
    }
  }

  private static interface Action {
    void perform(NotificationListener listener, AsyncCallback callback);
  }

  private NotificationListener listener = null;
  private LinkedList<Expectation> expectations = new LinkedList<Expectation>();

  @Override
  public synchronized void setNotificationListener(NotificationListener listener) {
    this.listener = listener;
  }

  @Override
  public synchronized void asyncCall(String methodName, JSONObject params,
                                     AsyncCallback callback) {
    final String callString = methodName + (params == null ? "<null>" : params.toJSONString());
    final Expectation expected = this.expectations.pollFirst();

    if (expected == null) {
      throw new RuntimeException("Did not expect more calls, but received " + callString);
    }

    if (!expected.methodName.equals(methodName) ||
        (expected.params != null && !expected.params.equals(params))) {
      throw new RuntimeException(String.format(
          "Expected call to %s%s, but received %s",
          expected.methodName,
          (expected.params == null ? "" : expected.params.toJSONString()),
          callString));
    }

    for (Action action : expected.actions) {
      action.perform(this.listener, callback);
    }
  }

  @Override
  public synchronized boolean hasOutstandingCalls() {
    return false;
  }

  @Override
  public synchronized void close() {
    this.listener = null;
  }

  /**
   * Call to indicate that we will not be making any more asyncCall()s; throws an exception if
   * there are unsatisfied expectations remaining.
   */
  public void doneMakingCalls() {
    if (!this.expectations.isEmpty()) {
      throw new RuntimeException(String.format("Expected %d more call(s), starting with %s",
                                               this.expectations.size(),
                                               this.expectations.getFirst().methodName));
    }
  }

  /**
   * We expect an asyncCall to a method with unspecified parameters.
   */
  public MockTabConnection expectCall(String methodName) {
    return this.expectCall(methodName, null);
  }

  /**
   * We expect an asyncCall to a method with specific parameters.
   */
  public MockTabConnection expectCall(String methodName, String params) {
    this.expectations.add(new Expectation(
        methodName, (params == null ? null : parse(params))));
    return this;
  }

  /**
   * Send a notification message with empty parameters.
   */
  public MockTabConnection willNotify(String methodName) {
    return this.willNotify(methodName, "{}");
  }

  /**
   * Send a notification message with specific parameters.
   */
  public MockTabConnection willNotify(final String methodName, final String params) {
    return this.addAction(new Action() {
        public void perform(NotificationListener listener, AsyncCallback callback) {
          if (listener != null) {
            listener.handleNotification(methodName, parse(params));
          }
        }
      });
  }

  /**
   * Respond to the asyncCall with a successful result.
   */
  public MockTabConnection willRespond(final String result) {
    return this.addAction(new Action() {
        public void perform(NotificationListener listener, AsyncCallback callback) {
          callback.onSuccess(parse(result));
        }
      });
  }

  /**
   * Respond to the asyncCall with an error message.
   */
  public MockTabConnection willError(final String message) {
    return this.addAction(new Action() {
        public void perform(NotificationListener listener, AsyncCallback callback) {
          callback.onError(message);
        }
      });
  }

  // Private convenience method to add an action to the most recent expectation.
  private MockTabConnection addAction(Action action) {
    if (this.expectations.isEmpty()) {
      throw new RuntimeException("no expectations yet");
    }
    this.expectations.getLast().actions.add(action);
    return this;
  }

  // Private convenience method to parse a string into a JSON object and check for errors.
  private JSONObject parse(String string) {
    JSONObject obj = (JSONObject)JSONValue.parse(string);
    if (obj == null) {
      throw new IllegalArgumentException("failed to parse JSON: " + string);
    }
    return obj;
  }

}
