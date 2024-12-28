#!/bin/sh

# If you want to clean up all local images, run this:
# `podman image prune --force && podman rmi --all --force`

# podman does not replace pods, so call down on them just to be safe
podman compose -f compose_elasticsearch.yaml down
podman compose -f compose_mesh-import.yaml down
podman compose -f compose_mesh.yaml down

# start elasticsearch
podman compose -f compose_elasticsearch.yaml up -d &&
# wait until elasticsearch has started
sleep 5 &&
# import mesh data. Use run instead of up to see progress. This pod will exit when import is done
podman compose -f compose_mesh-import.yaml run mesh-import &&
# start the MeSH web application, listening on HTTP port 8080 and HTTPS port 8443
podman compose -f compose_mesh.yaml up -d
