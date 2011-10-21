// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester;

/**
 * This checked exception indicates that an error occurred during configuration
 * or initialization, usually due to erroneous user input.  For example,
 * perhaps a config file was malformed or could not be opened, or perhaps a
 * server specified by the user could not be connected to.  The intended
 * pattern in the face of such problems is to print one or more user-friendly
 * error messages to System.err and then throw a ConfigException.
 */
@SuppressWarnings("serial")  // don't bother us about missing serialVersionUID
public class ConfigException extends Exception {
}
