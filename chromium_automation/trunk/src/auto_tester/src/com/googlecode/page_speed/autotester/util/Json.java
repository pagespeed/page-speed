// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.parser.ParseException;

import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;

/**
 * Utilities for working safely with JSON values.
 *
 * The JSON library we use is a bit inconvenient in that using it tends to result in "unchecked"
 * warnings and potentially unsafe casts.  This class provides a bunch of static utility methods
 * for manipulating JSON objects in a type-safe way.  The compiler warnings are eliminated, and any
 * potential parsing errors or dynamic type errors result in a checked exception, forcing the
 * client to handle them explicitly.
 *
 * This class does not yet contain all possibly-desirable overloadings of its methods, but we can
 * easily add more as we find we need them.
 */
public final class Json {

  private Json() {
    throw new AssertionError();  // uninstantiable class
  }

  /**
   * Parses text from a Reader into a JSONArray.
   * @throws IOException if there's a problem reading text from the Reader.
   * @throws JsonException if the text can't be parsed or parses to a non-array JSON value.
   */
  public static JSONArray parseArray(Reader reader) throws IOException, JsonException {
    return asArray(parse(reader));
  }

  /**
   * Parses a String into a JSONArray.
   * @throws JsonException if the string can't be parsed or parses to a non-array JSON value.
   */
  public static JSONArray parseArray(String string) throws JsonException {
    return asArray(parse(string));
  }

  /**
   * Parses a String into a JSONObject.
   * @throws JsonException if the string can't be parsed or parses to a non-object JSON value.
   */
  public static JSONObject parseObject(String string) throws JsonException {
    return asObject(parse(string));
  }

  /**
   * Gets a boolean field value from a JSONObject.
   * @throws JsonException if there's no such field or if the stored value is not an integer.
   */
  public static boolean getBoolean(JSONObject obj, String key) throws JsonException {
    return asBoolean(obj.get(key));
  }

  /**
   * Gets a double (or integral) field value from a JSONObject.
   * @throws JsonException if there's no such field or if the stored value is not a number.
   */
  public static double getDouble(JSONObject obj, String key) throws JsonException {
    return asDouble(obj.get(key));
  }

  /**
   * Gets an integral field value from a JSONObject.
   * @throws JsonException if there's no such field or if the stored value is not an integer.
   */
  public static long getLong(JSONObject obj, String key) throws JsonException {
    return asLong(obj.get(key));
  }

  /**
   * Gets an object field value from a JSONObject.
   * @throws JsonException if there's no such field or if the stored value is not an object.
   */
  public static JSONObject getObject(JSONObject obj, String key) throws JsonException {
    return asObject(obj.get(key));
  }

  /**
   * Gets a string field value from a JSONObject.
   * @throws JsonException if there's no such field or if the stored value is not a string.
   */
  public static String getString(JSONObject obj, String key) throws JsonException {
    return asString(obj.get(key));
  }

  /**
   * Appends an object value onto a JSON array.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONArray
  public static void add(JSONArray array, JSONObject value) {
    array.add(value);
  }

  /**
   * Inserts a boolean value into a JSON object.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONObject
  public static void put(JSONObject obj, String key, boolean value) {
    obj.put(key, Boolean.valueOf(value));
  }

  /**
   * Inserts a double value into a JSON object.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONObject
  public static void put(JSONObject obj, String key, double value) {
    obj.put(key, Double.valueOf(value));
  }

  /**
   * Inserts an object value into a JSON object.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONObject
  public static void put(JSONObject obj, String key, JSONObject value) {
    obj.put(key, value);
  }

  /**
   * Inserts an integral value into a JSON object.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONObject
  public static void put(JSONObject obj, String key, long value) {
    obj.put(key, Long.valueOf(value));
  }

  /**
   * Inserts an string value into a JSON object.
   */
  @SuppressWarnings("unchecked")  // safe by specification of JSONObject
  public static void put(JSONObject obj, String key, String value) {
    obj.put(key, value);
  }

  /**
   * Returns an iterable over an JSON array of JSON objects.
   * @throws JsonException if any of the values in the array are non-object JSON values.
   */
  public static Iterable<JSONObject> iterateObjects(JSONArray array) throws JsonException {
    List<JSONObject> objects = new ArrayList<JSONObject>();
    for (Object value : array) {
      objects.add(asObject(value));
    }
    return objects;
  }

  ///// Private helper methods:

  private static Object parse(Reader reader) throws IOException, JsonException {
    try {
      return JSONValue.parseWithException(reader);
    } catch (ParseException e) {
      throw new JsonException(e.getMessage());
    }
  }

  private static Object parse(String string) throws JsonException {
    try {
      return JSONValue.parseWithException(string);
    } catch (ParseException e) {
      throw new JsonException(e.getMessage());
    }
  }

  private static JSONArray asArray(Object value) throws JsonException {
    if (value == null || !(value instanceof JSONArray)) {
      throw new JsonException("not a JSONArray: " + JSONValue.toJSONString(value));
    } else {
      return (JSONArray)value;
    }
  }

  private static boolean asBoolean(Object value) throws JsonException {
    if (value == null || !(value instanceof Boolean)) {
      throw new JsonException("not a Boolean: " + JSONValue.toJSONString(value));
    } else {
      return ((Boolean)value).booleanValue();
    }
  }

  private static double asDouble(Object value) throws JsonException {
    if (value == null || !(value instanceof Number)) {
      throw new JsonException("not a Number: " + JSONValue.toJSONString(value));
    } else {
      return ((Number)value).doubleValue();
    }
  }

  private static long asLong(Object value) throws JsonException {
    if (value == null || !(value instanceof Long)) {
      throw new JsonException("not a Long: " + JSONValue.toJSONString(value));
    } else {
      return ((Long)value).longValue();
    }
  }

  private static JSONObject asObject(Object value) throws JsonException {
    if (value == null || !(value instanceof JSONObject)) {
      throw new JsonException("not a JSONObject: " + JSONValue.toJSONString(value));
    } else {
      return (JSONObject)value;
    }
  }

  private static String asString(Object value) throws JsonException {
    if (value == null || !(value instanceof String)) {
      throw new JsonException("not a String: " + JSONValue.toJSONString(value));
    } else {
      return (String)value;
    }
  }

}
