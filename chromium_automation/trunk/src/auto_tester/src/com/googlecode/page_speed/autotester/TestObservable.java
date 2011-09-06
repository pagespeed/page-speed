// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.iface.TestObserver;

import java.util.ArrayList;
import java.util.List;

/**
 * Class that can be observed for test status updates.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class TestObservable {

  private List<TestObserver> observers = new ArrayList<TestObserver>();

  public void addObserver(TestObserver obs) {
    if (obs == null) {
        throw new IllegalArgumentException();
    }
    if (observers.contains(obs)) {
       return;
    }
    observers.add(obs);
  }

  public void notifyTestCompleted(TestResult result) {
    for (TestObserver obs : observers) {
      try {
        obs.onTestCompleted(result);
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
  }

  public void notifyAllTestsCompleted() {
    for (TestObserver obs : observers) {
      try {
        obs.onAllTestsCompleted();
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
  }

}
