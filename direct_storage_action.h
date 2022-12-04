#ifndef _DIRECT_STORAGE_ACTION
#define _DIRECT_STORAGE_ACTION

#include "packet_definition.h"

#ifdef HAS_SD
  File myFile;
  void prepare_sd_card() {
    #ifdef DEBUG
      Serial.print("Initializing SD card...");
    #endif
    if (!SD.begin(4)) {
      #ifdef DEBUG
        Serial.println("initialization failed!");
      #endif
      return;
    }
    #ifdef DEBUG
      Serial.println("initialization done.");
    #endif
    SD.remove("test.t");
    myFile = SD.open("test.t", FILE_WRITE);
    if (myFile) {
      #ifdef DEBUG
        Serial.print("Writing to test.t...");
      #endif
      myFile.println(F("testing 1, 2, 3, 4, 5, 6, 7, 8, 9, 10"));
      myFile.close();
      myFile = SD.open("test.t");
      Serial.println(myFile.size());

      while (myFile.available())
        Serial.write(myFile.read());
      
      myFile.close();
    }
  }

#endif

#ifdef HAS_SD
void read_file_min_memory(ReliablePacket *packet, RF24Radio *rf) {
  printf("freeMemory: %d\n", freeMemory());
  packet->data[packet->data_len - 1] = '\0';    
  myFile = SD.open(packet->data);
  uint32_t byte_count = myFile.size();
  uint8_t packet_data_size = DATA_LEN - 1;
  uint8_t packet_count = (packet_data_size - 1 + byte_count) / (packet_data_size);
  uint8_t file_cursor = 0;
  ReliablePacket *dataPacket;
  for (uint8_t i = 0; i < packet_count; i++) {  
    dataPacket = (ReliablePacket *) malloc(sizeof(ReliablePacket));
    dataPacket->protocol = 1;
    dataPacket->action = packet->action; 
    dataPacket->isRequest = 0;

    while(file_cursor < byte_count) {
      uint8_t indexInPacket = file_cursor++ - (i * packet_data_size);
      if (indexInPacket == packet_data_size) {
        dataPacket->data[packet_data_size] = '\0';
        rf->send_queue->insert(dataPacket);
        dataPacket = NULL;
        rf->wait_send();
        file_cursor--;
        break;
      }
      char c = myFile.read();
      if (c == -1) {
        break;
      }
      dataPacket->data[indexInPacket] = c;
      dataPacket->data_len = indexInPacket + 1;
    }
  }

  if (dataPacket) {
    dataPacket->data[dataPacket->data_len] = '\0';
    rf->send_queue->insert(dataPacket);
    rf->wait_send();
  }
  myFile.close();
}
#endif


ReliablePacket **parse_ds_packet(ReliablePacket *packet, int* actionCount, RF24Radio *rf) {
  ReliablePacket **returnArray = NULL;
  #ifdef DEBUG
    printf("operation: %d\n", packet->action);
  #endif
  if (!packet->isRequest) {
    *actionCount = 0;
    return NULL;
  }

  #ifdef HAS_SD
    if (packet->action == file_exists) {
      returnArray = (ReliablePacket **)malloc(sizeof(ReliablePacket *));
      returnArray[0] = (ReliablePacket *)malloc(sizeof(ReliablePacket));
      packet->data[packet->data_len - 1] = '\0';
      bool file_exists = 1; // SD.exists(packet->data);
      returnArray[0]->protocol = 1;
      returnArray[0]->data[0] = file_exists ? 'y' : 'n';
      returnArray[0]->data_len = 1;
      returnArray[0]->action = packet->action; 
      returnArray[0]->isRequest = 0;
      *actionCount = 1;
    }
    else if (packet->action == create_file) {
      returnArray = (ReliablePacket **)malloc(sizeof(ReliablePacket *));
      returnArray[0] = (ReliablePacket *)malloc(sizeof(ReliablePacket));
      packet->data[packet->data_len - 1] = '\0';
      bool file_exists = SD.exists(packet->data);
      myFile = SD.open(packet->data, FILE_WRITE);
      myFile.close();
      returnArray[0]->protocol = 1;
      returnArray[0]->data[0] = file_exists ? 'n' : 'y';
      returnArray[0]->data_len = 1;
      returnArray[0]->action = packet->action; 
      returnArray[0]->isRequest = 0;
      *actionCount = 1;
    }
  
    else if (packet->action == append_data) {
      packet->data[packet->data_len - 1] = '\0';
      myFile = SD.open(packet->data, FILE_WRITE);
      myFile.println("testing 1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
      myFile.close();
      *actionCount = 0;
    }
    
    else if (packet->action == delete_file) {
      #ifdef DEBUG
        Serial.println(F("running delete operation"));
      #endif
      
      packet->data[packet->data_len - 1] = '\0';
      SD.remove(packet->data);
      *actionCount = 0;
    }
  
    else if (packet->action == read_file) {
      #ifdef DEBUG
        Serial.println(F("running read_file"));
      #endif

      read_file_min_memory(packet, rf);
      *actionCount = 0;
      return NULL;
      
      packet->data[packet->data_len - 1] = '\0';    
      myFile = SD.open(packet->data);
      uint32_t byte_count = myFile.size() - 2;
      uint8_t packet_data_size = DATA_LEN - 1;
      uint8_t packet_count = (packet_data_size - 1 + byte_count) / (packet_data_size);
      *actionCount = packet_count;
      returnArray = (ReliablePacket **)malloc(sizeof(ReliablePacket *) * packet_count);
      for (int i = 0; i < packet_count; i++) {
        returnArray[i] = (ReliablePacket *)malloc(sizeof(ReliablePacket));
      }
      
      ReliablePacket **returnArrayCursor = returnArray;
      uint8_t file_cursor = 0;
      for (uint8_t i = 0; i < packet_count; i++) {  
        returnArrayCursor[i]->protocol = 1;
        returnArrayCursor[i]->action = packet->action; 
        returnArrayCursor[i]->isRequest = 0;
        while(file_cursor < byte_count) {
          uint8_t indexInPacket = file_cursor++ - (i * packet_data_size);
          if (indexInPacket == packet_data_size) {
            returnArrayCursor[i]->data[packet_data_size] = '\0';
            file_cursor--;
            break;
          }
          returnArrayCursor[i]->data[indexInPacket] = myFile.read();
          returnArrayCursor[i]->data_len = indexInPacket + 1;
        }
      }
      myFile.close();
    }
    else if (packet->action == get_file_stats) {
      #ifdef DEBUG
        Serial.println(F("get_file_stats"));
      #endif
      
      returnArray = (ReliablePacket **)malloc(sizeof(ReliablePacket *));
      returnArray[0] = (ReliablePacket *)malloc(sizeof(ReliablePacket));
      packet->data[packet->data_len - 1] = '\0';
      myFile = SD.open(packet->data);
      unsigned long file_size = myFile.size() - 2;
      myFile.close();
  
      returnArray[0]->protocol = 1;
      returnArray[0]->data_len = 4;
      returnArray[0]->action = packet->action; 
      returnArray[0]->isRequest = 0;
      *actionCount = 1;    
      for (int i = 0; i < sizeof(unsigned long); i++) {
        returnArray[0]->data[i] = file_size >> (8 * i);
      }
    }
  
    else {
      #ifdef DEBUG
        Serial.println(F("Operation not identified"));
      #endif
    }
  #endif
  return returnArray;
}

#endif
