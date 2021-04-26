#include <Stepper.h>


#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN            6   
#define NUMPIXELS      60
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int i=0;
int direction1=1;
int delayval = 100;
// change this to the number of steps on your motor
#define STEPS 512

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS, 2, 3, 4, 5);


int state = 0;
int last_but=HIGH;

int r_val=10;
int g_val=0;
int b_val=0;
int light_state=0;

void setup() {
  // set the speed of the motor to 30 RPMs
  stepper.setSpeed(30);
  pinMode(8, INPUT_PULLUP);
  pixels.begin();
}

void loop() {

    if(direction1>0)
      LED(i,r_val,g_val,b_val);
   else
      LED(i,0,0,0);
   show();
   i+=direction1;
   if(i>=NUMPIXELS || i<0 )
   {
      direction1=-direction1;
      i+=direction1;
   }
  // move a number of steps equal to the change in the
  // sensor reading
  int val;
   val=digitalRead(8);
   if(last_but==LOW && val==HIGH)
   {
      stepper.step(2048);
      state=0;
   }
   last_but=val;
   light_state++;
   switch(light_state%3)
   {
    case 0:
      r_val=10;
      g_val=10;
      b_val=0;
      break;
    case 1:
      r_val=10;
      g_val=0;
      b_val=10;
      break;
    case 2:
      r_val=0;
      g_val=10;
      b_val=10;
      break;
   }
   delay(50);
}

void LED(int num,int R,int G,int B)
{
   pixels.setPixelColor(num, pixels.Color(R,G,B));  
}
void show()
{
  pixels.show();
}
