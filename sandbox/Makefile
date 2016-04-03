CFLAGS ?= -std=c11 -g -O3 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -fstack-protector-all \
	  -Wall -Wextra -Wpedantic -Wshadow -Wmissing-declarations -Wpointer-arith -Wunused-macros -Wstrict-aliasing=2
LDFLAGS ?= -pie -Wl,-z,relro,-z,now -lseccomp

DEPS := sandbox.h config.h utils.h
OBJS := main.o playground.o seccomp.o cgroup.o child.o

all: sandbox
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
sandbox: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
install: all
	install -m 6755 -o root sandbox runner
clean:
	$(RM) runner sandbox
	$(RM) *.o
.PHONY: all install clean sandbox

# per-object dependencies
seccomp.o: config_seccomp.h
