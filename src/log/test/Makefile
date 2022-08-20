main: mtest.cc ../rlog.cc ../rlog.h
	g++ -std=c++11 -c ../rlog.cc -lpthread -I ../../lock
	g++ -std=c++11 rlog.o mtest.cc -o main -lpthread -I ../ -I ../../lock