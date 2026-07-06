CC = gcc
CFLAGS = -Wall -Wextra -pthread

# 預設目標：編譯伺服器端與客戶端
all: server_bin client_bin

# 編譯伺服器端
server_bin: server/server.c
	$(CC) $(CFLAGS) server/server.c -o server/server

# 編譯客戶端
client_bin: client/client.c
	$(CC) $(CFLAGS) client/client.c -o client/client

# 清除編譯產物
clean:
	rm -f server/server client/client

.PHONY: all clean server_bin client_bin
