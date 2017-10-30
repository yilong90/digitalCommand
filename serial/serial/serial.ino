
// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  Serial.print("abc defg");
  Serial.println();
  Serial.println(12345);
  delay(500);        // delay in between reads for stability
}
