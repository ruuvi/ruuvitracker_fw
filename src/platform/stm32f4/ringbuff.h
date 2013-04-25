#ifndef RINGBUFF_H
#define RINGBUFF_H

struct rbuff {
  unsigned char *data;
  volatile int top;
  volatile int bottom;
  int size;
};

struct rbuff *rbuff_new(int size);
void rbuff_delete(struct rbuff *p);
void rbuff_push(struct rbuff *p, unsigned char c);
unsigned char rbuff_pop(struct rbuff *p);
unsigned char rbuff_peek(struct rbuff *p);
int rbuff_is_empty(struct rbuff *p);
int rbuff_is_full(struct rbuff *p);
char *rbuff_get_raw(struct rbuff *p);
void rbuff_remove(struct rbuff *p, int n);

#endif	/* RINGBUFF_H */
