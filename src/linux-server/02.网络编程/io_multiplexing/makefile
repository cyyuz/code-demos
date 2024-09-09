INCLUDE = -I../../../public/
LDLIBS_server2 = -lpthread

# List of targets
TARGETS = socket_server1 socket_server2 socket_client select poll epoll reactor1 mul_port_client_epoll

all: $(TARGETS)

# Pattern rule for compiling C++ files
%: %.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS)

# Specific rule for socket_server2
socket_server2: socket_server2.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS_server2)

clean:
	rm -f $(TARGETS)