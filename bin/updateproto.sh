#!/bin/sh
cd proto
protoc metrics.proto --cpp_out=../src
protoc vector_tile.proto --cpp_out=../src