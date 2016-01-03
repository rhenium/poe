CFLAGS += -std=c11 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -ggdb3 -O0 -fPIE -fstack-protector-all \
	  -Wall -Wextra -Wshadow -Wmissing-declarations -Wpointer-arith 
LDLIBS = -lseccomp
LDFLAGS += -pie -Wl,-z,relro,-z,now

sandbox: sandbox.c seccomp.c playground.c
install: sandbox
	install -m 4755 -o root sandbox safe_runner
