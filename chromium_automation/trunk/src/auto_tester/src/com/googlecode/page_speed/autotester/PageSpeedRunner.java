package com.googlecode.page_speed.autotester;

import com.googlecode.page_speed.autotester.util.Json;
import com.googlecode.page_speed.autotester.util.JsonException;
import com.googlecode.page_speed.autotester.util.StreamGobbler;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import org.json.simple.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.List;

// Copyright 2011 Google Inc. All Rights Reserved.

/**
 * Converts HAR, DOM and Instrumentation data into PageSpeed results.
 *
 * @author azlatin@google.com (Alexander Zlatin)
 */
public class PageSpeedRunner {

  private final String pathToPageSpeedBin;
  private final Strategy strategy;
  private final PrintStream logFile;

  public static enum Strategy {
    DESKTOP, MOBILE;
    public String toCommandLineString() {
      return this.toString().toLowerCase();
    }
  }

  /**
   * Creates a new Page Speed result creator.
   *
   * @param pathToPageSpeedBin The path to a pagespeed_bin executable
   * @param strategy The scoring strategy to use
   * @param pathToLogFile The path to which to log stderr output, or null
   * @throws IOException if the logfile cannot be opened or the pagespeed_bin cannot be run.
   * @throws InterruptedException if interrupted while trying to run pagespeed_bin.
   */
  public PageSpeedRunner(String pathToPageSpeedBin, Strategy strategy, String pathToLogFile)
      throws IOException, InterruptedException {
    this.pathToPageSpeedBin = pathToPageSpeedBin;
    this.strategy = strategy;
    this.logFile = pathToLogFile == null ? null : new PrintStream(pathToLogFile);

    // Try running `pathToPageSpeedBin --version` to make sure it works.
    ProcessBuilder builder = new ProcessBuilder(ImmutableList.of(pathToPageSpeedBin, "--version"));
    Process process = builder.start();
    (new StreamGobbler(process.getInputStream())).start();
    (new StreamGobbler(process.getErrorStream())).start();
    process.waitFor();
    int exitCode = process.exitValue();
    if (exitCode != 0) {
      throw new IOException("Page Speed returned nonzero exit code: " + exitCode);
    }
  }

  /**
   * Converts HAR, DOM and Instrumentation data into PageSpeed results.
   *
   * @param pathToHarFile Path to a readable HTTP Archive
   * @param pathToInstrumentationFile Path to readable timeline data
   * @param pathToDomFile Path to readable DOM data
   * @returns A JSONObject containing Page Speed results
   * @throws PageSpeedException if the subprocess can't be started or suffers an internal error.
   */
  public JSONObject generatePageSpeedResults(String testDescription, File pathToHarFile,
      File pathToInstrumentationFile, File pathToDomFile) throws PageSpeedException {
    // Construct the command to invoke Page Speed.
    List<String> command = Lists.newArrayList();
    command.add(this.pathToPageSpeedBin);
    command.add("--output_format");
    command.add("unformatted_json");
    command.add("--input_format");
    command.add("har");
    command.add("--strategy");
    command.add(this.strategy.toCommandLineString());

    command.add("--input_file");
    command.add(pathToHarFile.getPath());
    if (pathToInstrumentationFile != null) {
      command.add("--instrumentation_input_file");
      command.add(pathToInstrumentationFile.getPath());
    }
    if (pathToDomFile != null) {
      command.add("--dom_input_file");
      command.add(pathToDomFile.getPath());
    }

    // Invoke the pagespeed binary.
    final ProcessBuilder builder = new ProcessBuilder(command);
    final Process process;
    try {
      process = builder.start();
    } catch (IOException e) {
      throw new PageSpeedException("Error starting Page Speed process: " + e.getMessage(), e);
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
      process.destroy();
      throw new PageSpeedException("Interrupted", e);
    }

    // Log whatever was written to stderr.
    if (this.logFile != null) {
      synchronized (this.logFile) {
        this.logFile.println("======================================================");
        this.logFile.println(testDescription);
        this.logFile.println(Joiner.on(" ").join(command));
        this.logFile.println("------------------------------------------------------");
        this.logFile.println(stderr.getOutput());
      }
    }

    // Make sure we completed successfully.
    final int exitCode = process.exitValue();
    if (exitCode != 0) {
      throw new PageSpeedException("Page Speed returned nonzero exit code: " + exitCode);
    }

    // Parse results
    try {
      return Json.parseObject(stdout.getOutput());
    } catch (JsonException e) {
      throw new PageSpeedException("Bad JSON from Page Speed: " + e.getMessage());
    }
  }

}
