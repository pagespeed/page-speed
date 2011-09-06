package com.googlecode.page_speed.autotester.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

// Copyright 2011 Google Inc. All Rights Reserved.


/**
 * Various utility methods for dealing with files.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class FileUtils {

  /**
   * Converts a URL into a valid filesystem filename.
   *
   * @param s URL string
   * @return URL with any invalid filesystem characters removed.
   */
  public static String encodeURLToFile(String s) {
    char fileSep = System.getProperty("file.separator", "/").charAt(0);
    int len = s.length();
    StringBuilder sb = new StringBuilder(len);
    for (int i = 0; i < len; i++) {
      char ch = s.charAt(i);
      if (!(ch < ' ' || ch >= 0x7F || ch == fileSep || (ch == '.' && i == 0))) {
        sb.append(ch);
      }
    }
    return sb.toString();
  }

  /**
   * Fetch the entire contents of a text file, and return it in a String. This
   * style of implementation does not throw Exceptions to the caller.
   *
   * @param aFile is a file which already exists and can be read.
   */
  public static String getContents(File aFile) {
    StringBuilder contents = new StringBuilder();
    try {
      BufferedReader input = new BufferedReader(new FileReader(aFile));
      try {
        String line = null;
        while ((line = input.readLine()) != null) {
          contents.append(line);
          contents.append(System.getProperty("line.separator"));
        }
      } finally {
        input.close();
      }
    } catch (IOException ex) {
      ex.printStackTrace();
    }
    return contents.toString();
  }

  /**
   * Fetch the entire contents of a stream, and return it in a String.
   * http://stackoverflow.com/questions/309424/#5445161
   *
   * @param aStream is a stream that can be read.
   */
  public static String getContents(InputStream aStream) {
    return new Scanner(aStream).useDelimiter("\\A").next();
  }

  /**
   * Reads a file into a list of lines.
   *
   * @param aFile is a file which already exists and can be read.
   */
  public static List<String> getLines(File aFile) {
    ArrayList<String> contents = new ArrayList<String>();
    try {
      BufferedReader input = new BufferedReader(new FileReader(aFile));
      try {
        String line = null;
        while ((line = input.readLine()) != null) {
          contents.add(line);
        }
      } finally {
        input.close();
      }
    } catch (IOException ex) {
      ex.printStackTrace();
    }
    return contents;
  }

  /**
   * Change the contents of text file in its entirety, overwriting any existing
   * text.
   *
   * This style of implementation throws all exceptions to the caller.
   *
   * @param aFile is an existing file which can be written to.
   * @param aContents the contents to write to the file.
   * @throws IOException if problem encountered during write.
   */
  public static void setContents(File aFile, String aContents) throws IOException {
    Writer output = new BufferedWriter(new FileWriter(aFile));
    try {
      output.write(aContents);
    } finally {
      output.close();
    }
  }
}
