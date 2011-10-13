// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Defines and parses command line flags.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public final class Flags {

  private static Map<String, String[]> flagSet =
      new HashMap<String, String[]>();
  private static Map<String, String> flags = new HashMap<String, String>();
  private static List<String> required = new ArrayList<String>();

  private Flags() { throw new AssertionError(); }  // uninstantiable class

  /**
   * Parse command line flags out of the command line arguments.
   *
   * @param args command-line arguments
   */
  public static void init(String[] args) {
    //Set default values
    for (Entry<String, String[]> flag : flagSet.entrySet()) {
      flags.put(flag.getKey(), flag.getValue()[1]);
    }

    for (int x = 0; x < args.length; x++) {
      String arg = args[x];
      if (arg.startsWith("--")) {
        String[] parts = arg.substring(2).split("=", 2);
        if (flags.containsKey(parts[0])) {
          if (parts.length == 2) {
            flags.put(parts[0], parts[1]); // --flag=value
          } else if (x + 1 < args.length && !args[x + 1].startsWith("--")) {
            x++;
            flags.put(parts[0], args[x]); // --flag value
          } else {
            flags.put(parts[0], "1"); // --flag
          }
        } else {
          System.err.println("Unknown Argument: " + parts[0]);
        }
      }
    }
  }

  /**
   * Prints a list of flags with descriptions and default values.
   */
  public static void printHelp() {
    for (Entry<String, String[]> flag : flagSet.entrySet()) {
      System.out.println(String.format("  --%s\t\t%s\n\t\t\t%sDefault: %s\n",
          flag.getKey(), flag.getValue()[0], (required.contains(flag.getKey())
              ? "Required, " : ""), flag.getValue()[1]));
    }
  }

  /**
   * Returns whether all the required flags are specified.
   *
   * @return true if all required fields are specified, false otherwise.
   */
  public static boolean isValid() {
    if (required.size() > flags.size()) {
      return false;
    }
    for (String name : required) {
      if (getStr(name) == null) {
        return false;
      }
    }
    return true;
  }

  /**
   * Defines a new flag.
   *
   * @param name The flag name.
   * @param desc A description of what the flag is for.
   * @param def The default value if it is not specified.
   * @param req Whether the flag is required (must be false if def != null).
   * @return Success if the new flag was defined, false otherwise.
   */
  public static boolean define(String name, String desc, String def, boolean req) {
    if (!flagSet.containsKey(name)) {
      flagSet.put(name, new String[] {desc, def});
      if (req) {
        if (def != null) {
          System.err.println(String.format(
              "%s can't be both required and have default.", name));
          return false;
        }
        required.add(name);
      }
      return true;
    }
    return false;
  }

  /**
   * Gets a flag value as a boolean.
   *
   * @param name The flag name.
   * @return The flag value as a boolean.
   */
  public static boolean getBool(String name) {
    return flags.get(name) != null && !flags.get(name).equals("0");
  }

  /**
   * Gets a flag value as an integer.
   *
   * @param name The flag name.
   * @return The flag value as a integer.
   */
  public static Integer getInt(String name) {
    return Integer.parseInt(flags.get(name));
  }

  /**
   * Gets a flag value as a string.
   *
   * @param name The flag name.
   * @return The flag value as a string.
   */
  public static String getStr(String name) {
    return flags.get(name);
  }

}
