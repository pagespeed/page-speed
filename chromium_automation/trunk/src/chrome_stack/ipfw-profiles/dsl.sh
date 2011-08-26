#!/bin/sh
# Copyright 2011 Google Inc. All Rights Reserved.

sudo ipfw pipe 1 config bw 1500000bit/s delay 25ms plr 0
sudo ipfw pipe 2 config bw 384000bit/s delay 25ms plr 0
