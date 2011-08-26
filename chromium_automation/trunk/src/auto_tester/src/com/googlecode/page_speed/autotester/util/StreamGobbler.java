// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * Reads an InputStream in a new thread.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class StreamGobbler extends Thread {

  private InputStream stream = null;
  private StringBuilder contents = null;

  public StreamGobbler(InputStream aStream) {
    stream = aStream;
    contents = new StringBuilder();
  }

  @Override
  public void run() {
    try {
      InputStreamReader isr = new InputStreamReader(stream);
      BufferedReader br = new BufferedReader(isr);
      String line = null;
      while ((line = br.readLine()) != null) {
        contents.append(line);
        contents.append(System.getProperty("line.separator"));
      }
    } catch (IOException ioe) {
      ioe.printStackTrace();
    }
  }

  public String getOutput() {
    return contents.toString();
  }

}
