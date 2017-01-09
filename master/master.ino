#include <Wire.h>

//  MESSAGES

char MESSAGE__SYNCH = 'S';
char MESSAGE__AWAIT_SYNCH = 'W';
char MESSAGE__ERROR = '1';
char MESSAGE__EYE_START = 'E';
char MESSAGE__EYE_END = 'T';
char MESSAGE__EYE_X = 'X';
char MESSAGE__EYE_Y = 'Y';
char MESSAGES__REWARDS[ 2 ] = { 'A', 'B' };
char MESSAGE__REWARD_DELIVERED = 'R';

//	ADDRESSES

int SLAVE_ADDRESS = 9;

//	READ FROM SERIAL

byte byteRead;

//	response character

char message = MESSAGE__ERROR;

//	BEGIN

void setup() {
  Wire.begin();
  Serial.begin( 115200 );
}

void loop() {

  //	serial handling

  if ( Serial.available() ) {
    byteRead = Serial.read();

    char readChar = toChar( byteRead );

    if ( readChar == MESSAGE__SYNCH ) {
      synchronize();
    } else if ( readChar == MESSAGE__EYE_START ) {
      String inStr = "";
      while ( Serial.available() == 0 ) {
        delay(5);
      }
      while ( Serial.available() > 0 ) {
        int inChar = Serial.read();
        if ( inChar == MESSAGE__EYE_END ) {
//          Serial.println( inStr );
          delay( 5 );
          Wire.beginTransmission( SLAVE_ADDRESS );
          Wire.write( inStr.c_str() );
          Wire.endTransmission();
          break;
        } else {
          inStr += (char)inChar;
        }
      }
    } else if ( readChar == 'T' ) {
      relay( '*' );
    } else {
      //	deliver reward by relaying the message
      //  to the slave
      transmit( readChar );
    }
  }
}

void relay( char c ) {
  Serial.write( c );
}

void transmit( char c ) {
  Wire.beginTransmission( SLAVE_ADDRESS );
  Wire.write( c );
  Wire.endTransmission();
}

char toChar( byte toConvert ) {
  return toConvert & 0xff;
}

void synchronize() {
  transmit( MESSAGE__SYNCH );

  Wire.requestFrom( SLAVE_ADDRESS, 1 );

  while ( !Wire.available() ) {
    //	wait in the loop until we get a response
    delay( 10 );
    relay( MESSAGE__AWAIT_SYNCH );
    continue;
  }

  char response = Wire.read();

  if ( response == MESSAGE__SYNCH ) {
    relay( response );
  } else {
    synchronize();
  }
}

void handleReceipt( int nBytes ) {

  if ( Wire.available() == 0 ) {
    relay( MESSAGE__ERROR );
    return;
  }

  message = Wire.read();

  if ( message == MESSAGE__SYNCH ) {
    relay( message ); return;
  }
}
