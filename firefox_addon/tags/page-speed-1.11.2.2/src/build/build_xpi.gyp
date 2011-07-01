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
    'variables': {
      'xpi_stage_root%': '<(SHARED_INTERMEDIATE_DIR)/xpi',
    },
    'xpi_stage_root%': '<(xpi_stage_root)',
    'xpidl_out_dir': '<(SHARED_INTERMEDIATE_DIR)/xpidl_out',
    'xpt_output_path': '<(xpidl_out_dir)/pagespeed_firefox/xpi_resources/components',
    'archive_platform_root': '<(DEPTH)/pagespeed_firefox/xpi_resources/platform',

    # Here we list all of the file bundles that need to be copied to
    # the XPI staging location. They are listed here so they can be
    # referenced from multiple rules (once in the 'copies' rule,
    # another time as 'inputs' of the XPI generator rule). It would be
    # nice if gyp allowed us to indicate that when these files change
    # during copies, downstream dependencies should rebuild, but gyp
    # doesn't work that way.
    'xpi_files_static_root': [
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome.manifest',
      '<(DEPTH)/pagespeed_firefox/xpi_resources/icon.png',
      '<(DEPTH)/pagespeed_firefox/xpi_resources/install.rdf',
    ],
    'xpi_files_static_components': [
      '<(DEPTH)/pagespeed_firefox/js/components/componentCollectorService.js',
      '<(DEPTH)/pagespeed_firefox/js/components/stateStorageService.js',
    ],
    'xpi_files_static_content': [
      # XUL resources
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeedOverlay.xul',
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeedUtilOverlay.xul',

      # PNG resources
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeed-32.png',
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/pagespeed-64.png',
      '<(DEPTH)/pagespeed_firefox/xpi_resources/chrome/pagespeed/content/scoreIcon.png',

      # JavaScript files
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
    'xpi_files_static_preferences': [
      '<(DEPTH)/pagespeed_firefox/xpi_resources/defaults/preferences/pagespeed.js',
    ],
    'xpi_files_xpt': [
      '<(xpt_output_path)/IComponentCollector.xpt',
      '<(xpt_output_path)/IStateStorage.xpt',
    ],

    # Aggregate the files that are common to our XPI building targets,
    # so we can refer to this common variable instead of repeating the
    # lists twice.
    'xpi_files_common': [
      '<@(xpi_files_static_root)',
      '<@(xpi_files_static_components)',
      '<@(xpi_files_static_content)',
      '<@(xpi_files_static_preferences)',
      '<@(xpi_files_xpt)',
    ],
    'conditions': [
      ['OS=="mac"', {
        # On Mac, SHARED_LIB_PREFIX/SUFFIX refer to 'lib' and '.dylib'
        # but these do not match our shared library, so we manually
        # override here.
        'xpi_files_so_target': [
          '<(PRODUCT_DIR)/pagespeed.so',
        ],
      }, {
        'xpi_files_so_target': [
          '<(PRODUCT_DIR)/<(SHARED_LIB_PREFIX)pagespeed<(SHARED_LIB_SUFFIX)',
        ],
      }],
    ],
    'xpi_files_so_WINNT_x86-msvc': [
      '<(archive_platform_root)/WINNT_x86-msvc/components/pagespeed.dll',
    ],
    'xpi_files_so_Linux_x86-gcc3': [
      '<(archive_platform_root)/Linux_x86-gcc3/components/libpagespeed.so',
    ],
    'xpi_files_so_Linux_x86_64-gcc3': [
      '<(archive_platform_root)/Linux_x86_64-gcc3/components/libpagespeed.so',
    ],
    'xpi_files_so_Darwin_x86-gcc3': [
      '<(archive_platform_root)/Darwin_x86-gcc3/components/pagespeed.so',
    ],
    'xpi_files_so_Darwin_x86_64-gcc3': [
      '<(archive_platform_root)/Darwin_x86_64-gcc3/components/pagespeed.so',
    ],
    'xpi_stage_components_root': '<(xpi_stage_root)/components',
    'xpi_stage_platform_root': '<(xpi_stage_root)/platform',
  },
  'targets': [
    {
      # Copy pagespeed static files to the staging directory.
      'target_name': 'stage_xpi_static',
      'type': 'none',
      'copies': [
        {
          'destination': '<(xpi_stage_root)',
          'files': [ '<@(xpi_files_static_root)' ],
        },
        {
          'destination': '<(xpi_stage_root)/components',
          'files': [ '<@(xpi_files_static_components)' ],
        },
        {
          'destination': '<(xpi_stage_root)/chrome/pagespeed/content',
          'files': [ '<@(xpi_files_static_content)' ],
        },
        {
          'destination': '<(xpi_stage_root)/defaults/preferences',
          'files': [ '<@(xpi_files_static_preferences)' ],
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
          'destination': '<(xpi_stage_components_root)',
          'files': [ '<@(xpi_files_xpt)' ],
        },
      ],
    },
    {
      # Copy the shared object file for the current target platform to the
      # staging directory.
      'target_name': 'stage_xpi_so_target',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/pagespeed_firefox/pagespeed_firefox.gyp:pagespeed_firefox_module',
      ],
      'copies': [
        {
          'destination': '<(xpi_stage_platform_root)/<(xpcom_os)_<(xpcom_cpu_arch)-<(xpcom_compiler_abi)/components',
          'files': [ '<@(xpi_files_so_target)' ],
        },
      ],
    },
    {
      # Create the XPI.
      'target_name': 'build_xpi',
      'type': 'none',
      'dependencies': [
        'stage_xpi_static',
        'stage_xpi_xpt',
        'stage_xpi_so_target',
      ],
      'actions': [
        {
          'action_name': 'build_xpi',
          'script_name': 'generate_zip.py',
          'inputs': [
            '<@(_script_name)',
            '<@(xpi_files_common)',
            '<@(xpi_files_so_target)',
          ],
          'outputs': [ '<(PRODUCT_DIR)/page-speed.xpi' ],
          'action': [ 'python', '<@(_script_name)', '<@(_outputs)', '<(xpi_stage_root)' ],
        },
      ],
    },
    {
      # Copy the shared object files for all platforms to the staging directory
      # and create the XPI.
      'target_name': 'build_release_xpi',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'stage_xpi_static',
        'stage_xpi_xpt',
      ],
      'copies': [
        {
          'destination': '<(xpi_stage_platform_root)/WINNT_x86-msvc/components/',
          'files': [ '<@(xpi_files_so_WINNT_x86-msvc)' ],
        },
        {
          'destination': '<(xpi_stage_platform_root)/Linux_x86-gcc3/components/',
          'files': [ '<@(xpi_files_so_Linux_x86-gcc3)' ],
        },
        {
          'destination': '<(xpi_stage_platform_root)/Linux_x86_64-gcc3/components/',
          'files': [ '<@(xpi_files_so_Linux_x86_64-gcc3)' ],
        },
        {
          'destination': '<(xpi_stage_platform_root)/Darwin_x86-gcc3/components/',
          'files': [ '<@(xpi_files_so_Darwin_x86-gcc3)' ],
        },
        {
          'destination': '<(xpi_stage_platform_root)/Darwin_x86_64-gcc3/components/',
          'files': [ '<@(xpi_files_so_Darwin_x86_64-gcc3)' ],
        },
      ],
      'actions': [
        {
          'action_name': 'build_release_xpi',
          'script_name': 'generate_zip.py',
          'inputs': [
# TODO: the gyp make generator is broken and doesn't respect the
# suppress_wildcard flag for actions. In the meantime these are
# commented out.
#            '<@(_script_name)',
#            '<@(xpi_files_common)',
#            '<@(xpi_files_so_WINNT_x86-msvc)',
#            '<@(xpi_files_so_Linux_x86-gcc3)',
#            '<@(xpi_files_so_Linux_x86_64-gcc3)',
#            '<@(xpi_files_so_Darwin_x86-gcc3)',
#            '<@(xpi_files_so_Darwin_x86_64-gcc3)',
          ],
          'outputs': [ '<(PRODUCT_DIR)/page-speed-release.xpi' ],
          'action': [ 'python', '<@(_script_name)', '<@(_outputs)', '<(xpi_stage_root)' ],
        },
      ],
    },
  ],
}
