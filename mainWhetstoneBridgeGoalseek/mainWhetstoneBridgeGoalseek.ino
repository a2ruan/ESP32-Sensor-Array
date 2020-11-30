void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  Serial.println(goalseek(3.3,1.2,1000,2000,4000));
}


double goalseek(double Vs, double Vb, double gR1, double gR2, double gR3) {
    double lowerLimit = 0;
    double upperLimit = 2000;
    double iterations = 10000;
    double delta = (upperLimit - lowerLimit)/iterations;

    double Rx = 0;
    double y = 10000;
    double tempY;
    double tempRx;
    double T1;
    double T2;
    for (int i = 0; i<iterations;i++){
       tempRx = lowerLimit + delta*i;
       T1 = gR3 * gR2;
       T2 = gR1 + gR3;
       tempY = Vs*(T1-gR1*tempRx)/((T2)*(gR2+tempRx))-Vb;
       if (abs(tempY) < abs(y)) {
        y = tempY;
        Rx = tempRx;
       }
    }
    return Rx;
}
