#!/bin/bash
set -x
cp sbridge.conf /etc/default/
cp sbridge.init.debian /etc/init.d/
cp sbridge.db /etc/
set +x
