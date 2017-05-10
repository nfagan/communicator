#include <Wire.h>
#include <BrainsTask.h>

//  Other messages are determined in BrainsTask.h

//	ADDRESSES
int SLAVE_ADDRESS = 9;

//	READ FROM SERIAL
byte byteRead;

//	response character
char message = MESSAGE__ERROR;

// Initialization check
bool INITIALIZED = false;

//  configure rewards
const int N_REWARDS = 2;
int REWARD_PINS[ N_REWARDS ] = { 7, 40 };
// IMPORTANT -- Check slave.ino to ensure
// none of these reward characters are those
// match those of the slave
char MESSAGES__REWARDS[ N_REWARDS ] = { 'N', 'M' };
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
  if ( !INITIALIZED ) {
    Serial.println( '*' );
    INITIALIZED = true;
  }

  if ( Serial.available() ) {
    byteRead = Serial.read();

    char readChar = toChar( byteRead );

    if ( readChar == MESSAGE__SYNCH ) {
      synchronize();
    } else if ( readChar == MESSAGE__REQUEST_CHOICE ) {
      char choice = transmitAndReceive( readChar );
      Serial.println( choice );
    } else if ( readChar == MESSAGE__COMPARE_GAZE ) {
      Serial.println( gazesMatchChar() );
    } else if ( readChar == MESSAGE__COMPARE_STATES ) {
      Serial.println( statesMatchChar() );
    } else if ( readChar == MESSAGE__COMPARE_FIX_MET ) {
      Serial.println( fixMetMatchChar() );
    } else if ( readChar == MESSAGE__FIX_START ) {
      readAndTransmit( SLAVE_ADDRESS, MESSAGE__FIX_END, "" );
    } else if ( readChar == MESSAGE__STATE_START ) {
      readAndTransmit( SLAVE_ADDRESS, MESSAGE__STATE_END, "b" );
    } else if ( readChar == MESSAGE__CHOICE_START ) {
      readAndTransmit( SLAVE_ADDRESS, MESSAGE__CHOICE_END, "" );
    } else if ( readChar == MESSAGE__EYE_START ) {
      readAndTransmit( SLAVE_ADDRESS, MESSAGE__EYE_END, "" );
    } else if ( readChar == MESSAGE__REWARD_SIZE_START ) {
      readAndTransmit( SLAVE_ADDRESS, MESSAGE__REWARD_SIZE_END, "" );
    } else if ( readChar == 'T' ) {
      //      relay( '*' );
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

void synchronize() {
  transmit( MESSAGE__SYNCH );

  Wire.requestFrom( SLAVE_ADDRESS, 1 );

  while ( !Wire.available() ) {
    //	wait in the loop until we get a response
    delay( 10 );
    //relay( MESSAGE__AWAIT_SYNCH );
    continue;
  }

  char response = Wire.read();

  if ( response == MESSAGE__SYNCH ) {
    relay( response );
  } else {
    synchronize();
  }
}

char transmitAndReceive( char msg ) {
  transmit( msg );
  Wire.requestFrom( SLAVE_ADDRESS, 1 );
  while ( !Wire.available() ) {
    delay( 10 );
  }
  char response = Wire.read();
  return response;
}

bool matchesOther( char msg ) {
  char response = transmitAndReceive( msg );
  if ( response == '1' ) {
    return true;
  } else {
    return false;
  }
}

bool gazesMatch() {
  return matchesOther ( MESSAGE__COMPARE_GAZE );
}

bool statesMatch() {
  return matchesOther( MESSAGE__COMPARE_STATES );
}

bool fixMetMatch() {
  return matchesOther( MESSAGE__COMPARE_FIX_MET );
}

char statesMatchChar() {
  if ( statesMatch() ) {
    return '1';
  }
  return '0';
}

char gazesMatchChar() {
  if ( gazesMatch() ) {
    return '1';
  }
  return '0';
}

char fixMetMatchChar() {
  if ( fixMetMatch() ) return '1';
  return '0';
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
