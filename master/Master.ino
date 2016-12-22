#include <Wire.h>
#include <../shared/Messages.ino>
#include <../shared/Addresses.ino>

byte byteRead;

//	response character

char message = MESSAGE__ERROR;

void setup() {
	Wire.begin();
	Serial.begin( 9600 );
}

void loop() {

	//	serial handling

	if ( Serial.available() ) {
    byteRead = Serial.read();

    char readChar = toChar( byteRead );

    if ( readChar == MESSAGE__SYNCH ) {
    	synchronize();
    } else {
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

	delay( 100 );

	Wire.requestFrom( SLAVE_ADDRESS, 1 );

	while ( !Wire.available() ) {
		//	wait in the loop until we get a response
		delay( 5 );
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