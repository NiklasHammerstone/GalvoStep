#include <AccelStepper.h>
#include <MultiStepper.h>

//~~~~~~~~~~ Firmware Settings ~~~~~~~~~~~~

#define D 170 //orthogonal distance of "last" mirror and projection plane
#define E 19  //orthogonal distance of X and Y rotational axes
#define dir_x 3
#define dir_y 4
#define step_x 6
#define step_y 7
#define LASER 14
#define micro_step 64
#define INPUT_SIZE 20  //Maximum length of expected Gcode Commands
#define endswitchX 18
#define endswitchY 20
#define homePosX 1470  //Tweak this to get a perfect 45 deg angle as 0 position
#define homePosY 1350  //Tweak this to get a perfect 45 deg angle as 0 position
#define homeSpeed 1500
#define INTERPOLATION 20    //Radius moves will be approximated by n=INTERPOLATION linear submoves
  
int jogSpeed = 500;   //Default value for G0 jog Speed (step per second)
int workSpeed = 100;  //Default value for G1 work Speed (step per second)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0C) // Application Interrupt and Reset Control location

  int steps_per_rot = 200*micro_step;
  double degrees_per_step = 360.00 / steps_per_rot;
  struct XY{
    double x;
    double y;
  };

  AccelStepper Xaxis(1, step_x, dir_x); 
  AccelStepper Yaxis(1, step_y, dir_y);

  const byte numChars = INPUT_SIZE;
  char receivedChars[numChars]; // an array to store the received data, max length 20
  boolean newData = false;
  byte workMode = 0;

  struct XY currPos;
  
  struct XY saved;
  
void setup() {

  pinMode(endswitchX, INPUT);
  pinMode(endswitchY, INPUT);
  
  Serial.begin(115200);
  
  pinMode(LASER, OUTPUT);
  pinMode(13, OUTPUT);    //Indicates that the Teensy is working
  digitalWrite(13, HIGH);

  Serial.println("<< GALVO_STEP V1 READY >> ");
}

void loop() {

  recvWithEndMarker();
  if (newData == true){
    readGCode();
    newData = false;}
  if (digitalRead(endswitchY)==HIGH){
    Serial.println("Y has been triggered");
    delay(200);
  }
  if (digitalRead(endswitchX)==HIGH){
    Serial.println("X has been triggered");
    delay(200);
  }

}

void move_To(double x, double y){
  
  struct XY angle;
  angle.y = (atan(y/D) / 2 ) * 57.2957795131 ;
  angle.x = (atan( x / (E + sqrt(pow(D, 2) + pow(y, 2)) ) ) / 2 ) * 57.2957795131 ;
  long step_pos[2];
  step_pos[0] = (long)(angle.x / degrees_per_step);
  step_pos[1] = (long)(angle.y / degrees_per_step);
  MultiStepper steppers;  
  if (workMode == 0){
    Xaxis.setMaxSpeed(jogSpeed);
    Yaxis.setMaxSpeed(jogSpeed);
  }else{
    Xaxis.setMaxSpeed(workSpeed);
    Yaxis.setMaxSpeed(workSpeed);
  }
  steppers.addStepper(Xaxis);
  steppers.addStepper(Yaxis);
  steppers.moveTo(step_pos);
  steppers.runSpeedToPosition();
}

