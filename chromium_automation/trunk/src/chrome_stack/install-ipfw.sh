#!/bin/sh
# Copyright 2011 Google Inc. All Rights Reserved.

tar -C /tmp -xvzf 20??????-ipfw3.tgz
cd /tmp/ipfw3
make
sudo insmod dummynet2/ipfw_mod.ko
sudo cp ipfw/ipfw /usr/local/sbin
