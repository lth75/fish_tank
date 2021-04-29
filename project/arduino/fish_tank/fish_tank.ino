#include <SoftwareSerial.h>
#include <Stepper.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN            6   

int delayval = 100;
// change this to the number of steps on your motor
#define STEPS 512

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS, 2, 3, 4, 5);
SoftwareSerial mySerial(6, 7); // RX, TX

int state = 0;
int last_but=HIGH;

unsigned char gBuf[128];
unsigned char gCmd[16];
int gBufHead=0;
int gBufTail=0;
struct tag_board_state {
  unsigned char relay_mask;
  short temperature;
  short wetness;
  bool is_connected;
} g_state;


void setup() {
  // set the speed of the motor to 30 RPMs
  stepper.setSpeed(30);
  Serial.begin(9600);
  mySerial.begin(9600);
  g_state.relay_mask=0;
  g_state.temperature=200;
  g_state.wetness=500;
}

bool checkCmd(unsigned char *buf) {
  unsigned char b;
  b=buf[0];
  for(int i=1;i<16;i++) {
    b^=buf[i];
  }
  return (b==0);
}

void fill_cmd_check(unsigned char *buf) {
  unsigned char t;
  t=buf[0];
  for(i=1;i<15;i++) {
    t^=buf[i];
  }
  buf[15]=t;
}

void write_cmd(unsigned char *buf) {
  Serial.write(buf,16);
}
void int_to_byte(short data, unsigned char *buf) {
  buf[0]=data&0xff;
  buf[1]=(buf>>8)&0xff;
}
void processCmd(unsigned char *buf, int len) {
  unsigned char relayIdx,relayState;
  switch(buf[1]) {
  case 1:
    // set relay
    relayIdx=buf[2];
    relayState=buf[3];
    if(relayIdx>=6) {
      break;
    }
    if(relayState<1) {
      digitalWrite(8+relayIdx,LOW);
      g_state.relayMask|=(1<<relayIdx);
    }
    else {
      digitalWrite(8+relayIdx,HIGH);
      g_state.relayMask&=~(1<<relayIdx);
    }
    break;
  case 2:
    // send state
    buf[0]=UART_HEADER;
    buf[1]=2;
    int_to_byte(g_state.temperature,buf+2);
    int_to_byte(g_state.wetness,buf+4);
    fill_cmd_check(buf);
    write_cmd(buf);
    break;
  case 3:
    // write soft serial
    memset(buf,0,6);
    buf[0]=0xA0;
    buf[5]=0xA0;
    mySerial.write(buf,6);
    // read the response
    mySerial.read(buf,6);
    if(buf[0]==0xaa) {
      g_state.wetness=(buf[1]*256+buf[2])/10;
    }
    mySerial.read(buf,6);
    if(buf[0]=0xab) {
      g_state.temperature=(buf[1]*256+buf[2])/10;
    }
    // feed back to rpi
    buf[0]=UDP_HEADER;
    buf[1]=3;
    int_to_byte(g_state.temperature,buf+2);
    int_to_byte(g_state.wetness,buf+4);
    fill_cmd_check(buf);
    write_cmd(buf);
    break;
  case 4:
    stepper.step(2048);
    break;
  default:
    ;
  }
}
void loop() {
    while(Serial.available() > 0) {
      gBuf[gBufHead]=(unsigned char)Serial.read();
      gBufHead=(gBufHead+1)&127;
    }
    while( (gBufTail-gBufHead)&0x127 >=16) {
      if(gBuf[gBufTail]!=UART_HEADER) {
        gBufTail=(gBufTail+1)&0x127;
        continue;
      }
      for(int i=0;i<16;i++) {
        gCmd[i]=gBuf[(gBufTail+i)&127];
      }
      if(checkCmd(gCmd)==false) {
        gBufTail=(gBufTail+1)&0x127;
        continue;
      }
      // ready to process 
      processCmd(gCmd,16);
      gBufTail=(gBufTail+16)&127;
    }
}


