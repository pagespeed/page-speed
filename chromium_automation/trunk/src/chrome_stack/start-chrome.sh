#!/bin/sh
# Copyright 2011 Google Inc. All Rights Reserved.

USERDATA_DIR="/tmp/chrome-$RANDOM/"
HOSTNAME=`hostname -s`
WEB_PORTS="80,445,53"

# Forward external port to internal port since Chrome only listens on localhost
ssh -L 0.0.0.0:$PORT:localhost:9222 $HOSTNAME -N &
TUNNEL_PID=$!

# Get rid of old rules
sudo ipfw -q flush
sudo ipfw -q pipe flush

# Create pipes and queues
sudo ipfw pipe 1 config delay 0ms noerror
sudo ipfw pipe 2 config delay 0ms noerror
sudo ipfw queue 1 config pipe 1 queue 100 noerror mask dst-port 0xffff
sudo ipfw queue 2 config pipe 2 queue 100 noerror mask src-port 0xffff

# Whitelist local traffic
sudo ipfw add 1000 allow all from any to any via lo
sudo ipfw add 1001 allow all from any to any via lo0
sudo ipfw add skipto 60000 src-ip 127.0.0.1/8
sudo ipfw add skipto 60000 dst-ip 127.0.0.1/8

# Throttle traffic
sudo ipfw add queue 1 ip from any $WEB_PORTS to any in
sudo ipfw add queue 2 ip from any to any $WEB_PORTS out

sudo ipfw add 60000 allow ip from any to any

# Set bandwidth and latency on pipes
./ipfw-profiles/$NET.sh

# Start chrome with flags to reduce test variation
./chrome-linux/chrome-wrapper --remote-debugging-port=9222 --host-rules="MAP * $DNS" --user-data-dir=$USERDATA_DIR --dns-prefetch-disable --no-js-randomness --no-message-box --no-service-autorun --noerrdialogs --no-default-browser-check --disabled --dns-prefetch-disable --disable-preconnect --disable-logging --disable-client-side-phishing-detection --bwsi --no-first-run --no-pings

# Kill ssh tunnel
kill $TUNNEL_PID

# Remove ipfw rules
./ipfw-profiles/remove.sh

# Remove Chrome data directory
rm -rf $USERDATA_DIR