void readGCode(){

  struct XY pos;
  char CommandTypes[]= {'G', 'X', 'Y', 'M', 'I', 'J', 'F'};
  
  double Commands[sizeof(CommandTypes)]={0};
  int CommandIndeces[sizeof(CommandTypes)]={0};

  for (unsigned int j=0;j<sizeof(CommandTypes);j++){
    
    CommandIndeces[j] = indexof(receivedChars, CommandTypes[j]);
    if (CommandIndeces[j] != -1){
      int endIndex = CommandIndeces[j];
      while(receivedChars[endIndex] != ' ' && receivedChars[endIndex] != 0){
        endIndex++;
      }
      endIndex--;
      int dLen =  endIndex - CommandIndeces[j];
      char cmd[dLen+1];
      cmd[dLen]='\0';
      int h = 0;
      for (int k=CommandIndeces[j]+1;k<=endIndex;k++){
        cmd[h] = receivedChars[k];
        h++;
      }
      Commands[j]=atof(cmd);}
  }
  

  if (CommandIndeces[1]!=-1){
      pos.x =  Commands[1];
  }  else{pos.x = currPos.x;}
  
  if (CommandIndeces[2]!=-1){
      pos.y = Commands[2];
  }else{pos.y = currPos.y;}
  
  if(CommandIndeces[0]!=-1){
    switch((int) Commands[0]){
    case 0:
      if (CommandIndeces[6]!=-1){
         jogSpeed = (int) Commands[6];
      } 
      workMode = 0;
      move_To(pos.x, pos.y);
      Serial.println("Moved laser");
      Serial.println("OK");
      break;
    case 1:
      if (CommandIndeces[6]!=-1){
         workSpeed = (int) Commands[6];
      }
      workMode = 1;
      delay(50);
      move_To(pos.x, pos.y);
      delay(10);
      Serial.println("Moved laser");
      Serial.println("OK");
      break;
    case 2:
      if (CommandIndeces[4]==-1){
        Commands[4]=0;}
      if (CommandIndeces[5]==-1){
        Commands[5]=0;}
      moveArc(true, Commands[1], Commands[2], Commands[4], Commands[5]);
      Serial.println("Moved laser");
      Serial.println("OK");
      break;
    case 3:
      if (CommandIndeces[4]==-1){
        Commands[4]=0;}
      if (CommandIndeces[5]==-1){
        Commands[5]=0;}
      moveArc(false, Commands[1], Commands[2], Commands[4], Commands[5]);
      Serial.println("Moved laser");
      Serial.println("OK");
      break;
    case 28:
      homing();
      Serial.println("Mirrors homed successfully.");
      Serial.println("OK");
      break;
    case 60:
      saved.x = currPos.x;
      saved.y = currPos.y;
      Serial.println("Saved the current position");
      Serial.println("OK");
      break;
    case 61:
      pos.x = saved.x;
      pos.y = saved.y;
      move_To(pos.x, pos.y);
      Serial.print("Remembered X:"); Serial.print(pos.x); Serial.print(", Y: "); Serial.println(pos.y);
      Serial.println("OK");
      break;
    case 99:
      Serial.println("RESETTING");
      Serial.end();  //clears the serial monitor  if used
      SCB_AIRCR = 0x05FA0004;  //write value for restart
      break;
    default:
      Serial.println("Command unknown. I'll skip this one.");
      Serial.println("OK");
      break;
  }
  }else{
    switch((int) Commands[3]){
      case 3:
        digitalWrite(LASER, HIGH);
        Serial.println("LASER ON");
        Serial.println("OK");
        break;
      case 4:
        digitalWrite(LASER, HIGH);
        Serial.println("LASER ON");
        Serial.println("OK");
        break;
      case 5:
        digitalWrite(LASER, LOW);
        Serial.println("LASER OFF");
        Serial.println("OK");
        break;
      case 6:
        Serial.println("Firing the Laser for 100ms");
        digitalWrite(LASER, HIGH);
        delay(100);
        digitalWrite(LASER, LOW);
        Serial.println("OK");
        break;
      case 201: //Usage: M201 X[Accel] Y[Accel]      
        Serial.println("Acceleration is deprecated.");
        Serial.println("OK");
      case 203: //Usage: M203 X[maxSpeed] Y[maxSpeed]        
        Serial.println("Max Speed is deprecated.");
        Serial.println("OK");
      break;
    default:
      Serial.println("Command unknown. I'll skip this one.");
      Serial.println("OK");
      break;        
    }
  }
  
  currPos.x = pos.x;
  currPos.y = pos.y;
}

void recvWithEndMarker() {
  //Receives the Command from Serial 
 static byte ndx = 0;
 char endMarker = '\n';
 char rc;

 while (Serial.available() > 0 && newData == false) {
   rc = Serial.read();
   if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
   if (ndx >= numChars) {
      ndx = numChars - 1;}
   }
   else {
     receivedChars[ndx] = '\0'; // terminate the string
     ndx = 0;
     newData = true;}
   }
  
}

