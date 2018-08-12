#include <Wire.h>                                                                     // Include the wire library for I2C communication
#include <LiquidCrystal.h>

#define I2C_ADDRESS 0x28                                                              // PCap04 Address
#define REGISTER_TEST 0x7e                                                            // Test read opcode 0x7e
#define STATUS0 32                                                                    // read register 32 (status register)
#define STATUS1 33                                                                    // read register 33 (status register)
#define STATUS2 34                                                                    // read register 34 (status register)
#define RES0 0
#define RES1 4
#define RES2 8
#define RES3 12                                                                       // begin values of result registers
#define RES4 16
#define RES5 20
#define RES6 24
#define RES7 28

LiquidCrystal lcd(7, 8, 9, 10, 11 , 12); 
int n_setup_values  = 10;
double offset;

void setup() {
  Wire.begin();                                                                       // sets up i2c for operation

  Serial.begin(9600);                                                                 // set up baud rate for serial
//  Serial.println("Initializing");
//
//  testRead();                                                                         // perform a test read
//  
//  readStatus(STATUS0);                                                                // request status
//  

  lcd.begin(16,2);                                                                      // start the LCD
  lcd.setCursor(0,0);                                                                   // set cursor for next write
  lcd.write("Calibrating");                                                             
  
  int count;
  for (count = 0; count<n_setup_values; ++ count) {                                     // calculating offset
    double resultf = readResult(RES1);
    Serial.println(resultf);
    offset = (resultf + offset * count) / (count + 1);
    delay(700);
    
    if (count % 2 == 0) {
      lcd.setCursor(11,0);
      lcd.write(".");
    }
    else {
      lcd.setCursor(11,0);
      lcd.write(" ");
    }
  }
  
  Serial.println(offset);                                                             // print offset to serial bus

  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.write("dC:");
  lcd.setCursor(14,0);
  lcd.write("fF");                                                                    // write static text to LCD
  lcd.setCursor(0,1);
  lcd.write("dP:");
  lcd.setCursor(12,1);
  lcd.write("mbar");

}

unsigned char readStatus(unsigned char r)
{
  Wire.beginTransmission(I2C_ADDRESS);                                                // Begin transmission to the Sensor
  Wire.write(r | _BV(6));                                                             // point to read address converted from register number
  Wire.endTransmission();
  
  Wire.requestFrom(I2C_ADDRESS,1);                                                    // read status byte
  byte r_status = Wire.read();                                                        // request status
  Serial.print("status: ");
  Serial.println(r_status,BIN);                                                       // print status
  
  return r_status;
}

boolean testRead() {
  Wire.beginTransmission(I2C_ADDRESS);                                                // Begin transmission to the Sensor
  Wire.write(REGISTER_TEST);                                                          // request byte with test read opcode
  Wire.endTransmission();
  
  Wire.requestFrom(I2C_ADDRESS, 1);                                                   // request test read byte
  if (Wire.available() <= 2) {
    int test = Wire.read();
    if (test == 17) {                                                                 // DEC(17) is the the configuration of the byte that indicates a succesfull test read
      Serial.println("Test succes"); 
      return true;
    } else {
      Serial.println("Test Failed");
      return false;
    }
  }

}

float readResult(unsigned char RES) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(RES | _BV(6));                                                           // point to read address converted from register number
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, 4);                                                   // request 4 result bytes from 1 result name (Res0, Res1 ....)
  long res_1 = Wire.read();          
  long res_2 = Wire.read();
  long res_3 = Wire.read();
  long res_4 = Wire.read();
  long result_int = (res_4 >> 3);                                                     // 5 most significant bits of the most significant byte give the integer part of the result
  long res_4_frac = res_4-(result_int << 3);                                          // the fractional part of the most signifcant byte are the 3 least significant bits of that byte
  long result_frac_bin = (res_4_frac << 24) + (res_3 << 16) + (res_2 << 8) + (res_1); // after shifting bytes to their position in the 27 bit binary frac result they can be added together
  float result_frac_flt = result_frac_bin / pow(2, 27);                               // to convert the binary 27 frac part into the floating frac part it is divided by the max value of 27 bits +1 (2^27)
  float resultf = (result_int+result_frac_flt)*1000*10;                               // adding the frac part and int part together and multiplying by 1000 for aF->fF 
                                                                                      // also multiplying by 10 because otherwise it is the fraction between C/Cref and not C (Cref=10)
  return resultf;
}


void loop() {

  double resultf = readResult(RES1);                                                  // Request result from RES1
  Serial.println(resultf);                                                            // Print result to Serial
  delay(700);                                                                         // 700 ms conversion time per sample
  double drift = resultf - offset;                                                    // calculate Drift (current result - calculated offset)
  
  lcd.setCursor(4,0);                                                                 // write the drift
  lcd.print(drift,1);
  lcd.print("     ");
  lcd.setCursor(4,1);
  float responsivity = 0.42; // in fF/mbar                                            // using calculated responsitivity (unique for chip)
  lcd.print(drift/responsivity,1);                                                    // print calculated pressure
  lcd.print("   ");
  lcd.setCursor(11,1);
  lcd.write(" mbar");                                                                 // rewrite mbar and fF for the case where they are overwritten by decimal numbers
  lcd.setCursor(14,0);
  lcd.write("fF");

  
  

}
