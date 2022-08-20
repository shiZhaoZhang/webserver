#!/bin/bash
./main syn 1000000 100000
rm log/*
./main syn 1000000 1000000
rm log/*
./main syn 1000000 10000000
rm log/*
./main asyn 1000000 100000
rm log/*
./main asyn 1000000 1000000
rm log/*
./main asyn 1000000 10000000
rm log/*