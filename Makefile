CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
sandbox: sandbox.c
	$(CC) $(CFLAGS) sandbox.c -o sandbox
clean:
	rm -f sandbox
.PHONY: clean
