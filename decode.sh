#!/bin/sh
cat out.sdf | inflate | protoc --decode=llmr.vector.tile proto/vector_tile.proto