CFLAGS ?= -std=c11 -g -O3 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -fstack-protector-all \
	  -Wall -Wextra -Wpedantic -Wshadow -Wmissing-declarations -Wpointer-arith -Wunused-macros
LDFLAGS ?= -pie -Wl,-z,relro,-z,now -lseccomp -lsystemd

OBJS := main.o playground.o seccomp.o cgroup.o child.o
DEPS := sandbox.h config.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
sandbox: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
install: sandbox
	install -m 6755 -o root sandbox runner
clean:
	$(RM) runner sandbox
	$(RM) *.o