int indexof(char str[], char c){
  //Returns the index of a char in a chararray. 
  byte j = 0; 
  boolean found = false;
  while(j<numChars){
        if (str[j]!=c){j++; }
        else{found = true; break;}
    }
  if(found){
    return j;}
  else{return -1;}
}

void homing(){

  int prevXmaxSpeed = Xaxis.maxSpeed();
  int prevYmaxSpeed = Yaxis.maxSpeed();
  int prevXspeed = Xaxis.speed();
  int prevYspeed = Yaxis.speed();
  Xaxis.setMaxSpeed(homeSpeed);
  Yaxis.setMaxSpeed(homeSpeed);
  
  Xaxis.setSpeed(-homeSpeed);

  while(digitalRead(endswitchX)==LOW){
    Xaxis.runSpeed();
  }

  Xaxis.setCurrentPosition(0);
  Xaxis.moveTo(homePosX);
  Xaxis.setSpeed(homeSpeed);

  while(Xaxis.distanceToGo() != 0){
    Xaxis.runSpeed();}
  Xaxis.setCurrentPosition(0);

 Yaxis.setSpeed(-homeSpeed);

  while(digitalRead(endswitchY)==LOW){
    Yaxis.runSpeed();
  }

  Yaxis.setCurrentPosition(0);
  Yaxis.moveTo(homePosY);
  Yaxis.setSpeed(homeSpeed);
  while(Yaxis.distanceToGo() != 0){
    Yaxis.runSpeed();}
  Yaxis.setCurrentPosition(0);
  
  Xaxis.setMaxSpeed(prevXmaxSpeed);
  Yaxis.setMaxSpeed(prevYmaxSpeed);
  Xaxis.setSpeed(prevXspeed);
  Yaxis.setSpeed(prevYspeed);
  currPos.x = 0.0;
  currPos.y = 0.0;
}

void moveArc(boolean CW, double X, double Y, double I, double J){
  struct XY center;
  double startAngle;
  double endAngle;
  struct XY startPos = currPos;
  struct XY endPos; endPos.x=X; endPos.y=Y;
  struct XY points[INTERPOLATION];
  double radius = sqrt(pow(I,2)+pow(J,2));
  points[0]=startPos;
  points[INTERPOLATION-1]=endPos;
  
  center.x = startPos.x + I;
  center.y = startPos.y + J;

  startAngle = getAngle(startPos.x-center.x, startPos.y-center.y);
  endAngle = getAngle(endPos.x-center.x, endPos.y-center.y);
  if (CW){
      if(startAngle <= endAngle){
          startAngle += 6.28318530718;
      }
  }else{
    if (startAngle >= endAngle){
      endAngle += 6.28318530718;
    }
  }

  double increment = ((endAngle-startAngle)/(INTERPOLATION-1));
//  Serial.print("Start angle: "); Serial.println(startAngle);
//  Serial.print("End angle: "); Serial.println(endAngle);
//  Serial.print("Angle increment: "); Serial.println(increment);
  double currAngle = startAngle;
  struct XY currPoint;
  for(int i = 1; i<INTERPOLATION-1; i++){
    currAngle += increment;
    currPoint.x = radius*cos(currAngle)+center.x;
    currPoint.y = radius*sin(currAngle)+center.y;
    points[i] = currPoint;
  }
  for(int i=1; i<INTERPOLATION; i++){
    move_To(points[i].x, points[i].y);
    //Serial.print(points[i].x); Serial.print("  "); Serial.println(points[i].y);
  }
}

double getAngle(double x, double y){
  //Retrieves the polar angle phi from a point in cartesian coordinates.
  double phi = atan(y/x);
  if(x >= 0 && y >= 0){
    return phi;
  }else if(x > 0 && y < 0){
    return phi + 6.28318530718;
  }else{
    return phi + 3.14159265359;
  }
}
