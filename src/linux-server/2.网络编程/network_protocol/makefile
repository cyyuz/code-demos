INCLUDE = -I../../../public/
LDLIBS_server2 = -lpthread

# List of targets
TARGETS = http

all: $(TARGETS)

# Pattern rule for compiling C++ files
%: %.cpp
	g++ -o $@ -g $< $(INCLUDE) $(LDLIBS)

# Specific rule

clean:
	rm -f $(TARGETS)