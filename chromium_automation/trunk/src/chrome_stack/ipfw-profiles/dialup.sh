#!/bin/sh
# Copyright 2011 Google Inc. All Rights Reserved.

sudo ipfw pipe 1 config bw 49000bit/s delay 60ms plr 0
sudo ipfw pipe 2 config bw 30000bit/s delay 60ms plr 0
