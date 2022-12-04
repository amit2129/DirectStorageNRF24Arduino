#include <SPI.h>
#define DEBUG
#define HAS_SD

#ifdef HAS_SD
  #include <SD.h>
#endif

#include "free_memory.h"
#include "RF24.h"
#include <printf.h>
#include "circ_buff.h"
#include "packet_definition.h"
#include "nrf24_comm.h"
#include "direct_storage_action.h"

RF24Radio *rf;

void setup(){
  Serial.begin(115200);
  printf_begin();
  uint8_t address[] = { "radio" };
  rf = new RF24Radio(30, 9, 10, address);
  attachInterrupt(0, radio_event, LOW); // radio events will call radio_event
  
  #ifdef HAS_SD
    prepare_sd_card();
  #endif
  printf("freeMemory: %d\n", freeMemory());
}


void *getNextAction() {
  ParsedAction parsedAction;
  if (parseInputAction(&parsedAction) == 0) {
    void *packet_pointer;
    switch(parsedAction.protocol) {
      case 0:
        //printf("packet is of type 0\n");
        packet_pointer = malloc(sizeof(RawPacket));
        ((RawPacket *)packet_pointer)->protocol = parsedAction.protocol;
        ((RawPacket *)packet_pointer)->data_len = parsedAction.data_len;
        memcpy(packet_pointer + offsetof(RawPacket, data), parsedAction.input_str, parsedAction.data_len);
        break;

      case 1:
        //printf("packet is of type 1\n");
        packet_pointer = malloc(sizeof(ReliablePacket));
        ((ReliablePacket *)packet_pointer)->protocol = parsedAction.protocol;
        ((ReliablePacket *)packet_pointer)->action = parsedAction.action;
        ((ReliablePacket *)packet_pointer)->isRequest = 1;
        ((ReliablePacket *)packet_pointer)->data_len = parsedAction.data_len;
        memcpy(packet_pointer + offsetof(ReliablePacket, data), parsedAction.input_str, parsedAction.data_len);
        break;
    }
    
    return packet_pointer;
  }
  return NULL;
}

void loop() {
  ReliablePacket *nextAction = getNextAction();
  if (nextAction) {
    if (nextAction->protocol == 1 && nextAction->action == read_file){
      getFileContentsBlocking(nextAction);
    }
    else {
      rf->send_queue->insert(nextAction);
    }
    
  }
//  printf("freeMemory: %d\n", freeMemory());
  rf->run_cycle();
  recv_async();
} 

void handle_packet(RawPacket *packet) {
  switch(packet->protocol) {
    case 0:
      //printf("received raw packet\n");
      break;
    case 1:
      //printf("received ds action\n");
      int actionCount = 0;
      ReliablePacket **packetsToPush = parse_ds_packet((ReliablePacket *)packet, &actionCount, rf);
      if (actionCount) {
        for (int i = 0; i < actionCount; i++) {
          rf->send_queue->insert(packetsToPush[i]);
          //printf("pushing packet:\n");
          print_raw_packet((RawPacket *)(packetsToPush[i]));
        }
        free(packetsToPush);
      }

      break;
  }
}


void getFileContentsBlocking(ReliablePacket *get_file_packet) {
  printf("freeMemory: %d\n", freeMemory());
  ReliablePacket *get_file_size = (ReliablePacket *) malloc(sizeof(ReliablePacket));
  memcpy(get_file_size, get_file_packet, sizeof(ReliablePacket));
  get_file_size->action = get_file_stats;


  ReliablePacket *response = rf->blocking_request(get_file_size);
  uint32_t file_size = *((uint32_t *)response->data);
  free(response); response = NULL;
  
  char *file_contents = (char *) malloc(file_size + 1);
  uint8_t packet_data_size = DATA_LEN - 1;
  uint8_t packet_count = (packet_data_size - 1 + file_size) / (packet_data_size);

  rf->send_queue->insert(get_file_packet);
  rf->wait_send();

  // waiting to receive <packet_count> packets
  for (int i = 0; i < packet_count; i++) {
    rf->wait_recv();
    response = rf->recv_queue->read();
    response->data[response->data_len] = '\0';
    memcpy(file_contents + (i * packet_data_size), response->data, strlen(response->data));
    free(response); response = NULL;
  }
  file_contents[file_size] = '\0';

  printf("received file\n\tsize:\t\t%lu \n\tcontents:\t%s\n", file_size, file_contents);
  free(file_contents);
 
}


void recv_async() {
  void *read_val = rf->recv_queue->read();
  if (read_val) {
    handle_packet(read_val);
    switch(((RawPacket *)read_val)->protocol) {
      case 0:
        //printf("print_raw_packet\n");
        print_raw_packet(read_val);
        break;
      case 1:
        //printf("print_packet\n");
        //print_packet(read_val);
        break;
    }
    free(read_val);
  }
}


void radio_event(void) {
  rf->check_radio();
}

