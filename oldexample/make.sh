#!/bin/bash
g++ -g -std=c++11 ./src/mysql/connect_pool.cpp ./src/log/rlog.cc ./src/http/httpconn.cpp ./src/http/httpResponse.cpp ./src/http/httpRequestParser.cpp main.cpp -o main -I ./src/http/ -I ./src/lock/ -I ./src/log -I ./src/mysql -I ./src/threadPool -I ./src/timer -I ./src/correctip -lpthread -lcrypto -lssl `mysql_config --cflags --libs`