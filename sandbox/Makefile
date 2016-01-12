CFLAGS += -std=c11 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -ggdb3 -O3 -fPIE -fstack-protector-all \
	  -Wall -Wextra -Wshadow -Wmissing-declarations -Wpointer-arith 
LDLIBS = -lseccomp -lsystemd
LDFLAGS += -pie -Wl,-z,relro,-z,now

sandbox: sandbox.c seccomp.c playground.c systemd.c
install: sandbox
	install -m 6755 -o root sandbox safe_runner
