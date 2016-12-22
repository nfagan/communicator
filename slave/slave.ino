#include <Wire.h>

//  synchronization character

char MESSAGE__SYNCH = 'S';
char MESSAGE__AWAIT_SYNCH = 'W';
char MESSAGE__ERROR = '1';

//  rewards

char MESSAGES__REWARDS[ 2 ] = { 'A', 'B' };

int SLAVE_ADDRESS = 9;

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
	Serial.begin( 9600 );
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

    deliverReward( readChar, REWARD_SIZE );
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

	message = Wire.read();

	if ( message == MESSAGE__SYNCH ) {
		Wire.write( message );
		relay( message ); return;
	}

	//	otherwise, send a reward

	deliverReward( message, REWARD_SIZE );

}

void handleRequest() {
	Wire.write( message );
}

void deliverReward( char msg, int amount ) {
	int index = findIndex( MESSAGES__REWARDS, N_REWARDS, msg );
	if ( index == -1 ); return;

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