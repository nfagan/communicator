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
char MESSAGE__REWARD_SIZE_START = 'U';
char MESSAGE__REWARD_SIZE_END = 'V';
char MESSAGE__REWARD_DELIVERED = 'R';

// DEBUG

char DEBUG__RESET_GAZE = 'D';
char DEBUG__PRINT_GAZE = 'P';
char DEBUG__COMPARE_GAZE = 'C';
char DEBUG__PRINT_REWARDS = 'Q';

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
int REWARD_SIZES[ N_REWARDS ] = { 100, 100 };

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
  pinMode( LED_BUILTIN, OUTPUT );
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
    } else if ( readChar == MESSAGE__REWARD_SIZE_START ) {
      String rewardSize = "";
      while ( Serial.available() == 0 ) {
        delay(5);
      }
      while ( Serial.available() > 0 ) {
        int size_int = Serial.read();
        if ( size_int == MESSAGE__REWARD_SIZE_END ) {
          break;
        } else {
          rewardSize += (char)size_int;
        }
      }
      updateRewardSizes( rewardSize );
    } else if ( readChar == DEBUG__PRINT_REWARDS ) {
      printRewards();
    } else if ( readChar == DEBUG__COMPARE_GAZE ) {
      compareOwnToOtherPosition();
    } else if ( readChar == DEBUG__PRINT_GAZE ) {
      printGaze( true ); printGaze( false );
    } else if ( readChar == DEBUG__RESET_GAZE ) {
      resetGaze( true ); resetGaze( false );
    } else {
      //  check to see if this is a reward message
      int index = findIndex( MESSAGES__REWARDS, N_REWARDS, readChar );
      if ( index != -1 ) {
        deliverReward( index );
      }
    }
  }
}

void relay( char c ) {
  Serial.write( c );
}

void resetGaze( bool resetOwn ) {
  for ( int i = 0; i < 2; i++ ) {
    if ( resetOwn ) {
      OWN[i] = 0;    
    } else {
      OTHER[i] = 0;
    }
  }
}

void printGaze( bool logOwn ) {
  char ids[ 2 ] = { 'X', 'Y' };
  for ( int i = 0; i < 2; i++ ) {
    if ( logOwn ) {
      Serial.println( "OWN:" );
      Serial.println( ids[i] );
      Serial.println( OWN[i] );
    } else {
      Serial.println( "OTHER:" );
      Serial.println( ids[i] );
      Serial.println( OTHER[i] );
    }
  }
}

void printRewards() {
  for ( int i = 0; i < N_REWARDS; i++ ) {
    Serial.println( MESSAGES__REWARDS[i] );
    Serial.println( REWARD_SIZES[i] );
  }
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
    if ( message == DEBUG__PRINT_GAZE ) {
      printGaze( true );
      printGaze( false );
    }
    //	see if this is a reward character
    int index = findIndex( MESSAGES__REWARDS, N_REWARDS, message );
    if ( index != -1 ) {
//      relay( MESSAGES__REWARDS[index] );
      deliverReward( index );
    }
    return;
  }
  //  otherwise, this is either an eye position
  //  or a reward size
  String valueStr = "";
  while ( Wire.available() > 0 ) {
    char pos = Wire.read();
    valueStr += pos;
  }
  
  //  valueStr[0] will be the
  //  X or Y target. Update the corresponding
  //  OTHER array accordingly

  char id_char = valueStr.charAt( 0 );
  if ( id_char == 'X' || id_char == 'Y' ) {
    updateEyePosition( valueStr, false ); 
  } else {
    updateRewardSizes( valueStr ); 
  }
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
  } else {
    OTHER[ posIndex ] = stringEyePositionToInt( eyePosition );
  }
}

int stringEyePositionToInt( String eyePosition ) {
  return stringToInt( eyePosition, 1 );
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

void handleRequest() {
  Wire.write( message );
}

void deliverReward( int index ) {
  digitalWrite( REWARD_PINS[ index ], HIGH );
  digitalWrite( LED_BUILTIN, HIGH );
  delay( REWARD_SIZES[ index ] );
  digitalWrite( REWARD_PINS[ index ], LOW );
  digitalWrite( LED_BUILTIN, LOW );
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
