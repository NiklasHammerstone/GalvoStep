#include <AccelStepper.h>
#include <MultiStepper.h>



#define D 100 //orthogonal distance of "last" mirror and projection plane
#define E 20  //orthogonal distance of X and Y rotational axes
#define dir_x 3
#define dir_y 6
#define step_x 4
#define step_y 7
#define LASER 11  //teensy: 20
#define micro_step 64
#define INPUT_SIZE 20  //Maximum length of expected Gcode Commands
#define jogSpeed 800000  //speed in steps per second
#define workSpeed 2500    //speed in steps per second
#define endswitchX 9
#define endswitchY 10
#define homePosX 1750  //Tweak this to get a perfect 45 deg angle as 0 position
#define homePosY 1500  //Tweak this to get a perfect 45 deg angle as 0 position
#define INTERPOLATION 20    //Radius moves will be approximated by n=INTERPOLATION linear submoves


  int steps_per_rot = 200*micro_step;
  double degrees_per_step = 360.00 / steps_per_rot;

  struct XY{
    double x;
    double y;
  };

  AccelStepper Xaxis(1, step_x, dir_x); 
  AccelStepper Yaxis(1, step_y, dir_y);
  MultiStepper steppers;

  const byte numChars = INPUT_SIZE;
  char receivedChars[numChars]; // an array to store the received data, max length 20
  boolean newData = false;

  struct XY currPos;
  
  struct XY saved;
  
void setup() {

  Xaxis.setAcceleration(jogSpeed);   //Acceleration must be very high
  Xaxis.setMaxSpeed(jogSpeed);  
  Yaxis.setAcceleration(jogSpeed);   //Acceleration must be very high
  Yaxis.setMaxSpeed(jogSpeed);  
  steppers.addStepper(Xaxis);
  steppers.addStepper(Yaxis);

  pinMode(endswitchX, INPUT);
  pinMode(endswitchY, INPUT);
  
  Serial.begin(115200);
  
  pinMode(LASER, OUTPUT);
  pinMode(13, OUTPUT);    //Indicates that the Teensy is working
  digitalWrite(13, HIGH);

  Serial.println("<< GALVO_STEP V0 READY >> ");
}

void loop() {

  recvWithEndMarker();
  if (newData == true){
    readGCode();
    newData = false;}

}

void move_To(double x, double y){
  
  struct XY angle;
  angle.y = (atan(y/D) / 2 ) * 57.2957795131 ;
  angle.x = (atan( x / (E + sqrt(pow(D, 2) + pow(y, 2)) ) ) / 2 ) * 57.2957795131 ;
  long step_pos[2];
  step_pos[0] = (int)(angle.x / degrees_per_step);
  step_pos[1] = (int)(angle.y / degrees_per_step);
  steppers.moveTo(step_pos);
  steppers.runSpeedToPosition();

}

void readGCode(){

  struct XY pos;
  char CommandTypes[]= {'G', 'X', 'Y', 'M', 'I', 'J', 'F'};
  
  double Commands[sizeof(CommandTypes)]={0};
  int CommandIndeces[sizeof(CommandTypes)]={0};

  for (int j=0;j<sizeof(CommandTypes);j++){
    
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
      pos.x = Commands[1];
  }  else{pos.x = currPos.x;}
  
  if (CommandIndeces[2]!=-1){
      pos.y = Commands[2];
  }else{pos.y = currPos.y;}
  
  if(CommandIndeces[0]!=-1){
    switch((int) Commands[0]){
    case 0:
      if (CommandIndeces[6]!=-1){
         int linSpeed = (int) atan(Commands[6]/D)/degrees_per_step;
         Xaxis.setSpeed(linSpeed);
         Yaxis.setSpeed(linSpeed);
      }else{
         Xaxis.setSpeed(jogSpeed);
         Yaxis.setSpeed(jogSpeed);
        }
      move_To(pos.x, pos.y);
      Serial.println("OK");
      break;
    case 1:
      if (CommandIndeces[6]!=-1){
         int linSpeed = (int) atan(Commands[6]/D)/degrees_per_step;
         Xaxis.setSpeed(linSpeed);
         Yaxis.setSpeed(linSpeed);
      }else{
         Xaxis.setSpeed(jogSpeed);
         Yaxis.setSpeed(jogSpeed);}
      move_To(pos.x, pos.y);
      Serial.println("OK");
      break;
    case 2:
      if (CommandIndeces[4]==-1){
        Commands[4]=0;}
      if (CommandIndeces[5]==-1){
        Commands[5]=0;}
      moveArc(true, Commands[1], Commands[2], Commands[4], Commands[5]);
      Serial.println("OK");
      break;
    case 3:
      if (CommandIndeces[4]==-1){
        Commands[4]=0;}
      if (CommandIndeces[5]==-1){
        Commands[5]=0;}
      moveArc(false, Commands[1], Commands[2], Commands[4], Commands[5]);
      Serial.println("OK");
      break;
    case 28:
      homing();
      Serial.println("Mirrors homed successfully.");
      break;
    case 60:
      saved.x = currPos.x;
      saved.y = currPos.y;
      Serial.println("Saved the current position");
      break;
    case 61:
      pos.x = saved.x;
      pos.y = saved.y;
      move_To(pos.x, pos.y);
      Serial.print("Remembered X:"); Serial.print(pos.x); Serial.print(", Y: "); Serial.println(pos.y);
      break;
    default:
      Serial.println("Command unknown. I'll skip this one.");
      break;
  }
  }else{
    switch((int) Commands[3]){
      case 3:
        digitalWrite(LASER, HIGH);
        Serial.println("LASER ON");
        break;
      case 4:
        digitalWrite(LASER, HIGH);
        Serial.println("LASER ON");
        break;
      case 5:
        digitalWrite(LASER, LOW);
        Serial.println("LASER OFF");
        break;
      case 201: //Usage: M201 X[Accel] Y[Accel]
        int maxAccelX = (int) atan(Commands[1]/D)/degrees_per_step;
        int maxAccelY = (int) atan(Commands[2]/D)/degrees_per_step;
        Xaxis.setAcceleration(maxAccelX);
        Yaxis.setAcceleration(maxAccelY);           
        Serial.println("Max Acceleration has been set.");
      case 203: //Usage: M203 X[maxSpeed] Y[maxSpeed]
        int maxSpeedX = (int) atan(Commands[1]/D)/degrees_per_step;
        int maxSpeedY = (int) atan(Commands[2]/D)/degrees_per_step;
        Xaxis.setMaxSpeed(maxAccelX);
        Yaxis.setMaxSpeed(maxAccelY);           
        Serial.println("Max Speed has been set.");
      break;
    default:
      Serial.println("Command unknown. I'll skip this one.");
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
  int j = 0; 
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
  Xaxis.moveTo(-steps_per_rot);

  while(digitalRead(endswitchX)==LOW){
    Xaxis.run();
  }

  Xaxis.setCurrentPosition(0);
  Xaxis.moveTo(homePosX);

  while(Xaxis.distanceToGo() != 0){
    Xaxis.run();
  }
  Xaxis.setCurrentPosition(0);

 Yaxis.moveTo(-steps_per_rot);

  while(digitalRead(endswitchY)==LOW){
    Yaxis.run();
  }

  Yaxis.setCurrentPosition(0);
  Yaxis.moveTo(homePosY);

  while(Yaxis.distanceToGo() != 0){
    Yaxis.run();
  }
  Yaxis.setCurrentPosition(0);
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
