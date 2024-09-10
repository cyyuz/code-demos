INCLUDE = -I../../../public/
LDLIBS_server2 = -lpthread

# List of targets
TARGETS = socket_server socket_server_thread socket_client select poll epoll reactor_single_thread server_mulport_epoll mul_port_client_epoll

all: $(TARGETS)

# Pattern rule for compiling C++ files
%: %.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS)

# Specific rule
socket_server_thread: socket_server_thread.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS_server2)

server_mulport_epoll: server_mulport_epoll.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS_server2)

clean:
	rm -f $(TARGETS)