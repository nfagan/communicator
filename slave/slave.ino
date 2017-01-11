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

//  GAZE POSITION

long OWN[ 2 ] = { 0, 0 };
long OTHER[ 2 ] = { 0, 0 };
int BOUNDS = 200;

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

  //	slave serial input

  if ( Serial.available() ) {
    byteRead = Serial.read();
    char readChar = toChar( byteRead );
    //  update our OWN current gaze coordinates
    //  if readChar corresponds to MESSAGE__EYE_START    
    if ( readChar == MESSAGE__EYE_START ) {
      String eyePosition = "";
      while ( Serial.available() == 0 ) {
        delay(5);
      }
      while ( Serial.available() > 0 ) {
        int pos = Serial.read();
        if ( pos == MESSAGE__EYE_END ) {
          break;
        } else {
          eyePosition += (char)pos;
        }
      }
      updateEyePosition( eyePosition, true );
    } else if ( readChar == 'P' ) {
      compareOwnToOtherPosition();
    } else {
      //  check to see if this is a reward message
      int index = findIndex( MESSAGES__REWARDS, N_REWARDS, readChar );
      if ( index != -1 ) {
        deliverReward( index, REWARD_SIZE );
      }
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

  String eyePosition = "";
  while ( Wire.available() > 0 ) {
    char pos = Wire.read();
    eyePosition += pos;
  }
  
  //  eyePosition[0] will be the
  //  X or Y target. Update the corresponding
  //  OTHER array accordingly

  updateEyePosition( eyePosition, false );
  
}

void compareOwnToOtherPosition() {
  bool isWithinBounds[ 2 ] = { false, false };
  for ( int i = 0; i < 2; i++ ) {
    isWithinBounds[i] = abs( OWN[i] - OTHER[i] ) <= BOUNDS;
    if ( !isWithinBounds[i] ) {
      Serial.println( "OOB" );
      return;
    }
  }
  //  otherwise, we're in bounds
  Serial.println( "IB" );
}

void updateEyePosition( String eyePosition, bool isOwn ) {
  char positionID = eyePosition.charAt( 0 );
  bool valid = false;
  int posIndex;
  if ( positionID == 'X' ) {
    valid = true;
    posIndex = 0;
  } else if ( positionID == 'Y' ) {
    valid = true;
    posIndex = 1;
  }
  if ( !valid ) {
    Serial.println( '!' ); 
    return;
  }
  if ( isOwn ) {
    OWN[ posIndex ] = stringEyePositionToInt( eyePosition );
    Serial.println( "OWN" );
    Serial.println( OWN[ posIndex ] );
  } else {
    OTHER[ posIndex ] = stringEyePositionToInt( eyePosition );
    Serial.println( "OTHER" );
    Serial.println( OTHER[ posIndex ] );
  }
}

int stringEyePositionToInt( String eyePosition ) {
  int bufferSize = eyePosition.length() + 1;
  char charNumber[ bufferSize ] = { 'b' };
  //  get rid of the X or Y
  eyePosition.remove(0, 1);
  eyePosition.toCharArray( charNumber, bufferSize );
  return atol( charNumber );
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
