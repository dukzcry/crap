EILIB_CFLAGS := ${EILIB_PATH}/include
EILIB_LDFLAGS := ${EILIB_PATH}/lib

all: compile

compile:
	@echo ${EILIB_PATH}
	cc -I${EILIB_CFLAGS} -Wl,-L${EILIB_LDFLAGS} \
		-lzmq -lczmq -lreadline -lncurses \
		-lerl_interface -lei \
		main.c -o cli

clean:
	rm cli
