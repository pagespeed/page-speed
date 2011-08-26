// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.iface;

import com.googlecode.page_speed.autotester.TestResult;

/**
 * Interface for an observer that is notified of test status.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public interface TestObserver {
  
  /**
   * Called when all tests are finished.
   */
  void onTestCompleted(TestResult result);
  
  /**
   * Called when all tests are finished.
   */
  void onAllTestsCompleted();
}
