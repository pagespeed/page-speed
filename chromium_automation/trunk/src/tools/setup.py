#!/usr/bin/python2.4
#
# Copyright 2011 Google Inc. All Rights Reserved.

"""Sets up everything needed to run slurped tests with apache+mod_pagespeed.

Currently this script is only able to run on Linux due to lots of native
command execution and paths.

Usage:
setup.py server # Everything needed for the server
setup.py [apache|mps|ramdisk] # Individual server components
setup.py readonly [on|off] # Sets the ModPagespeedSlurpReadOnly directive
setup.py clearram # Clears the ram disk slurp/cache data

Flags:
  --ramdisk = absolute path to the ram disk
  --ramsize = size of the ram disk in MB
"""

__author__ = "azlatin@google.com (Alexander Zlatin)"

import getpass
import os
import re
import subprocess
import sys
import urllib2


class Flags(object):
  """Command line flags parser"""

  def __init__(self):
    self.params = {}

  def define(self, name, default = None, description = ""):
    self.params[name] = [default, description]

  def parse(self, argv):
    for x in range(len(argv)):
      arg = argv[x]
      if arg.startswith("--"):
        parts = arg[2:].split("=", 2)
        name = parts[0]
        if name in self.params:
          if len(parts) == 2:
            self.params[name][0] = parts[1]  # --x=y
          elif x + 1 < len(argv) and not argv[x + 1].startswith("--"):
            x += 1
            self.params[name][0] = argv[x]  # --x y
          else:
            self.params[name][0] = True  #--x
        else:
          print "Unknown flag: %s" % name

  def usage(self):
    print "Usage:"
    for name in self.params:
      print "--%s\t%s (Current: %s)" % (name, self.params[name][1], self.params[name][0])
  
  def get(self, name):
    return self.params[name][0]


flags = Flags()

flags.define("ramdisk", "/mnt/ram", "Path to the ramdisk to use.")
flags.define("ramsize", 500, "Memory to use for the ramdisk in MB")

PAGESPEED_CONF = "/etc/apache2/mods-available/pagespeed.conf"
COMMANDS = ["server", "apache", "mps", "ramdisk",
            "readonly", "clearram", "restart"]


def main(argv):
  ram_disk, mps_cache, mps_files, mps_slurp = GetRamDiskDirs()
  ram_disk_size = flags.get("ramsize")

  if len(argv) < 2:
    print "Usage: %s [%s]" % (argv[0], "|".join(COMMANDS))
    return

  allserver = (argv[1] == "server")

  if allserver or argv[1] == "apache":
    print "Installing Apache..."
    if InstallApache():
      print "Installed Apache 2."
    else:
      print "Error Installing Apache2"
      return

  if allserver or argv[1] == "mps":
    print "Installing mod_pagespeed"
    if InstallModPagespeed():
      print "Installed mod_pagespeed"
    else:
      print "Error Installing mod_pagespeed"
      return

  if allserver or argv[1] == "ramdisk":
    print "Setting up ram disk at %s" % ram_disk
    if SetupRamdisk(ram_disk, ram_disk_size, mps_cache, mps_files, mps_slurp):
      print "Set up ram disk at %s." % ram_disk
    else:
      print "Error setting up ram disk"
      return
    print "Configuring mod_pagespeed..."
    ReplacePSConf(ModPagespeedFileCachePath="\"%s\"" % mps_cache,
                  ModPagespeedGeneratedFilePrefix="\"%s\"" % mps_files,
                  ModPagespeedSlurpDirectory="\"%s\"" % mps_slurp,
                  ModPagespeedSlurpReadOnly="on",
                  ModPagespeedDomain="*")

  if allserver or argv[1] == "clearram":
    print "Clearing Ram Disk..."
    ClearRamdisk(mps_cache, mps_files, mps_slurp)

  if argv[1] == "readonly":
    if len(argv) < 3:
      print "Usage: %s readonly [on|off]" % argv[0]
      return
    if argv[2] in ("on", "off"):
      ReplacePSConf(ModPagespeedSlurpReadOnly=argv[2])

  if allserver or argv[1] in ("mps", "ramdisk", "readonly", "restart"):
    print "Restarting Apache..."
    if RestartApache():
      print "Apache Restarted."
    else:
      print "Error Restarting Apache2: Please restart it manually."


