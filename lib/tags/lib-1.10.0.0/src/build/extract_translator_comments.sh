#!/bin/bash
#
# Copyright 2009 Google Inc.
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

# Extracts translator comments (comments starting with "TRANSLATOR: ") into the
# src/pagespeed/po/pagespeed.pot file.

POT=pagespeed/po/pagespeed.pot
SOURCES=$(find pagespeed -name '*.cc')

echo "Extracting strings from:"
echo "$SOURCES"

# Invoke xgettext.  The flags are,
#  -s : sort output by string
#  --no-location : don't output "#: <file>:<line>" for each string
#  --no-wrap : don't wrap long strings over multiple lines
#  --omit-header : don't output the initial header
#  -cTRANSLATOR : extract comments starting with "TRANSLATOR"
#  -d pagespeed : set the gettext domain to "pagespeed"
#  -k_ : extract strings marked by the _(...) macro
xgettext -s --no-location --no-wrap --omit-header -cTRANSLATOR \
  -d pagespeed -k_ -o $POT $SOURCES

# remove the "TRANSLATOR:" from comments
perl -pi -e 's/TRANSLATOR: //g;' $POT

# change extracted comments ("#. ...") into translator comments ("#  ...")
perl -pi -e 's/^#\. /#  /g;' $POT

echo "Done!"
