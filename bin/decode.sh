#!/bin/sh
cat $1 | inflate | protoc --decode=llmr.vector.tile proto/vector_tile.proto