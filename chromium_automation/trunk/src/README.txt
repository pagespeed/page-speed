Automatic Page Speed Tester
Copyright 2011 Google Inc. All Rights Reserved.
Author: azlatin@google.com (Alexander Zlatin)
################################################################################

################################################################################
INTRODUCTION
This tool allows you to run pagespeed automatically on a set of URLs in a real
Chrome instance. This data will then be aggregated into a format that can be 
analyzed by another analysis script or a CSV file viewable by any spreadsheet
software.

################################################################################
REQUIREMENTS
Chrome 15.0.839.0 (Developer Build 94659 Linux) custom
JRE + JDK 1.6
pagespeed_bin

################################################################################
BUILDING

Run the "jar" target in the build.xml ANT build file.
PageSpeedChromeRunner.jar should appear in dist/

Get Chrome:
Linux: http://build.chromium.org/f/chromium/snapshots/Linux_x64/94658/
Windows: http://build.chromium.org/f/chromium/snapshots/Win/94658/

################################################################################
USAGE
* These directions are for Linux only. It is possible to run tests on Windows
  by launching chrome and installing/configuring IPFW manually.

Basic (without IPFW):
Run "chrome-wrapper --remote-debugging-port=[port to listen on] --user-data-dir=/tmp/chrome/"
* Modify the executable name and paths as needed for your platform.

Advanced (with IPFW, Linux only):
Extract chrome to chrome_stack/chrome-linux/
Download "Source code and tools" from http://info.iet.unipi.it/~luigi/dummynet/
Put the zip in the chrome_stack directory. Make sure it is called 20*.tar
Run `install-ipfw.sh` to add IPFW to the kernel.
Run "NET=[dsl|dialup|fios] DNS=[test server] PORT=[port to listen on] ./start-chrome.sh"

Run Tests:
To get up to date usage run: java -jar PageSpeedChromeRunner.jar

################################################################################
CONFIGURATION FILES

Servers File:
One server host:port per line.
Ex:
localhost:1234
fakemachine:4321

URL File:
One URL per line.
Ex:
http://www.google.com/
http://www.bing.com/

Variations File:
One name=query string per line.
Ex:
Dogs=q=dogs
Cats=q=cats

The result will be the 4 URLS:
http://www.google.com/?q=dogs
http://www.google.com/?q=cats
http://www.bing.com/?q=dogs
http://www.bing.com/?q=cats

################################################################################
OUTPUT FILES

JSON:
This is a 5 dimensional data structure consisting of:
Object Key [str]URL:        Base URL
Object Key [str]Variation:  Variation Name
Array Key [int]Cached:      0 = First View, 1 = Repeat View (cached)
Object Key [str]Metric:     Metric Name*
Array Key [int]Run:         The run number.

CSV:
One column per test parameter/metric value*. One row per page load.

* Prefixes:
  PSI = Page Speed Impact
  PSS = Page Speed Score
  PS  = Page Speed

################################################################################
KNOWN ISSUES

It is only possible to bind Chrome's debugging socket to localhost. The
workaround is to set up a tunnel on the machines running Chrome to forward
an external socket to the localhost port. The linux server stack bundle takes
care of this.

Sites with invalid #hash parts will fail.

Sites with alert()s will fail/stall.

Sites that redirect with 302s might fail.

If a large number of remote servers is used, bottlenecks will exist in sending
all network data back to the client. Therefore, a local network is recommended.

################################################################################
DEVELOPING

Packages:
autotester
* Classes that control test scheduling, tab interaction and result parsing.
autotester.iface
* Interfaces
autotester.output
* Classes that generate the final output.
autotester.util
* Utility classes and methods.
res/
* Resources and dependencies.
lib/
* External libraries
tools/
* Tools

Flow:
* PageSpeedChromeRunner
 * Constructs TestGenerator and ServerGenerator objects.
 * Configures a URLTester to run TestRequests from TestGenerator on servers from ServerGenerator.
 * Creates a PageSpeedRunner object to convert HAR,DOM,Timeline to scores/impacts.
 * Configures OutputGenerator to use PageSpeedRunner and *Output builders.
 * Configures URLTester to notify OutputGenerator.
 * Starts URLTester
* URLTester creates a TabController worker for each server.
* TabController
 * Is responsible for running the TestRequests and storing all response data in TestResults.
 * Notifies URLTester when a test is completed.
* URLTester cycles through TabControllers as they complete tests.
 * Notifies OutputGenerator with TestResults on completion.
* OutputGenerator uses the *Output builders to build the final output files.

Output Formats:
* In order to create your own output format, all that is needed is a new OutputBuilder
 and to register it in OutputGenerator.addOutput.

See PageSpeedChromeRunner.main for high level usage of the classes.
Low level docs: http://code.google.com/chrome/devtools/docs/remote-debugging.html

################################################################################
TESTING mod_pagespeed IMPACT WITH UBUNTU

Find an Ubuntu server machine to run mod_pagespeed.
* sudo required
* Recommended 4GB+ RAM, but for small tests 1GB will do.
* It can be a VM, but for obvious reasons, a real machine is preferable.

Run setup.
* This will install Apache, mod_pagespeed and configure it to use a ramdisk.
* Run it with no parameters for a list of commands and flags.
* For defaults on a clean server: `python tools/setup.py server`

Turn on slurping.
* Run `python tools/setup.py readonly off`
* This will put mod_pagespeed in slurp mode (it will hit the live internet)

Find an Ubuntu desktop machine.
* sudo required
* Recommended 4GB+ RAM, but for small tests 1GB will do.

Checkout and build page-speed-library
* http://code.google.com/p/page-speed/wiki/HowToBuildNativeLibraries
* You will need `pagespeed_bin` for later.

Build the Page Speed Auto Tester.
* See BUILDING
* Build using `ant jar` in auto_tester/
* No ANT?
** Run `sudo apt-get install ant`
** Use the ANT build feature built into Eclipse.
* auto_tester/dist/ will contain PageSpeedChromeRunner.jar

Set up Chrome/IPFW/Dummynet.
* See USAGE -> Advanced

Slurp site data.
* On server: `python tools/setup.py readonly off`
* On client: * `java -Xmx3000m -jar auto_tester/dist/PageSpeedChromeRunner.jar --servers=servers.txt --urls=urls.txt --repeat --variations=variations.txt --output=csv,analysis`
* Slurping the variations is required so that pages that use document.location in query strings are not slurp misses, ex, http://advertiser.com/?page=http%3A%2F%2Ffoo.com%2F%3FModPagespeed%3Don
* Change runs and -Xmx*m as needed. Xmx3000m gives java 3GB of heap space.

Disable mod_pagespeed slurping.
* On server: `python tools/setup.py readonly on`

Start testing:
* `java -Xmx3000m -jar auto_tester/dist/PageSpeedChromeRunner.jar --servers=servers.txt --urls=urls.txt --ps_bin=pagespeed_bin --repeat --variations=variations.txt --runs=20 --results=~/results/ --output=csv,analysis`
* Unscientific testing has shown 15-30 runs all have about the same results.
* For best results, do not restart Chrome between slurping and testing as this will reset the Date and Math.random clamps, causing slurp misses.

Analyze the JSON data.
* Download and extract the GAE SDK
** http://code.google.com/appengine/downloads.html#Google_App_Engine_SDK_for_Python
* Run `python dev_appserver.py analysis/`

Analyze the CSV file.
* Excel or OOo Spreadsheet.

