#!/bin/sh -eux

PRODUCTION_HOST=malachite.rss.chalmers.se
REMOTE_DIR=/etc/docker-compose/odincal
REMOTE_COMPOSE_FILE=${REMOTE_DIR}/docker-compose.yml
GIT_ROOT=$(git rev-parse --show-toplevel)

remoteDockerCompose () {
    ssh odinop@"$PRODUCTION_HOST" -t -- docker-compose -f "$REMOTE_COMPOSE_FILE" "$*"
}

scp "${GIT_ROOT}"/scripts/docker-compose.deploy.yml odinop@"$PRODUCTION_HOST:$REMOTE_COMPOSE_FILE"
remoteDockerCompose pull
remoteDockerCompose up -d --remove-orphans
remoteDockerCompose ps
