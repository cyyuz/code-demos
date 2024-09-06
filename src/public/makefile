# 定义一些变量来简化 Makefile
CC = g++
CFLAGS = -Wall -Wextra
SOURCES = public.cpp file.cpp
OBJECTS = $(SOURCES:.cpp=.o)
LIBRARY = libpublic.a

# 默认目标
all: $(LIBRARY)

# 创建静态库
$(LIBRARY): $(OBJECTS)
	@echo "Linking library $(LIBRARY)"
	ar rcs $@ $^
	@echo "Library $(LIBRARY) built"

# 编译规则
%.o: %.cpp
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	@echo "Cleaning up..."
	rm -f $(OBJECTS) $(LIBRARY)

# 非常干净 (删除 Makefile 产生的所有文件)
mrproper: clean
	rm -f .*.d

# 显示帮助信息
help:
	@echo "Usage:"
	@echo " make all        # To compile and link the library"
	@echo " make clean      # To remove object files and libraries"
	@echo " make mrproper   # To remove object files, libraries, and Makefile's dependencies"