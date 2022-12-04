#include "packet_definition.h"
#include "circ_buff.h"


#define MAX_PAYLOAD_SIZE 32

class RF24Radio {
  private:
    uint8_t current_seq_num = 0;
    uint8_t current_ack_num = 0;
    uint32_t lastWrite = 0;
    uint32_t sendCount = 0;
    volatile bool writeLock = false;
    uint32_t startWriteTime;

    void handle_rx() {
      if (radio->getDynamicPayloadSize()) {
        RawPacket *r_packet = (RawPacket *)malloc(radio->getDynamicPayloadSize());
        radio->read(r_packet, radio->getDynamicPayloadSize());
        recv_queue->insert(r_packet);
        if (r_packet->seq_num <= current_ack_num) {
          current_ack_num = r_packet->seq_num + 1;
        }
      }
    }

    void handle_tx(bool tx) {
      radio->startListening();
      #ifdef DEBUG
        //printf("Write Time: %lu microseconds\n", micros() - startWriteTime);
        //Serial.print(F("tx"));
        //Serial.println(tx ? F(":OK") : F(":Fail"));
      #endif
      if (tx){
        send_queue->skip_val();
      }
    }

    void init_radio(uint8_t *address) {
        radio->begin();
        radio->enableDynamicPayloads();
        radio->setAutoAck(1);
        radio->setRetries(15,15);
        radio->setPayloadSize(MAX_PAYLOAD_SIZE);
        radio->openWritingPipe(address);
        radio->openReadingPipe(1,address);
        radio->setPALevel(RF24_PA_MAX);
        radio->setDataRate(RF24_2MBPS);
        radio->startListening();
        radio->printDetails();    
    }

    void delay_us() {
      for(uint32_t i=0; i<130;i++){
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
      }
    }

  public:
    CircularBuffer *send_queue;
    CircularBuffer *recv_queue;
    RF24 *radio;

    RF24Radio(uint16_t queue_size, uint8_t cepin, uint8_t cspin, uint8_t *address) {
      send_queue = new CircularBuffer(queue_size);
      recv_queue = new CircularBuffer(queue_size);
      radio = new RF24(cepin, cspin);
      init_radio(address);
    }

    ~RF24Radio() {
      delete send_queue;
      delete recv_queue;
      delete radio;
    }
  
    void check_radio(void){
      bool tx,fail,rx;
      radio->whatHappened(tx,fail,rx);
      if ( rx )
        handle_rx();
      if( tx || fail )
        handle_tx(tx);
      writeLock = false;
    }

    bool run_cycle() {
      void *peek_val = send_queue->peek();
      if (peek_val) {
        if (!writeLock) {
          radio->stopListening();
          delay_us();
          lastWrite = millis();
          ((RawPacket *)peek_val)->seq_num = current_seq_num++;
          ((RawPacket *)peek_val)->ack_num = current_ack_num;
          #ifdef DEBUG
            printf("send sequence_num: %d\n", ((RawPacket *)peek_val)->seq_num);
          #endif
          startWriteTime = micros();
          radio->startWrite(peek_val, ((RawPacket *)peek_val)->data_len + 4,0);
          writeLock = true;
          return 0;
        }
      }
      return 1;
    }

    void *blocking_request(void *request) {
        send_queue->insert(request);
        wait_send();
        wait_recv();
        return recv_queue->read();
    }

    void wait_send() {
      while(run_cycle())
          __asm__("nop");
    }

    void wait_recv() {
      while(!recv_queue->peek())
          __asm__("nop");
    }
};



