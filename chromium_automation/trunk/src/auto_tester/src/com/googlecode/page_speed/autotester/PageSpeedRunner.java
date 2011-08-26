package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.StreamGobbler;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

// Copyright 2011 Google Inc. All Rights Reserved.

/**
 * Converts HAR, DOM and Instrumentation data into PageSpeed results.
 * 
 * @author azlatin@google.com (Alexander Zlatin)
 * 
 */
public class PageSpeedRunner {

  private String pathToPageSpeedBin;
  private String strategy;

  /**
   * Creates a new Page Speed result creator.
   * 
   * @param aPathToPageSpeedBin The path to a runnable pagespeed_bin
   */
  public PageSpeedRunner(String aPathToPageSpeedBin) {
    pathToPageSpeedBin = aPathToPageSpeedBin;
    strategy = "desktop";
  }

  /**
   * Gets the scoring strategy to be used by pagespeed.
   * 
   * @return the scoring strategy to use
   */
  public String getStrategy() {
    return strategy;
  }

  /**
   * Sets the scoring strategy (desktop, mobile)
   * 
   * @param strategy the scoring strategy to use
   */
  public void setStrategy(String strategy) {
    strategy = strategy.toLowerCase();
    if (!strategy.equals("desktop") && !strategy.equals("mobile")) {
      return;
    }
    this.strategy = strategy;
  }

  /**
   * Converts HAR, DOM and Instrumentation data into PageSpeed results.
   * 
   * @param aPathToHarFile Path to a readable HTTP Archive
   * @param aPathToInstrumentationFile Path to readable timeline data
   * @param aPathToDomFile Path to readable DOM data
   * @return A JSONObject containing Page Speed results
   */
  public JSONObject generatePageSpeedResults(File aPathToHarFile,
      File aPathToInstrumentationFile, File aPathToDomFile) {
    // Construct the command to invoke Page Speed.
    List<String> command = new ArrayList<String>();
    command.add(pathToPageSpeedBin);
    command.add("--output_format");
    command.add("unformatted_json");
    command.add("--input_format");
    command.add("har");
    command.add("--strategy");
    command.add(strategy);

    command.add("--input_file");
    command.add(aPathToHarFile.getPath());
    if (aPathToInstrumentationFile != null) {
      command.add("--instrumentation_input_file");
      command.add(aPathToInstrumentationFile.getPath());
    }
    if (aPathToDomFile != null) {
      command.add("--dom_input_file");
      command.add(aPathToDomFile.getPath());
    }

    // Invoke the pagespeed binary.
    ProcessBuilder builder = new ProcessBuilder(command.toArray(new String[0]));
    Process process = null;
    try {
      process = builder.start();
    } catch (IOException e) {
      System.err.println(e);
      return null;
    }

    //TODO(azlatin): Use a thread pool.
    StreamGobbler stdout = new StreamGobbler(process.getInputStream());
    StreamGobbler stderr = new StreamGobbler(process.getErrorStream());

    stderr.start();
    stdout.start();
    
    try {
      // Wait for process to end
      process.waitFor();
      // Lets prevent some race conditions.
      stderr.join();
      stdout.join();
    } catch (InterruptedException e) {
      System.err.println(e);
    }

    // Parse results
    JSONObject jsonResults = (JSONObject) JSONValue.parse(stdout.getOutput());

    return jsonResults;
  }

}
