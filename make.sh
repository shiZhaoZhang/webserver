#!/bin/bash
g++ -g -std=c++11 ./mysql/connect_pool.cpp ./log/rlog.cc ./http/httpconn.cpp ./http/httpRequestParser.cpp main.cpp -o main -I ./http/ -I ./lock/ -I ./log -I ./mysql -I ./threadPool -I ./timer -I ./correctip -lpthread `mysql_config --cflags --libs`