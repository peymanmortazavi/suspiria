#!/usr/bin/env bash
docker run -p 0.0.0.0:2222:22 -p 0.0.0.0:1500:1500 --cap-add=SYS_PTRACE --name suspiria-dev suspiria-dev-image