// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Command line utility that invokes pagespeed_bin and processes the
// protocol buffer results.

package com.googlecode.page_speed;

import com.googlecode.page_speed.PagespeedProtoFormatter;

import java.io.InputStream;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.ArrayList;
import java.util.List;

public class Pagespeed {

  private static Pattern formatArgumentPattern = Pattern.compile("\\$\\d+");

  public static void ShowUsageAndExit() {
    System.err.println("Pagespeed <path_to_pagespeed_bin> <path_to_har_file>");
    System.exit(1);
  }

  /**
   * Emit the FormattedResults to the console as plain text.
   *
   * This is a very simple proof-of-concept implementation that takes
   * a FormattedResults protocol buffer and presents it as plain
   * text. A production implementation might format the results with
   * HTML markup for presentation in a web UI.
   */
  public static void PresentResults(PagespeedProtoFormatter.FormattedResults results) {
    System.out.println("Page Speed Score: " + results.getScore() + "\n");
    for (PagespeedProtoFormatter.FormattedRuleResults ruleResults :
             results.getRuleResultsList()) {
      System.out.println(DoPresentRuleName(ruleResults));
      for (PagespeedProtoFormatter.FormattedUrlBlockResults urlBlocks :
               ruleResults.getUrlBlocksList()) {
        if (urlBlocks.hasHeader()) {
          System.out.println(DoFormat(urlBlocks.getHeader()));
        }
        for (PagespeedProtoFormatter.FormattedUrlResult urlResult :
                 urlBlocks.getUrlsList()) {
          System.out.println(DoFormat(urlResult.getResult()));
          for (PagespeedProtoFormatter.FormatString detail :
                   urlResult.getDetailsList()) {
            System.out.println(DoFormat(detail));
          }
        }
      }
      System.out.println("");
    }
  }

  /**
   * Presents the localized rule name, score, and impact. This is a
   * very simple proof-of-concept implementation.
   */
  public static String DoPresentRuleName(
      PagespeedProtoFormatter.FormattedRuleResults ruleResults) {
    StringBuilder sb = new StringBuilder();
    sb.append(ruleResults.getLocalizedRuleName());
    boolean hasScore = ruleResults.hasRuleScore();
    boolean hasImpact = ruleResults.hasRuleImpact();
    if (hasScore || hasImpact) {
      sb.append(" (");
    }
    if (hasScore) {
      sb.append("Score: ");
      sb.append(ruleResults.getRuleScore());
    }
    if (hasImpact) {
      if (hasScore) {
        sb.append(", ");
      }
      sb.append("Impact: ");
      sb.append(ruleResults.getRuleImpact());
    }
    if (hasScore || hasImpact) {
      sb.append(')');
    }
    return sb.toString();
  }

  /**
   * Converts a FormatString to a presentable String.
   *
   * This is a very simple proof-of-concept implementation. A
   * production implementation might present FormatArguments
   * differently depending on the ArgumentType (e.g. put a URL inside
   * <a> HTML tags).
   */
  public static String DoFormat(PagespeedProtoFormatter.FormatString formatString) {
    // TODO(mdsteele): This method uses the deprecated $1 placeholder format.
    // We need to update it to the new %(FOO)s placeholder format.

    StringBuilder sb = new StringBuilder();
    final String formatStr = formatString.getFormat();
    Matcher m = formatArgumentPattern.matcher(formatStr);
    int last = 0;
    while (m.find()) {
      // Append the substring from the end of the last match, to the
      // beginning of the current match.
      sb.append(formatStr.substring(last, m.start()));
      last = m.end();

      // Next append the argument itself.
      final String matchedString = m.group();

      // The matchedString contains a dollar sign followed by the
      // 1-based index of the argument (e.g. $1). Convert to an
      // integer by trimming the dollar sign and performing a
      // parseInt.
      int argNum = Integer.parseInt(matchedString.substring(1));

      // Get the FormatArgument for the given argNum. Subtract 1 since
      // argNum is 1-based whereas the args are 0-based.
      PagespeedProtoFormatter.FormatArgument arg = formatString.getArgs(argNum - 1);

      // Emit the argument's localized value.
      if (arg.getType() == PagespeedProtoFormatter.FormatArgument.ArgumentType.URL) {
        // Simple example to show how different types of
        // FormatArguments can be presented differently.
        sb.append("[");
        sb.append(arg.getLocalizedValue());
        sb.append("]");
      } else {
        sb.append(arg.getLocalizedValue());
      }
    }

    // Finally, append anything left over after the last argument.
    sb.append(formatStr.substring(last));
    return sb.toString();
  }

  public static void main(String[] args) {
    if (args.length != 2) {
      ShowUsageAndExit();
    }
    final String pathToPagespeedBin = args[0];
    final String pathToHarFile = args[1];

    // Construct the command to invoke pagespeed.
    List<String> command = new ArrayList<String>();
    command.add(pathToPagespeedBin);
    command.add("--output_format");
    command.add("formatted_proto");
    command.add("--input_format");
    command.add("har");
    command.add("--input_file");
    // Note: alternatively, can specify a final argument of "-", and
    // then send the input to pagespeed over p.getOutputStream().
    command.add(pathToHarFile);

    // Invoke the pagespeed binary.
    Runtime r = Runtime.getRuntime();
    Process p = null;
    try {
      p = r.exec(command.toArray(new String[0]));
    } catch (IOException e) {
      System.err.println(e);
      System.exit(1);
    }

    // Read the results from the pagespeed binary.
    InputStream is = p.getInputStream();
    PagespeedProtoFormatter.FormattedResults results = null;
    try {
      results = PagespeedProtoFormatter.FormattedResults.parseFrom(is);
    } catch (IOException e) {
      System.err.println("Failed to parse protocol buffer.");
      System.exit(1);
    }

    // Make sure the process exited successfully.
    int pagespeedBinReturnValue = -1;
    try {
      pagespeedBinReturnValue = p.waitFor();
    } catch (InterruptedException e) {
      System.err.println(pathToPagespeedBin + " was interrupted.");
      System.exit(1);
    }
    if (pagespeedBinReturnValue != 0) {
      System.err.println(pathToPagespeedBin
                         + " exited with non-zero status. Standard error:");
      InputStream es = p.getErrorStream();
      byte[] buffer = new byte[1024];
      int len;
      try {
        while ((len = es.read(buffer)) != -1) {
          System.err.write(buffer, 0, len);
        }
      } catch (IOException e) {
        System.err.println("Failed to read standard error stream.");
      }
      System.exit(1);
    }

    // Present the results on the console.
    PresentResults(results);
  }
}
