#!/bin/bash

java -cp out/java/protobuf.jar:out/java/pagespeed_proto.jar:out/java/classes/pagespeed com.googlecode.page_speed.Pagespeed $@
