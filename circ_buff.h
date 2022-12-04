#ifndef _CIRC_BUFF
#define _CIRC_BUFF


#include <stdint.h>
#include <stdlib.h>


class CircularBuffer
{

 public:
  uint16_t consumer;
  uint16_t producer;
  void **data_buffer;
  uint16_t count;
  
  public:
    CircularBuffer(uint16_t count) {
      this->count = count;
      data_buffer = (void **) malloc(sizeof(void *) * count);
      for (uint16_t i = 0; i < this->count; i++) {
        data_buffer[i] = NULL;
      }
      producer = 0;
      consumer = 0;
    }

    int insert(void *element) {
      if (((producer + 1) % count) == consumer || !element) {
        printf("circular buffer is full, rejected value!!!!\n");
        return 1;
      }
      producer = (producer + 1) % count;
      data_buffer[producer] = element;
      return 0;
    }

    void *peek() {
      if (consumer == producer) { // empty
        return NULL;
      }
      return data_buffer[(consumer + 1) % count];
    }

    void *read() {
      if (consumer == producer) { // empty
        return NULL;
      }
      consumer = (consumer + 1) % count;
      void *read_val = data_buffer[consumer];
      data_buffer[consumer] = NULL;
      return read_val;
    }

    void skip_val() {
      void *read_val = read();
      if (read_val) {
        free(read_val);
      }
    }

    uint16_t getCount() {
      return this->count - 1;
    }

    ~CircularBuffer() {
      free(data_buffer);
    }
};

#endif
