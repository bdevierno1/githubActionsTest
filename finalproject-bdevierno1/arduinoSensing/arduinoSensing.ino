
#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate Sensing math.

// Possible Variables
const int led_reading = 13;     // led that blinks 1/second when on "resume"
int last_reading = 0;           // previous value
int user_val = -1;              // number sent by user from host used in command "blink"
int last_flash = 0;             // last time we performed an action (to measure time)
int ledState = LOW;            // to alternate blink on/off
int ms_per_beat = 1000;         // defines frequency for counting when to do events
char val = '0';                 // input character from user (action code)
char buf[8];                    // for reading more than 1 character from serial
float pi = 3.141592;
float samples[100];            // for storing the fake sensor readings
int next_reading = 0;           // for indexing the fake sensor readings
unsigned long previousMillis = 0;        // will store last time LED was updated
double myReading = 0;
boolean pause = false;
int num = 0;

void setup() {
  Serial.begin(9600);           // For Serial Monitor
  pinMode(led_reading, OUTPUT);

  // Defining fake sensor readings (in range 0 -> 1)
  for (int i = 0; i < 100; i++) 
  {
    samples[i] = 0.5 + 0.5 *sin(2*pi*i/100 +  0.25*sin(16*pi*i/100));
  } 
}

// respond to any commands from the serial port
void process_command() {
  int i = 0;
  int msg_length;
  // this function will be called every loop, so it's fine if we exit if there's no data
  if (!Serial.available()) return;
  //read the value returned by the 
  val = Serial.read();
  val = int(val) - 48;
  // based on the command, do something!
  // the comm program will send over an unsigned byte.
  // 1: pause
  //        - enter a loop which will only respond to resume 
  // 2: resume 
  //        - exit pause loop (if it is in it) 
  //        - reset user_val so fake data is read again
  // 3: blink [val]
  //        - After '3', receives a length and a string of bytes representing an integer
  if (val == 1){  // PAUSE
      //when paused keep light on
    pause = true;
  }
  else if (val == 2){ // RESUME
    //reset frequency to default
    pause = false;
    myReading =  samples[next_reading];
    next_reading += 1;
    Serial.println(myReading);     
    if (next_reading == 100){
        next_reading == 0;
    }
  }
  else if (val == 3){  // BLINK
  //update the frequency at which the led blinks.
    Serial.println("hello");
    while (!Serial.available());
    num = Serial.read();
    num = int(num) - 48;
    Serial.println(num);
    ms_per_beat = 1000;
    ms_per_beat = ms_per_beat/val;
  }
}

void loop() {
  //turn the led on and off every second.
  unsigned long currentMillis = millis();
  if (pause == true){
    digitalWrite(13, HIGH);
  } 
  else if (currentMillis - previousMillis >= ms_per_beat && pause == false) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(13,ledState);
  }   
  // Now process any incoming command from Serial
  process_command();
}
