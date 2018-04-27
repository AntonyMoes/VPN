#!/bin/bash
openvpn --mktun --dev $1
ip link set $1 up
ip addr add $2/24 dev $1
