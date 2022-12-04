#ifndef _PACKET_DEFINITION
#define _PACKET_DEFINITION

#define DATA_LEN 28


typedef struct {
  uint8_t seq_num;
  uint8_t ack_num;
  uint8_t protocol:3;
  uint8_t action:4;
  uint8_t isRequest:1;
  uint8_t data_len;
  uint8_t data[DATA_LEN];
} ReliablePacket;

typedef struct {
  uint8_t seq_num;
  uint8_t ack_num;
  uint8_t protocol:3;
  uint8_t data_len;
  uint8_t data[DATA_LEN];
} RawPacket;


typedef struct _parsed_action {
  uint8_t protocol;
  uint8_t action:7;
  uint8_t data_len;
  char input_str[DATA_LEN];
} ParsedAction;


typedef enum {raw_packet=0, directStorage=1} protocol_type;
typedef enum {file_exists, create_file, delete_file, append_data, get_file_stats, read_file, list_files, response} file_op;




uint8_t parseInputAction(ParsedAction *action) {
  uint32_t initial_micros = micros();
  if (!Serial.available()) {
    return -1;
  }

  else {
    #ifdef DEBUG
      Serial.println(F("parsing input\n"));
    #endif
    
    while(!Serial.available()) { }
    action->protocol = (uint8_t )Serial.read() - '0';

    if (action->protocol != 0) {
      while(!Serial.available()) { }
      action->action = (uint8_t)Serial.read() - '0';
    }
    else {
      action->action = 0;
    }

    uint8_t name_cursor = 0;
    while(true) {
      while(!Serial.available()) { }    
      char read_char = Serial.read();
      if (read_char == '\n') {
        action->input_str[name_cursor] = '\0';
        break;
      }
       if (name_cursor == DATA_LEN - 1) {
        while(Serial.read() != '\n') {
          
        }
        return -1;
      }
      action->input_str[name_cursor++] = read_char;
    }
    action->data_len = name_cursor + 1;
  }
  
  uint32_t diff_micros = micros() - initial_micros;
  #ifdef DEBUG
      printf("protocol:\t%d\naction:\t\t%d\ninput_str:\t%s\n\nprocessing_time: %lu microseconds\n\n", action->protocol, action->action, action->input_str, diff_micros);
  #endif
  
  return 0;
}


void print_packet(ReliablePacket *packet) {
  #ifdef DEBUG
    Serial.println(F("Packet: {"));
    Serial.print(F("\tseq_num:\t\t"));
    Serial.println(packet->seq_num);
    Serial.print(F("\tack_num:\t\t"));
    Serial.println(packet->ack_num);
    Serial.print(F("\taction:\t\t\t"));
    Serial.println(packet->action);
    Serial.print(F("\tdata_len:\t\t"));
    Serial.println(packet->data_len);
  
    Serial.print(F("data: "));
    for (uint8_t i = 0; i < DATA_LEN; i++) {
      Serial.print(packet->data[i]);
      Serial.print(F(" "));
    }
  
    
    Serial.println("data:");
    if (packet->action == get_file_stats) {
      unsigned long file_size = 0;
      for (int i = 0; i < packet->data_len; i++) {
        file_size = file_size << 8;
        file_size |= packet->data[i];
      }
      printf("file_size: %lu\n", *((unsigned long *)packet->data)); 
    }
    else {
      
      packet->data[packet->data_len] = '\0';
      printf("asString: %s\n", packet->data);
    }
    Serial.println();
    Serial.println("}");
  #endif
}

void print_raw_packet(RawPacket *packet) {
  #ifdef DEBUG
    Serial.println(F("RawPacket: {"));
    Serial.print(F("\tseq_num:\t\t"));
    Serial.println(packet->seq_num);
    Serial.print(F("\tack_num:\t\t"));
    Serial.println(packet->ack_num);
    Serial.print(F("\tdata_len:\t\t"));
    Serial.println(packet->data_len);
    Serial.print(F("data: "));
    for (uint8_t i = 0; i < packet->data_len; i++) {
      Serial.print(packet->data[i]);
      Serial.print(F(" "));
    }
    Serial.println("data:");
    packet->data[packet->data_len] = '\0';
    printf("asString: %s\n", packet->data);
    Serial.println();
    Serial.println("}");
  #endif
}

#endif
