// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

import com.googlecode.page_speed.autotester.util.FileUtils;

import junit.framework.TestCase;

/**
 * Unit tests for <code>FileUtils</code>.
 *
 * @author mdsteele@google.com (Matthew D. Steele)
 */
public class FileUtilsTest extends TestCase {

  public void testEncodeURLToFile() {
    assertEquals("http:www.example.comfoo.html",
                 FileUtils.encodeURLToFile("http://www.example.com/foo.html"));
  }

}
