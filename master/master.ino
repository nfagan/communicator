#include <Wire.h>

//  MESSAGES

char MESSAGE__SYNCH = 'S';
char MESSAGE__AWAIT_SYNCH = 'W';
char MESSAGE__ERROR = '1';
char MESSAGE__EYE_START = 'E';
char MESSAGE__EYE_END = 'T';
char MESSAGE__EYE_X = 'X';
char MESSAGE__EYE_Y = 'Y';
// IMPORTANT -- Check slave.ino to ensure
// none of these reward characters are those
// match those of the slave
char MESSAGES__REWARDS[ 2 ] = { 'N', 'M' };
char MESSAGE__REWARD_SIZE_START = 'U';
char MESSAGE__REWARD_SIZE_END = 'V';
char MESSAGE__REWARD_DELIVERED = 'R';

//	ADDRESSES

int SLAVE_ADDRESS = 9;

//	READ FROM SERIAL

byte byteRead;

//	response character

char message = MESSAGE__ERROR;

//  configure pin numbers

const int N_REWARDS = 2;
int REWARD_PINS[ N_REWARDS ] = { 37, 40 };

//  reward size

const int REWARD_SIZE = 100;
int REWARD_SIZES[ N_REWARDS ] = { 100, 100 };

//	BEGIN

void setup() {
  Wire.begin();
  Serial.begin( 115200 );
  for ( int i = 0; i < N_REWARDS; i++ ) {
    pinMode( REWARD_PINS[i], OUTPUT );
  }
  pinMode( LED_BUILTIN, OUTPUT );
}

void loop() {

  //	serial handling

  if ( Serial.available() ) {
    byteRead = Serial.read();

    char readChar = toChar( byteRead );

    if ( readChar == MESSAGE__SYNCH ) {
      synchronize();
    } else if ( readChar == MESSAGE__EYE_START ) {
      String eyePosition = "";
      while ( Serial.available() == 0 ) {
        delay(5);
      }
      while ( Serial.available() > 0 ) {
        int pos = Serial.read();
        if ( pos == MESSAGE__EYE_END ) {
          delay( 5 );
          Wire.beginTransmission( SLAVE_ADDRESS );
          Wire.write( eyePosition.c_str() );
          Wire.endTransmission();
          break;
        } else {
          eyePosition += (char)pos;
        }
      }
    } else if ( readChar == MESSAGE__REWARD_SIZE_START ) {
      String rewardSize = "";
      while ( Serial.available() == 0 ) {
        delay(5);
      }
      while ( Serial.available() > 0 ) {
        int pos = Serial.read();
        if ( pos == MESSAGE__REWARD_SIZE_END ) {
          delay( 5 );
          int rwdIndex = findIndex( MESSAGES__REWARDS, N_REWARDS, rewardSize.charAt(0) );
          // is this the master's reward, or the slave's?
          // if we can't find the character in the MESSAGES__REWARDS array,
          // it must the slave's reward
          if ( rwdIndex == -1 ) {
            Wire.beginTransmission( SLAVE_ADDRESS );
            Wire.write( rewardSize.c_str() );
            Wire.endTransmission();
          } else {
            updateRewardSizes( rewardSize );
          }
          break;
        } else {
          rewardSize += (char)pos;
        }
      }
    } else if ( readChar == 'T' ) {
      relay( '*' );
    } else {
      //	is the reward message one of the master's rewards?
      //  if so, deliver the reward. if not, transmit the read char
      //  to the slave.
      int rwdIndex = findIndex( MESSAGES__REWARDS, N_REWARDS, readChar );
      if ( rwdIndex == -1 ) {
        transmit( readChar );
      } else {
        deliverReward( rwdIndex );
      }
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

int stringToInt( String str, int removeNLeading ) {
  int bufferSize = str.length() + 1;
  char charNumber[ bufferSize ] = { 'b' };
  if ( removeNLeading > 0 ) {
    for ( int i = 0; i < removeNLeading; i++ ) {
      str.remove(0, 1);
    }
  }
  str.toCharArray( charNumber, bufferSize );
  return atol( charNumber );
}

void updateRewardSizes( String rewardSize ) {
  //  Update the corresponding value of REWARD_SIZES
  //  given a string rewardSize.
  char rewardID = rewardSize.charAt( 0 );
  int rewardIndex = findIndex( MESSAGES__REWARDS, N_REWARDS, rewardID );
  if ( rewardIndex == -1 ) {
    Serial.println( '!' );
    return;
  }
  REWARD_SIZES[ rewardIndex ] = stringToInt( rewardSize, 1 );
}

void deliverReward( int index ) {
  digitalWrite( REWARD_PINS[ index ], HIGH );
  digitalWrite( LED_BUILTIN, HIGH );
  delay( REWARD_SIZES[ index ] );
  digitalWrite( REWARD_PINS[ index ], LOW );
  digitalWrite( LED_BUILTIN, LOW );
  delay( 100 );
}

int findIndex( char arr[], int len, char lookFor ) {
  for ( int i = 0; i < len; ++i ) {
    if ( arr[i] == lookFor ) return i;
  }
  return -1;
}
