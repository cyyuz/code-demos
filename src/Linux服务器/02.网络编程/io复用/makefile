INCLUDE = -I../../../public/

all:socket_server1 socket_server2 socket_client select poll

socket_server1:socket_server1.cpp
	g++ -o socket_server1 -g socket_server1.cpp $(INCLUDE)

socket_server2:socket_server2.cpp
	g++ -o socket_server2 -g socket_server2.cpp $(INCLUDE) -lpthread

socket_client:socket_client.cpp
	g++ -o socket_client -g socket_client.cpp $(INCLUDE)

select:select.cpp
	g++ -o select -g select.cpp $(INCLUDE)

poll:poll.cpp
	g++ -o poll -g poll.cpp $(INCLUDE)

clean:
	rm -f socket_server1 socket_server2 socket_client select poll