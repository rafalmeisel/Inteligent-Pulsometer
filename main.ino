#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// range of pulse limits
#define UpperThreshold 750
#define LowerThreshold 700

// lcd display phisical adress
#define OLED_Address 0x3C

// reset pin for lcd display
Adafruit_SSD1306 oled(4);

bool ignoreAanalogValue = false;
bool firstBeatsDetected = false;
unsigned long firstBeatseTime = 0;
unsigned long secondBeatsTime = 0;
unsigned long intervalBeetweenHeartBeats = 0;

int analogValue = 0;
int average = 0;
int sumBpmHistory = 0;
int counterSave = 0;


// variables for displaying pulse graph
int x = 0;
int y = 0;
int lastx = 0;
int lasty = 0;

// heart beats per minutes
int BPM = 0;

// BPM history
const int historyLenght = 10;
int bpmHistory[historyLenght] ={0,0,0,0,0,0,0,0,0,0};

unsigned long currentTime;

// counting time between update history
unsigned long updateHistoryTimeDelay = 5000;

// counting dangereus situation 
int alarmCount = 0;

 // phone number with AT command
String commandAT = "ATD";
String phoneNumber = "XXX-XXX-XXX";
String endCommandATChar = ";";
String tmpCommandAT = "";
char finalCommandAT[20];

// declaration of pins for receiver and transimiter on Arduino 
SoftwareSerial GsmModuleSerial(9, 8);

void setup() {
  Serial.begin(9600);
  GsmModuleSerial.begin(9600);
  
  // set phisical adress of lcd display to object
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  oled.clearDisplay();
  oled.writeFillRect(0,20,128,16,BLACK);
  oled.setTextColor(WHITE);

  currentTime = millis(); 
}
 
void loop(){

  // reading value from pulse sensor
  analogValue=analogRead(0);
  
  // Heart beat leading edge detected.
  if(analogValue > UpperThreshold && ignoreAanalogValue == false){
    if(firstBeatsDetected == false){
      firstBeatseTime = millis();
      firstBeatsDetected = true;
    }
    else{
      secondBeatsTime = millis();
      intervalBeetweenHeartBeats = secondBeatsTime - firstBeatseTime;
      firstBeatseTime = secondBeatsTime;
    }
    ignoreAanalogValue = true;
  }

  // Heart beat trailing edge detected.
  if(analogValue < LowerThreshold){
    ignoreAanalogValue = false;
  }  
  
  // computing beats per minute
  BPM = (1.0/intervalBeetweenHeartBeats) * 60.0 * 1000;

  // check if is time to update history 
  if((currentTime + updateHistoryTimeDelay) < millis()){
    
    currentTime = millis();
    
    // compute average
    sumBpmHistory = 0;
    for (int i = 0; i< historyLenght; i++){
      sumBpmHistory = sumBpmHistory + bpmHistory[i];
    }
    average = 0;
    average = sumBpmHistory / historyLenght; 

      // check if current BPM isn't to low
      if(BPM < (average * 0.8)){
        alarmCount = alarmCount + 1;
      }
      else{
        alarmCount = 0;

        // przesuwanie historii - FIFO
        for( int i = historyLenght-1; i<0; i--){
          bpmHistory[i] = bpmHistory[i-1];
        }
        bpmHistory[counterSave] = BPM;
        counterSave = counterSave + 1;

        if(counterSave == historyLenght){
          counterSave = 0;
        }
      }

      // Start calling
      if(alarmCount > 5){
        tmpCommandAT = commandAT + phoneNumber + endCommandATChar;
        tmpCommandAT.toCharArray(finalCommandAT, 20);
        GsmModuleSerial.write(finalCommandAT);  
        GsmModuleSerial.println();
      }
      // Stop calling
      else if(alarmCount>8){
        GsmModuleSerial.write("ATH;");
        GsmModuleSerial.println();
      }
      
  }
  
  // display bpm
  
  // clear display
  if(x>127){
      oled.clearDisplay();
      x=0;
      lastx=0;
    }

  // computing hight rate for display on display
  y = 50 - (analogValue / 20);

  oled.setCursor(0,20);
  oled.writeLine(lastx,lasty,x,y,WHITE);
  
  lasty=y;
  lastx=x;
  
  oled.setCursor(0,0);
  oled.print("BPM: ");
  oled.print(BPM);

  oled.setCursor(0,20);
  oled.print("AVERAGE: ");
  oled.print(average);

  oled.display();
  oled.clearDisplay();
  x++;
}
