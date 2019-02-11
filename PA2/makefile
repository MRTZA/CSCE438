all: fbc

fbc: tsc.cc
	g++ -lpthread -std=c++11 -o fbc tsc.cc

clean: 
	rm -rf fbc

#protoc -I ../../protos --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../../protos/route_guide.proto
#protoc -I ../../protos --cpp_out=. ../../protos/route_guide.proto