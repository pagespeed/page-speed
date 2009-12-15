#!/bin/bash

#
# Script to automatically run PageSpeed on many sites.
#
# This currently uses a list of the Alexa top 1m sites.  If the file is
# not in the cwd, it fetches it.
#
# Settings for PageSpeed and Firebug
# ----------------------------------
# There are several settings that need to be made in so that both
# Firebug and PageSpeed start automatically.  These preferences are
# set in the script below, and are documented there.
#
# Failed sites/Issues
# -------------------
# It seems that sites with redirects fail to work properly.  This is
# bad because then Firefox stays open and the script stops running.
# Fixing that is left as an exercise for the reader.
#
# Orkut redirects to Signup
# rakutan.co.jp redirects to en.rakutan.co.jp
# Most Google foreign sites
# www.uil.com.br


#
# The following variables provide sample values that are used to find
# the preferences file and the annotations file for the "PageSpeed"
# Firefox profile on a Mac.
#
# These values need to be changed to match your Firefox installation
# and platform.
#
firefox=/Applications/Firefox.app/Contents/MacOS/firefox
profile_name=PageSpeed
profile_dir=~/Library/Application\ Support/Firefox/Profiles/ud5g2w7s.PageSpeed
annotations="${profile_dir}/firebug/annotations.json"
prefs="${profile_dir}/prefs.js"
alexa_file=top-1m.csv

#
# Get the Alexa top 1m sites file
#
get_alexa()
{
  if [ -r "$alexa_file" ] ; then
    return
  fi
  curl -O "http://s3.amazonaws.com/alexa-static/top-1m.csv.zip"
  unzip top-1m.csv.zip
}

#
# Add a site to the annotations file so Firebug will automatically
# start for that site
#
addsite()
{
  site=$1
  annotations_file=$2
  grep -q "'$1/'" "${annotations_file}" || sed -i -e "1a\\
{'uri': '${site}/', 'value': 'firebugged.showFirebug'},
" "${annotations_file}"
}

#
# Set a Firebug preference
#
set_pref()
{
    pref_file=$1
    pref=$2
    value=$3

    echo "Pref was: " $(grep "${pref}" "${pref_file}")

    if grep -q "${pref}" "${pref_file}"; then
	sed -i -e "/${pref}/s/.*/user_pref(\"${pref}\", ${value});/" \
          "${pref_file}"
    else
	echo "user_pref(\"${pref}\", ${value});" >> "${pref_file}"
    fi

    echo "Pref is : " $(grep "${pref}" "${pref_file}")
}

# Score the top 100 sites
score_sites()
{
  for line in $(head -100 "${alexa_file}") ; do

    # Make sure Firebug starts automatically.
    #
    # Most of the Alexa sites names have elided the 'www', but
    # the site name does include the www.  To be safe we add both
    # urls to the annotations file.  It would be preferable to
    # enable Firebug for all urls rather than individually enabling it
    # for each url we intend to visit.  It is not clear that this is
    # possible.
    addsite ${line/*,/http://} "${annotations}"
    addsite ${line/*,/http://www.} "${annotations}"

    # Run Firefox
    ${firefox} -P ${profile_name} ${site}
  done
}

# Turn on dump logging so we can see the output.  This is not strictly
# necssary, but it helps if you are trying to debug issues.
set_pref "${prefs}" 'extensions.PageSpeed.enable_dump_logging' 'true'

# Automatically run PageSpeed.  This preference causes PageSpeed to
# run automatically when Firebug is opened.  Later we will ensure that
# Firebug is automatically opened for each URL we visit.
set_pref "${prefs}" 'extensions.PageSpeed.autorun' 'true'

# Have PageSpeed automatically send a scoring beacon to showslow.com.
# One can change the URL if you want the beacon to go to a different
# webserver.
set_pref "${prefs}" 'extensions.PageSpeed.beacon.minimal.enabled' 'true'
set_pref "${prefs}" 'extensions.PageSpeed.beacon.minimal.autorun' 'true'

# Automatically quit PageSpeed after the rules have been run and the
# results sent.
set_pref "${prefs}" 'extensions.PageSpeed.quit_after_scoring' 'true'

# The URL preference for the minimal beacon,
#    'extensions.PageSpeed.beacon.minimal.url',
# is by default set to:
#    'http://www.showslow.com/beacon/pagespeed/'
# so we do not modify it in this script.
#
# One can also set the full_results beacon preferences to have the
# full results sent to a web server.

# Make sure we have the Alexa list of top sites file around
get_alexa

# Score the sites
score_sites
