#!/usr/bin/env bash
docker build -t cpp-dev-tools .
docker run -p 0.0.0.0:2222:22 -p 0.0.0.0:5000:5000 --name cpp-dev cpp-dev-tools