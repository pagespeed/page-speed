// Copyright 2011 Google Inc. All Rights Reserved.

package com.googlecode.page_speed.autotester.output;

import com.googlecode.page_speed.autotester.PageSpeedRunner;
import com.googlecode.page_speed.autotester.TestRequest;
import com.googlecode.page_speed.autotester.TestResult;
import com.googlecode.page_speed.autotester.iface.OutputBuilder;
import com.googlecode.page_speed.autotester.iface.TestObserver;
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
public class OutputGenerator implements TestObserver {

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

  /**
   * Called when all tests are finished.
   */
  @Override
  public void onTestCompleted(TestResult result) {
    TestRequest request = result.request;
    completedTests++;
    System.out.println(String.format("Test %d/%d Completed: %dms (%s - %s #%d-%d)",
      completedTests, totalTests, result.getTestDuration(),
      request.url, request.varName, request.run, request.first));

    if (resultsDir == null || result.failed()) {
      // If resultsDir is null, don't calculate results. Just load the pages.
      // This is useful if you just want to slurp or warm mod_pagespeed caches.
      return;
    }

    try {
      String hash = String.valueOf(Thread.currentThread().hashCode());
      String urlRunView = "_" + request.run + "_" + request.first;
      String urlFileName = FileUtils.encodeURLToFile(request.getFullURL()) + urlRunView;
      File harFile = savedFiles.contains("har")
          ? newResultFile(urlFileName + ".har") : newTempFile(hash, ".har.tmp");
      File tlFile = savedFiles.contains("tl")
          ? newResultFile(urlFileName + ".tl") : newTempFile(hash, ".tl.tmp");
      File domFile = savedFiles.contains("dom")
          ? newResultFile(urlFileName + ".dom") : newTempFile(hash, ".dom.tmp");

      System.out.print("Writing ");
      System.out.print("HAR...");
      FileUtils.setContents(harFile, result.getHAR().toJSONString());
      System.out.print("Timeline...");
      FileUtils.setContents(tlFile, result.getTimeline().toJSONString());
      JSONObject dom = result.getDOM();
      if (dom == null || dom.isEmpty()) {
        domFile = null;
      } else {
        System.out.print("DOM...");
        FileUtils.setContents(domFile, dom.toJSONString());
      }
      System.out.println();

      System.out.println("Running Pagespeed...");
      JSONObject pagespeed = psr.generatePageSpeedResults(harFile, tlFile, domFile);

      if (pagespeed != null) {
        System.out.println("Adding Data to Output...");
        for (OutputBuilder builder : outputs) {
          synchronized (builder) {
            builder.buildPart(request, result.getMetrics(), pagespeed);
          }
        }
      } else {
        System.err.println("Pagespeed Error. Ignoring Test.");
      }

    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      // TestRun objects are very memory hungry
      // Java's GC will already schedule this for collection,
      // but this will hopefully expedite it.
      result.clearData();
    }
  }

  /**
   * Called when all tests are finished.
   */
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
