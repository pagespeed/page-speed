// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

import junit.framework.TestCase;

import java.net.URL;

/**
 * Unit tests for <code>TestRequest</code>.
 */
public class TestRequestTest extends TestCase {

  public void testGetFullUrlTrivial() throws Exception {
    assertEquals("http://www.example.com/",
                 new TestRequest(new URL("http://www.example.com/"),
                                 "Default", "", 2, 0).getFullURL());
  }

  public void testGetFullUrlVariation() throws Exception {
    assertEquals("http://www.example.com/?ModPagespeed=off",
                 new TestRequest(new URL("http://www.example.com/"),
                                 "Off", "ModPagespeed=off", 3, 1).getFullURL());
  }

  public void testGetFullUrlVariationWithQueryParams() throws Exception {
    assertEquals("http://www.example.com/?awesome=yes&ModPagespeed=off",
                 new TestRequest(new URL("http://www.example.com/?awesome=yes"),
                                 "Off", "ModPagespeed=off", 1, 0).getFullURL());
  }

}
