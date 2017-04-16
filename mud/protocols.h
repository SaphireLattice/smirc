#ifndef SMIRC_PROTOCOLS_H
#define SMIRC_PROTOCOLS_H

enum {
  SE   = (char) 0xF0, // end of subnegotiation
  SB   = (char) 0xFA, // start of subnegotiation
  WILL = (char) 0xFB,
  WONT = (char) 0xFC,
  DO   = (char) 0xFD,
  DONT = (char) 0xFE,
  IAC  = (char) 0xFF,  // Interpret As Command

  //
  ATCP = (char) 0xC8,

  //Generam MUD Client Protocol
  GMCP = (char) 0xC9,   // GMCP sequence (decimal 201)

  //MUD Terminal Type Standard, TTYPE extension
  TTYPE= (char) 0x18,
  IS   = (char) 0x00,
  SEND = (char) 0x01,

  //MUD Server Data Protocol
  MSDP = (char) 0x45,
  MSDP_VAR = (char) 0x01,
  MSDP_VAL = (char) 0x02,
  MSDP_TABLE_OPEN  = (char) 0x03,
  MSDP_TABLE_CLOSE = (char) 0x04,
  MSDP_ARRAY_OPEN  = (char) 0x05,
  MSDP_ARRAY_CLOSE = (char) 0x06,

  //MUD Server Status Protocol
  MSSP = (char) 0x46,
  MSSP_VAR = (char) 0x01,
  MSSP_VAL = (char) 0x02,

  //Mud Sound Protocol
  MSP  = (char) 0x5A,

  //MUD Client Compression Protocol
  COMPRESS2 = (char) 0x56,
  COMPRESS  = (char) 0x55,
};


struct state {
  int fg;
  int bg;
};

char* get_protocols();
char* ttoa(unsigned int code);
char* ansi_to_irc_color(char* str);

#endif //SMIRC_PROTOCOLS_H
