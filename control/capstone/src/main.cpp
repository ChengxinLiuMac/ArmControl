#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <stdlib.h>


void move_motor_XY(double radius, double theta, int speed);
void move_motor_Z(double height, int speed);
void motor_movement_distance(double distance, String dir, int speed);
void motor_movement_degree(double degree, String dir, int speed);
int Distance_stepConverter(double distance, String axis);
int Degree_stepConverter(double degree);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void watering(String Water);
void XY_init();
void z_init();
void r_init();
void r_init_reverse();

int totalsteps = 0;
//pins for controlling radian movement
const int dirPin_XY = 25;
const int stepPin_XY = 33;
// const int dirPin_XY = 19;
// const int stepPin_XY = 18;
//pin for controlling vertical movement
const int dirPin_Z = 23;
const int stepPin_Z = 22;

const int dirPin_Base = 19;
const int stepPin_Base = 18;
int stepsPerRevolution = 200;
const int limit_switch_Z = 21;
const int limit_switch_XY = 36;
const int limit_switch_base = 5;

const int watering_signal = 27;
// Connect to the WiFi
const char* ssid = "COGECO-DE490";
const char* password =  "38956COGECO";
const char* mqttServer = "192.168.0.164";
const int mqttPort = 1883;

String z = "";
String a = "";
String th = "";
double z_len = 0;
double a_len = 0;
double th_len = 0;
int counter= 0;



const double Single_Step_Degree = 1.8;
double current_pos_xy = 0;
double current_pos_z = 0;
double current_deg_r = 0;

WiFiClient espClient;
PubSubClient client(espClient);
//RGB

//

// the setup function runs once when you press reset or power the board
void setup() {
  
  pinMode(dirPin_XY, OUTPUT);
  pinMode(stepPin_XY, OUTPUT);
  pinMode(dirPin_Z, OUTPUT);
  pinMode(stepPin_Z, OUTPUT);
  pinMode(dirPin_Base, OUTPUT);
  pinMode(stepPin_Base, OUTPUT);
  pinMode(watering_signal, OUTPUT);

  pinMode(limit_switch_Z, INPUT_PULLDOWN);
  pinMode(limit_switch_XY, INPUT_PULLDOWN);
  pinMode(limit_switch_base, INPUT_PULLDOWN);

  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);
  
  // r_init();
  // XY_init();
  // z_init(); 
  WiFi.begin(ssid, password);
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
    ESP.restart();
    WiFi.begin(ssid, password);
    delay(3000);
  }
  
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("")) {
      Serial.println("connected");
    } 

    else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }

  client.subscribe("test/request");

  // From top to bottom:
  digitalWrite(watering_signal, LOW);
  r_init();
  XY_init();
  z_init(); 
  move_motor_Z(40.0, 1000);
}

// the loop function runs over and over again forever
void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {

  z = "";
  a = "";
  th = "";
  counter = 0;
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    if (counter == 0 && (char)payload[i]!= ','){
      z+=(char)payload[i];
    }
    else if (counter == 1 && (char)payload[i]!= ','){
      a+=(char)payload[i];
    }
    else if (counter == 2 && (char)payload[i]!= ','){
      th+=(char)payload[i];
    }
    else if ((char)payload[i]== ','){
      counter ++;
    }

  }
  

  z_len = z.toDouble();
  if(z != "-1"){
    a_len = a.toDouble();
    th_len = th.toDouble();
    Serial.println(z_len);
    Serial.println(a_len);
    Serial.println(th_len);
  }
  else if(z == "-1"){
    a_len = atoi(a.c_str());
    digitalWrite(watering_signal, HIGH);
    delay(a_len*1000);
    digitalWrite(watering_signal, LOW);
  }


  //char *ptr = strtok(str, delim);
  //char *ptr = strtok(str, delim);
  move_motor_XY(a_len, th_len, 1000);
  move_motor_Z(z_len, 1000);


  client.publish("test/response", "fin");
  Serial.println();
  Serial.println("-----------------------");
 
}

void watering(String Water){
  if(Water == "W"){
    digitalWrite(watering_signal,HIGH);
  }
  else{
    digitalWrite(watering_signal,LOW);
  }    
}
// dir = HIGH, Move towards center/down (Counter-clockwise), LOW towards outside/up (Clockwise).

