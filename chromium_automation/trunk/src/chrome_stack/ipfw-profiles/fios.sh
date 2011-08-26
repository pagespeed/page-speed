#!/bin/sh
# Copyright 2011 Google Inc. All Rights Reserved.

sudo ipfw pipe 1 config bw 20000000bit/s delay 2ms plr 0
sudo ipfw pipe 2 config bw 5000000bit/s delay 2ms plr 0
