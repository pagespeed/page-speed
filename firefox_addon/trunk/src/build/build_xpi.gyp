# Copyright 2011 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

{
  'variables': {
    'xpi_stage_root': '<(SHARED_INTERMEDIATE_DIR)/xpi',
    'xpidl_out_dir': '<(SHARED_INTERMEDIATE_DIR)/xpidl_out',
  },
  'targets': [
    {
      # Copy pagespeed static files to the staging directory.
      'target_name': 'stage_xpi_static',
      'type': 'none',
      'copies': [
        {
          'destination': '<(xpi_stage_root)',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome.manifest',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/icon.png',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/install.rdf',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/components',
          'files': [
            '<(DEPTH)/pagespeed_firefox/js/components/componentCollectorService.js',
            '<(DEPTH)/pagespeed_firefox/js/components/stateStorageService.js',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/chrome/pagespeed/content',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeed-32.png',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeed-64.png',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeedOverlay.xul',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeedUtilOverlay.xul',
            '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/scoreIcon.png',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/chrome/pagespeed/content',
          'files': [
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/beaconTraits.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/callbackHolder.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/cssEfficiencyChecker.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/deps.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/fullResultsBeacon.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/jsCoverageLint.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/lint.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/minimalBeacon.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/pageLoadTimer.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/pagespeedClientApi.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/pagespeedContext.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/pagespeedLibraryRules.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/pagespeedPanel.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/parallelXhrFlow.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/resultsContainer.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/resultsWriter.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/unusedCssLint.js',
            '<(DEPTH)/pagespeed_firefox/js/pagespeed/util.js',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/defaults/preferences',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/defaults/preferences/pagespeed.js',
          ],
        },
      ],
    },
    {
      # Copy pagespeed generated XPT files to the staging directory.
      'target_name': 'stage_xpi_xpt',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/pagespeed_firefox/pagespeed_firefox.gyp:pagespeed_firefox_genxpt',
      ],
      'copies': [
        {
          'destination': '<(xpi_stage_root)/components',
          'files': [
            '<(xpidl_out_dir)/pagespeed_firefox/xpi_resources/components/IComponentCollector.xpt',
            '<(xpidl_out_dir)/pagespeed_firefox/xpi_resources/components/IPageSpeedRules.xpt',
            '<(xpidl_out_dir)/pagespeed_firefox/xpi_resources/components/IStateStorage.xpt',
          ],
        },
      ],
    },
    {
      # Copy the shared object file for the current target platform to the
      # staging directory and create the XPI.
      'target_name': 'build_xpi',
      'type': 'none',
      'dependencies': [
        'stage_xpi_static',
        'stage_xpi_xpt',
        '<(DEPTH)/pagespeed_firefox/pagespeed_firefox.gyp:pagespeed_firefox_module',
      ],
      'copies': [
        {
          'destination': '<(xpi_stage_root)/platform/<(xpcom_os)_<(xpcom_cpu_arch)-<(xpcom_compiler_abi)/components',
          'files': [
            '<(PRODUCT_DIR)/<(SHARED_LIB_PREFIX)pagespeed<(SHARED_LIB_SUFFIX)',
          ],
        },
      ],
      'actions': [
        {
          'action_name': 'build_xpi',
          'script_name': 'generate_zip.py',
          'inputs': [ '<@(_script_name)' ],
          'outputs': [ '<(PRODUCT_DIR)/page-speed.xpi' ],
          'action': [ 'python', '<@(_script_name)', '<@(_outputs)', '<(xpi_stage_root)' ],
        },
      ],
    },
    {
      # Copy the shared object files for all platforms to the staging directory
      # and create the XPI.
      'target_name': 'build_xpi_all',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'stage_xpi_static',
        'stage_xpi_xpt',
      ],
      'copies': [
        {
          'destination': '<(xpi_stage_root)/platform/WINNT_x86-msvc/components/',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/WINNT_x86-msvc/components/pagespeed.dll',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/platform/Linux_x86-gcc3/components/',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/Linux_x86-gcc3/components/libpagespeed.so',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/platform/Linux_x86_64-gcc3/components/',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/Linux_x86_64-gcc3/components/libpagespeed.so',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/platform/Darwin_x86-gcc3/components/',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/Darwin_x86-gcc3/components/libpagespeed.dylib',
          ],
        },
        {
          'destination': '<(xpi_stage_root)/platform/Darwin_x86_64-gcc3/components/',
          'files': [
            '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/Darwin_x86_64-gcc3/components/libpagespeed.dylib',
          ],
        },
      ],
      'actions': [
        {
          'action_name': 'build_xpi_all',
          'script_name': 'generate_zip.py',
          'inputs': [ '<@(_script_name)' ],
          'outputs': [ '<(PRODUCT_DIR)/page-speed.xpi' ],
          'action': [ 'python', '<@(_script_name)', '<@(_outputs)', '<(xpi_stage_root)' ],
        },
      ],
    },
  ],
}
