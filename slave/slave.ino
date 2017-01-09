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

//	configure pin numbers

const int N_REWARDS = 2;
int REWARD_PINS[ N_REWARDS ] = { 37, 40 };

//	reward size

const int REWARD_SIZE = 100;

void setup() {
	Wire.begin( SLAVE_ADDRESS );
	Serial.begin( 115200 );
	Wire.onRequest( handleRequest );
	Wire.onReceive( handleReceipt );

	for ( int i = 0; i < N_REWARDS; i++ ) {
		pinMode( REWARD_PINS[i], OUTPUT );
  }
}

void loop() {

	//	rewards

	if ( Serial.available() ) {
    byteRead = Serial.read();
    char readChar = toChar( byteRead );
    if ( readChar == MESSAGE__EYE_START ) {
    	relay( readChar );
    }
    //	check to see if this is a reward message
    int index = findIndex( MESSAGES__REWARDS, N_REWARDS, readChar );
    if ( index != -1 ) {
    	deliverReward( index, REWARD_SIZE );
    }
  }

}

void relay( char c ) {
	Serial.write( c );
}

void handleReceipt( int nBytes ) {

	if ( Wire.available() == 0 ) {
		relay( MESSAGE__ERROR ); 
		return;
	}

  if ( nBytes == 1 ) {

  	message = Wire.read();
  
  	if ( message == MESSAGE__SYNCH ) {
  		Wire.write( message );
  		relay( message ); 
  		return;
  	}
  
  	//	see if this is a reward character
  
  	int index = findIndex( MESSAGES__REWARDS, N_REWARDS, message );
  	if ( index != -1 ) {
      relay( MESSAGES__REWARDS[index] );
  		deliverReward( index, REWARD_SIZE );
  	}
    return;
  }

  //  otherwise, this is an eye position

  String inStr = "";
  while ( Wire.available() > 0 ) {
    char inChar = Wire.read();
    inStr += inChar;
  }

  Serial.println( inStr );

}

void handleRequest() {
	Wire.write( message );
}

void deliverReward( int index, int amount ) {
	digitalWrite( REWARD_PINS[ index ], HIGH );
	delay( amount );
	digitalWrite( REWARD_PINS[ index ], LOW );
	delay( 100 );
}

char toChar( byte toConvert ) {
  return toConvert & 0xff;
}

int findIndex( char arr[], int len, char lookFor ) {
  for ( int i = 0; i < len; ++i ) {
    if ( arr[i] == lookFor ) return i; 
  }
  return -1;
}
