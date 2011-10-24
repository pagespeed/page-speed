// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

import junit.framework.TestCase;
import org.json.simple.JSONObject;

/**
 * Unit tests for the <code>Json</code> class.
 */
public class JsonTest extends TestCase {

  public void testGetBoolean() throws JsonException {
    assertEquals(false, Json.getBoolean(Json.parseObject("{\"foo\":false}"), "foo"));
    assertEquals(true, Json.getBoolean(Json.parseObject("{\"baz\":true}"), "baz"));
  }

  public void testGetDouble() throws JsonException {
    assertEquals(0.625, Json.getDouble(Json.parseObject("{\"foo\":0.625}"), "foo"));
    assertEquals(-31.0, Json.getDouble(Json.parseObject("{\"baz\":-31.0}"), "baz"));
    // getDouble should work for integral values too:
    assertEquals(17.0, Json.getDouble(Json.parseObject("{\"bar\":17}"), "bar"));
  }

  public void testGetLong() throws JsonException {
    assertEquals(0L, Json.getLong(Json.parseObject("{\"foo\":0}"), "foo"));
    assertEquals(14L, Json.getLong(Json.parseObject("{\"foo\":14}"), "foo"));
    assertEquals(-283L, Json.getLong(Json.parseObject("{\"foo\":-283}"), "foo"));
  }

  public void testPutDouble() throws JsonException {
    JSONObject obj = new JSONObject();
    Json.put(obj, "bar", 19.5);
    assertEquals(19.5, Json.getDouble(obj, "bar"));
  }

  public void testPutLong() throws JsonException {
    JSONObject obj = new JSONObject();
    Json.put(obj, "bar", 9283);
    assertEquals(9283L, Json.getLong(obj, "bar"));
  }

  public void testGetStringError() {
    JSONObject obj = new JSONObject();
    Json.put(obj, "baz", 42);
    try {
      Json.getString(obj, "baz");
    } catch (JsonException e) {
      assertEquals("not a String: 42", e.getMessage());
      return;
    }
    fail("should have thrown a JsonException");
  }

  public void testIterateObjects() throws JsonException {
    int count = 0;
    for (JSONObject obj : Json.iterateObjects(Json.parseArray("[{},{\"a\":1},{\"b\":true}]"))) {
      assertNotNull(obj);
      ++count;
    }
    assertEquals(3, count);
  }

}