def InstallApache():
  """Installs Apache2."""
  return subprocess.call(["sudo", "apt-get", "install", "apache2"]) == 0


def InstallModPagespeed():
  """Installs the latest mod_pagespeed beta."""
  server = "https://dl-ssl.google.com/"
  path = "dl/linux/direct/mod-pagespeed-beta_current_amd64.deb"
  pkg = urllib2.urlopen(server + path)
  f = open("/tmp/mod_pagespeed.deb", "wb")
  f.write(pkg.read())
  f.close()
  return subprocess.call(["sudo", "dpkg", "-i", "/tmp/mod_pagespeed.deb"]) == 0


def SetupRamdisk(ram_disk, ram_disk_size, *dirs):
  """Sets up a ramdisk of a set size and chowns it to the apache owner.

  Args:
    ram_disk: The path where to mount the ram disk.
    ram_disk_size: The size in MB of the ramdisk.
    *dirs: Any directories to create in addition to the ram disk.
  """
  MakeDirP(ram_disk)
  subprocess.call(["sudo", "mount",
                   "-t", "ramfs", "-o",
                   "size=%dm" % ram_disk_size, "ramfs", ram_disk])
  for path in dirs:
    MakeDirP(path)
  subprocess.call(["sudo", "chmod", "-R", "775", ram_disk])
  return subprocess.call(["sudo", "chown", "-R",
                          getpass.getuser() + ":www-data", ram_disk]) == 0


def ClearRamdisk(*dirs):
  """Clears all ramdisk data such as slurp and cache.

  Args:
    *dirs: Any directories to clear on the ram disk.
  """
  for path in dirs:
    subprocess.call(["sudo", "rm", "-rf", path + "/*"])


def ReplacePSConf(filename=None, **options):
  """Replaces settings in pagespeed.conf with new values.

  Writes a new file and overwrites the old one, since we can't open for writing
  with su permissions.

  Note: This does not add settings if they do not exist.

  Args:
    filename: The location of the pagespeed.conf file.
    **options: A dict of key word arguments of setting->value.

  Returns:
    True on success false on failure.
  """
  if not filename:
    filename = PAGESPEED_CONF
  f = open(filename, "rb")
  pagespeed_conf = f.read()
  f.close()
  for (name, val) in options.items():
    regex = re.compile("^([ \t]*)#?[ \t]*%s[ \t]+[^\s]+$" % name, re.MULTILINE)
    pagespeed_conf = regex.sub("\\1%s  %s" % (name, val), pagespeed_conf)
  outtmp = open("/tmp/pagespeed.conf", "wb")
  outtmp.write(pagespeed_conf)
  outtmp.close()
  return subprocess.call(["sudo", "mv", "/tmp/pagespeed.conf", filename]) == 0


def RestartApache():
  """Restarts the apache2 process."""
  return subprocess.call(["sudo", "/etc/init.d/apache2", "restart"]) == 0


def GetRamDiskDirs():
  """Returns a list of directories on the ram disk.

  Returns:
    ram_disk, mps_cache, mps_files, mps_slurp
  """
  ram_disk = flags.get("ramdisk").rstrip("/")
  mps_cache = ram_disk + "/mod_pagespeed/cache"
  mps_files = ram_disk + "/mod_pagespeed/files"
  mps_slurp = ram_disk + "/slurp"
  return ram_disk, mps_cache, mps_files, mps_slurp


def MakeDirP(directory):
  """Executes 'mkdir -p [dir]'."""
  if not os.path.exists(directory):
    return subprocess.call(["sudo", "mkdir", "-p", directory]) == 0
  return True


if __name__ == "__main__":
  flags.define("help", False, "View usage.")
  if flags.get("help"):
    flags.usage()
  else:
    main(sys.argv)

