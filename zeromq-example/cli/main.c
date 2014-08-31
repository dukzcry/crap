#include <czmq.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <curses.h>

#undef ETERM
#include "erl_interface.h"

#include "config.h"

/* zlist.c */
struct node_t {
    struct node_t
        *next;
    void
        *item;
};
struct _zlist {
    struct node_t
        *head, *tail;
    struct node_t
        *cursor;
    size_t
        size;
};
/* */
typedef struct {
	void *data;
	size_t size;
} item_t;
#define FOR_EACH(item, list) \
    for(item = list->head; item != NULL; item = item->next)

void *async_thread(void *arg)
{
	void *reader_async = arg;
	char *msg = NULL;

	for (;;) {
		msg = zstr_recv (reader_async);
		if (msg) {
			printf("%s\n", msg);
			free (msg);
		}
		msg = NULL;
	}

	return NULL;
}

int
main(void)
{
	zctx_t *ctx;
	zmsg_t *zmsg;
	zlist_t *list;
	struct node_t *item;
	pthread_t thread;
	void *arg, *reader, *reader_async;
	int rc, i;

	ctx = zctx_new ();
	list = zlist_new ();
	erl_init(NULL,0);

	reader = zsocket_new (ctx, ZMQ_REQ);
	reader_async = zsocket_new (ctx, ZMQ_SUB);
	/* Timeout */
	zsocket_set_rcvtimeo (reader, 60 * 1000);
	/* Subscribe to all msgs */
	zmq_setsockopt (reader_async, ZMQ_SUBSCRIBE, NULL, 0);

	rc = zsocket_connect (reader, CHANNEL);
	assert (rc == 0);

	/* Example */
	ETERM *terms[3];
	terms[0] = erl_mk_atom("tobbe");
	terms[1] = erl_mk_atom("lmao");
	terms[2] = erl_mk_int(1234);
	unsigned char buf1[erl_term_len(terms[0])];
	unsigned char buf2[erl_term_len(terms[1])];
	unsigned char buf3[erl_term_len(terms[2])];
	i = erl_encode(terms[0], buf1);
	item_t item1 = {
		buf1,
		i
	};
	i = erl_encode(terms[1], buf2);
	item_t item2 = {
		buf2,
		i
	};
	i = erl_encode(terms[2], buf2);
	item_t item3 = {
		buf2,
		i
	};	
	zlist_append (list, &item1);
	zlist_append (list, &item2);
	zlist_append (list, &item3);
	/* */

	zmsg = zmsg_new ();
	FOR_EACH(item, list)
		zmsg_addmem (zmsg, ( *((item_t *)item->item) ).data, 
			( *((item_t *)item->item) ).size);
	rc = zmsg_send (&zmsg, reader);
	assert (rc == 0);
	zmsg_destroy (&zmsg);

	zmsg = zmsg_recv (reader);
	assert (zmsg);
	rc = zmsg_size(zmsg);
	for (i = 0; i < rc; i++)
		printf ("%d: %s\n", i, zmsg_popstr(zmsg));
	zmsg_destroy (&zmsg);

	rc = zsocket_connect (reader_async, CHANNEL_AEN);
	assert (rc == 0);

	arg = reader_async;
	rc = pthread_create(&thread, NULL, async_thread, arg);
	assert (rc == 0);

	sleep(60);

	zctx_destroy (&ctx);
	zlist_destroy (&list);
	return 0;
}
