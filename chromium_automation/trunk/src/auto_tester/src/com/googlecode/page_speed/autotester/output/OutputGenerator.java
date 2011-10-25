// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.output;

import com.googlecode.page_speed.autotester.PageSpeedException;
import com.googlecode.page_speed.autotester.PageSpeedRunner;
import com.googlecode.page_speed.autotester.TestController;
import com.googlecode.page_speed.autotester.TestData;
import com.googlecode.page_speed.autotester.TestRequest;
import com.googlecode.page_speed.autotester.util.FileUtils;

import org.json.simple.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Generates output files from test results.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 *
 */
public class OutputGenerator implements TestController.Listener {

  private final String name;
  private final Set<String> savedFiles = new HashSet<String>();
  private final PageSpeedRunner psr;

  private final String resultsDir;

  private long completedTests = 0L;
  private final long totalTests;
  private final List<OutputBuilder> outputs = new ArrayList<OutputBuilder>();

  /**
   * Creates a new test generator.
   * @param aPSR PageSpeedRunner object.
   * @param aResultsDir Where to save results.
   * @param aTotalTests The total number of tests to be run.
   */
  public OutputGenerator(PageSpeedRunner aPSR,
                         String aResultsDir,
                         long aTotalTests) {
    this.name = new Date().toString().replace(" ", "_") + "_" +
        String.valueOf(this.hashCode());
    this.psr = aPSR;
    this.resultsDir = aResultsDir;
    this.totalTests = aTotalTests;
  }

  /**
   * Marks specific intermediate files to be saved.
   * @param list A comma delimited list of intermediate files to save.
   */
  public void setSavedContentByString(String list) {
    String[] files = list.toLowerCase().split(",");
    for (String fileType : files) {
      savedFiles.add(fileType);
    }
  }

  /**
   * Adds a list of builders from a comma delimited list.
   * @param list A comma delimited list of builder names.
   */
  public void addOutputsByString(String list) {
    String[] output = list.toLowerCase().split(",");
    for (String builder : output) {
      if (builder.equals("analysis")) {
        addOutput(new AnalysisOutput());
      } else if (builder.equals("csv")) {
        addOutput(new CSVOutput());
      } else {
        System.err.println("Unknown output builder: " + builder);
      }
    }
  }

  /**
   * Add an output builder.
   *
   * @param aBuilder An instance of an OutputBuilder.
   */
  public void addOutput(OutputBuilder aBuilder) {
    outputs.add(aBuilder);
  }

  /**
   * Gets the file object representing a file in the result directory.
   *
   * @param file The path to the file under the result directory.
   * @return The file object of a file in the results directory.
   */
  private File newResultFile(String file) {
    return new File(resultsDir, file);
  }

  /**
   * Returns a temporary file object.
   *
   * @param prefix The prefix of the file.
   * @param suffix the suffix of the file name.
   * @return The file object of a temp file.
   */
  private File newTempFile(String prefix, String suffix) {
    File tempFile = new File(System.getProperty("java.io.tmpdir"), prefix + suffix);
    tempFile.deleteOnExit();
    return tempFile;
  }

  @Override
  public synchronized void onTestCompletedOnce(TestData testData) {
    if (testData.isFailure()) {
      System.out.println(String.format("Test [%s] failed after %dms: %s",
                                       testData.getTestRequest().getDescription(),
                                       testData.getTestDurationMillis(),
                                       testData.getFailureMessage()));
    } else {
      System.out.println(String.format("Test [%s] completed after %dms",
                                       testData.getTestRequest().getDescription(),
                                       testData.getTestDurationMillis()));
    }
  }

  @Override
  public synchronized void onTestTotallyDone(TestData testData) {
    final TestRequest testRequest = testData.getTestRequest();

    final String description = testRequest.getDescription();
    if (testData.isFailure()) {
      System.out.println(String.format("Test [%s] FAILED", description));
      return;
    } else if (this.resultsDir == null) {
      // If resultsDir is null, don't calculate results. Just load the pages.
      // This is useful if you just want to slurp or warm mod_pagespeed caches.
      System.out.println(String.format("Test [%s] COMPLETED", description));
      return;
    }

    System.out.println(String.format("Test [%s] COMPLETED; running Page Speed...", description));

    String hash = String.valueOf(Thread.currentThread().hashCode());
    String urlFileName = FileUtils.encodeURLToFile(testRequest.getFullURL()) +
        "_" + testRequest.getRunNumber() + "_" + (testRequest.isRepeatView() ? "R" : "F");
    File harFile = savedFiles.contains("har")
        ? newResultFile(urlFileName + ".har") : newTempFile(hash, ".har.tmp");
    File tlFile = savedFiles.contains("tl")
        ? newResultFile(urlFileName + ".tl") : newTempFile(hash, ".tl.tmp");
    File domFile = savedFiles.contains("dom")
        ? newResultFile(urlFileName + ".dom") : newTempFile(hash, ".dom.tmp");

    try {
      FileUtils.setContents(harFile, testData.getHarString());
      FileUtils.setContents(tlFile, testData.getTimelineString());
      FileUtils.setContents(domFile, testData.getDomString());
    } catch (IOException e) {
      System.out.println(String.format("  I/O error: %s", e.getMessage()));
      return;
    }

    final JSONObject pagespeedResults;
    try {
      pagespeedResults = this.psr.generatePageSpeedResults(harFile, tlFile, domFile);
    } catch (PageSpeedException e) {
      System.out.println(String.format("  Page Speed error: %s", e.getMessage()));
      return;
    }

    for (OutputBuilder builder : this.outputs) {
      builder.addTestResult(testData, pagespeedResults);
    }
  }

  @Override
  public void onAllTestsCompleted() {
    System.out.println("All Tests Completed.");
    if (resultsDir == null) {
      return;
    }
    System.gc();
    System.out.println("Writing Final Output...");
    for (OutputBuilder builder : outputs) {
      File outputFile = newResultFile(name + "." + builder.getResultExtension());
      String outputData = builder.getResult();
      try {
        FileUtils.setContents(outputFile, outputData);
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
    System.out.println("Done.");
  }

}
