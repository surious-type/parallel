#!/usr/bin/env bash

for f in tasks/*.lock; do mv "$f" "${f%.lock}"; done
