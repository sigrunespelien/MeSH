#!/bin/sh

# podman image prune --force && podman rmi --all --force

podman compose -f compose_elasticsearch.yaml down
podman compose -f compose_mesh-import.yaml down
podman compose -f compose_mesh.yaml down

podman compose -f compose_elasticsearch.yaml up -d &&
sleep 5 &&
podman compose -f compose_mesh-import.yaml run mesh-import &&
podman compose -f compose_mesh.yaml up -d