void r_init(){
  current_deg_r = 0;
  int r_flag = digitalRead(limit_switch_base);
  digitalWrite(dirPin_Base, HIGH);
  while(r_flag == 1){
    r_flag = digitalRead(limit_switch_base);
    digitalWrite(stepPin_Base, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin_Base, LOW);
    delayMicroseconds(1000);
  }
}

void r_init_reverse(){
  current_deg_r = 0;
  int r_flag = digitalRead(limit_switch_base);
  digitalWrite(dirPin_Base, LOW);
  while(r_flag == 1){
    r_flag = digitalRead(limit_switch_base);
    digitalWrite(stepPin_Base, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin_Base, LOW);
    delayMicroseconds(1000);
  }
}

// dir = HIGH, Move towards center/down (Counter-clockwise), LOW towards outside/up (Clockwise).
void XY_init(){
  current_pos_xy = 0;
  int XY_flag = digitalRead(limit_switch_XY);
  digitalWrite(dirPin_XY, HIGH);
  while(XY_flag == 1){
    XY_flag = digitalRead(limit_switch_XY);
    digitalWrite(stepPin_XY, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin_XY, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(dirPin_XY, LOW); 
  motor_movement_distance(1.0,"o", 1000);
  current_pos_xy = 1.0;
}

// dir = HIGH, Move towards center/down (Counter-clockwise), LOW towards outside/up (Clockwise).
void z_init(){
  current_pos_z = 0;
  int z_flag = digitalRead(limit_switch_Z);
  digitalWrite(dirPin_Z, HIGH);
  while(z_flag == 1){
    z_flag = digitalRead(limit_switch_Z);
    digitalWrite(stepPin_Z, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin_Z, LOW);
    delayMicroseconds(1000);
  }
  delay(700);
  z_flag = digitalRead(limit_switch_Z);
  if(z_flag == 0){
    digitalWrite(dirPin_Z, LOW); 
    motor_movement_distance(1.0,"u", 1000);
    current_pos_z = 1.0;
  }
  else{
    while(z_flag == 1){
      z_flag = digitalRead(limit_switch_Z);
      digitalWrite(stepPin_Z, HIGH);
      delayMicroseconds(1000);
      digitalWrite(stepPin_Z, LOW);
      delayMicroseconds(1000);
    }
    digitalWrite(dirPin_Z, LOW); 
    motor_movement_distance(1.0,"u", 1000);
    current_pos_z = 1.0;
  }
}

// dir = HIGH, Move towards center/down (Counter-clockwise), LOW towards outside/up (Clockwise).
void motor_movement_distance(double distance, String dir, int speed){
  int steps = 0;
  String up = "u";
  String down = "d";
  String outward = "o";
  String inward = "i";
  int dirPin = 0;
  int stepPin = 0;
  if(dir == outward || dir == inward){
    dirPin = dirPin_XY;
    stepPin = stepPin_XY;
    steps = Distance_stepConverter(distance, "r");
  }
  else if(dir == up || dir == down){
    dirPin = dirPin_Z;
    stepPin = stepPin_Z;
    steps = Distance_stepConverter(distance, "z");
  }
  stepsPerRevolution = steps;
  if(dir == outward || dir == up){
    digitalWrite(dirPin, LOW);
    for(int x = 0; x < stepsPerRevolution; x++)
    {
        // Moving forward
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(speed);
    }
    /*
    totalsteps += stepsPerRevolution;
    Serial.print("total steps");
    Serial.print(totalsteps);
    Serial.print("\n");
    */
  }
  else if(dir == inward || dir == down){
    digitalWrite(dirPin, HIGH);
    for(int x = 0; x < stepsPerRevolution; x++)
    {
        // Moving forward
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(speed);
    }
    /*
    totalsteps += stepsPerRevolution;
    Serial.print("total steps");
    Serial.print(totalsteps);
    Serial.print("\n");
    */
  }
}

// dir = HIGH, Move towards center/down (Counter-clockwise), LOW towards outside/up (Clockwise).
// This function control motor movement by degree.
void motor_movement_degree(double degree, String dir, int speed){
  int steps = 0;
  String clockwise = "c";
  String Counter_clockwise = "cc";
  steps = 8*Degree_stepConverter(degree);
  if(dir == clockwise){
    digitalWrite(dirPin_Base, LOW);
    for(int x = 0; x < steps; x++)
    {
      if(digitalRead(limit_switch_base) == 0)
        current_deg_r = 0.0;
        // Moving forward
      digitalWrite(stepPin_Base, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin_Base, LOW);
      delayMicroseconds(speed);
    }
  }
  else if(dir == Counter_clockwise){
    digitalWrite(dirPin_Base, HIGH);
    for(int x = 0; x < steps; x++)
    {
        // Moving forward
      digitalWrite(stepPin_Base, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin_Base, LOW);
      delayMicroseconds(speed);
    }
  }
}

// This function convert arm travel distance (cm) into steps for 2 stepmotor. 
int Distance_stepConverter(double distance, String axis){
  int AxisTotalSteps = 0;
  double total_radius_height = 0;
  int steps_to_move = 0;
  if(distance != 0 ){
    if (axis = "z"){
      AxisTotalSteps = 12700;
      total_radius_height = 55.8;
    }
    else if (axis = "xy"){
      AxisTotalSteps = 7800;
      total_radius_height = 32.0;
    }
    steps_to_move = round((distance/total_radius_height)*AxisTotalSteps);
  }
  return steps_to_move;
}

// This function convert degree to steps
int Degree_stepConverter(double degree){
  int steps_to_move = 0;
  if(degree != 0 ){
    steps_to_move = round(4*(degree/Single_Step_Degree));
  }
  return steps_to_move;
}

void move_motor_Z(double height, int speed){
  // double first_distance = 0.0;
  // double second_distance = 0.0;
  String dir = "";
  double distance = 0.0;
  if(height > 55.8 || height < 0);
 else if(height == 0.0)
    z_init();
  else{
    dir  = height >= current_pos_z ? "u" : "d";
    distance  = abs(height - current_pos_z);
    current_pos_z = height;
    motor_movement_distance(distance, dir, speed);
  }
}

void move_motor_XY(double radius, double theta, int speed){
  String dir = "";
  String theta_dir = "";
  double xy_distance = 0.0;
  double angle = 0.0;

  if(theta == 0.0 && current_deg_r <=  180)
    r_init();
  else if(theta == 0.0 && current_deg_r > 180)
    r_init_reverse();
  else if ((theta > 0.0 && theta <= 180.0) &&  current_deg_r < 180.0){
    theta_dir = theta >= current_deg_r ? "c" : "cc";
    angle = abs(theta - current_deg_r);
    motor_movement_degree(angle, theta_dir, speed);
    current_deg_r = theta;
  }
  else if ((theta > 0.0 && theta <= 180.0) &&  current_deg_r > 180.0){
    r_init_reverse();
    // theta_dir = theta >= current_deg_r ? "c" : "cc";
    theta_dir = "c";
    angle =  abs(theta);
    motor_movement_degree(angle, theta_dir, speed);
    current_deg_r = theta;
  }
  else if ((theta >180.0 && theta <= 360.0) &&  current_deg_r > 180.0){
    theta_dir = theta >= current_deg_r ? "c" : "cc";
    angle = abs(theta - current_deg_r);
    motor_movement_degree(angle, theta_dir, speed);
    current_deg_r = theta;
  }
  else if ((theta >180.0 && theta <= 360.0) &&  current_deg_r < 180.0){
    r_init();
    // theta_dir = theta >= current_deg_r ? "cc" : "c";
    theta_dir = "cc";
    angle = abs(360.0 - theta);
    motor_movement_degree(angle, theta_dir, speed);
    current_deg_r = theta;
  }

  if(radius > 32.0 || radius < 0.0)
;  else if(radius == 0.0)
    XY_init();
  else{
    dir  = radius >= current_pos_xy ? "o" : "i";
    xy_distance  = abs  (radius - current_pos_xy);
    current_pos_xy = radius;
    motor_movement_distance(xy_distance, dir, speed);
  }
} 

void reconnect() {
  // Loop until we're reconnected
  if(WiFi.status() != WL_CONNECTED){
     while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
        WiFi.begin(ssid, password);
        delay(1500);
      }
  }
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("")) {
      Serial.println("connected");
    } 

    else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }
  client.subscribe("test/request");
  client.publish("test/response", "failed");
  Serial.println();
  Serial.println("-----------------------");
}