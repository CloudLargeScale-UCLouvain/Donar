#!/usr/bin/bash

mkdir -p release
docker run \
    --rm \
    -it \
    -v `pwd`/release:/tmp/release \
    registry.gitlab.inria.fr/qdufour/donar \
    cp \
        /usr/local/bin/{tor2,tor3,dcall,donar,measlat} \
        /etc/{torrc_guard_12,torrc_guard_2,torrc_single_hop_12} \
        /tmp/release
zip -r donar.zip release/
