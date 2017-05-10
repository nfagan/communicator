#include <Wire.h>
#include <BrainsTask.h>

//  MESSAGES

// DEBUG
char DEBUG__RESET_GAZE = 'D';
char DEBUG__PRINT_GAZE = 'P';
char DEBUG__COMPARE_GAZE = 'C';
char DEBUG__PRINT_REWARDS = 'Q';
bool DEBUG = false;

bool SHOULD_DELIVER_REWARD = false;
int CURRENT_REWARD_INDEX = 0;

//  ADDRESSES
int SLAVE_ADDRESS = 9;

//  READ FROM SERIAL
byte byteRead;

//  Check if initialized
bool INITIALIZED = false;

//  response character
char message = MESSAGE__ERROR;

//  configure reward
const int N_REWARDS = 2;
int REWARD_PINS[ N_REWARDS ] = { 11, 40 };
// IMPORTANT -- Check master.ino to ensure
// none of these reward characters are those
// match those of the slave
char MESSAGES__REWARDS[ N_REWARDS ] = { 'A', 'B' };
int REWARD_SIZES[ N_REWARDS ] = { 100, 100 };

//  GAZE POSITION
long OWN[ 2 ] = { 0, 0 };
long OTHER[ 2 ] = { 0, 0 };
int BOUNDS = 200;

//  STATES
int STATES[ 2 ] = { 0, 0 };

//  FIXATION DURATION

int FIX_MET[ 2 ] = { 0, 0 };

//  Choices
int CORRECT_CHOICE = 0;

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

  //  slave serial input

  if ( !INITIALIZED ) {
    Serial.println( '*' );
    INITIALIZED = true;
  }

  if ( Serial.available() ) {
    byteRead = Serial.read();
    char readChar = toChar( byteRead );
    //  update our OWN current gaze coordinates
    //  if readChar corresponds to MESSAGE__EYE_START    
    if ( readChar == MESSAGE__EYE_START ) {
      String eyePosition = readIn( MESSAGE__EYE_END, "" );
      updateEyePosition( eyePosition, true );
    } else if ( readChar == MESSAGE__REWARD_SIZE_START ) {
      String rewardSize = readIn( MESSAGE__REWARD_SIZE_END, "" );
      updateRewardSizes( rewardSize );
    } else if ( readChar == MESSAGE__CHOICE_START ) {
      String chosenOption = readIn( MESSAGE__CHOICE_END, "" );
      updateChoice( chosenOption );
    } else if ( readChar == MESSAGE__STATE_START ) {
      //  n.b. incomingState must be defined with a single
      //  char, unlike eyePosition, etc.
      String incomingState = readIn( MESSAGE__STATE_END, "b" );
      updateStates( incomingState, true );
    } else if ( readChar == MESSAGE__FIX_START ) {
      String readFix = readIn( MESSAGE__FIX_END, "" );
      updateFixMet( readFix, true );
    } else if ( readChar == MESSAGE__COMPARE_STATES ) {
      Serial.println( statesMatchChar() );
    } else if ( readChar == MESSAGE__COMPARE_GAZE ) {
      Serial.println( gazesMatchChar() );
    } else if ( readChar == MESSAGE__COMPARE_FIX_MET ) {
      Serial.println( fixMetMatchChar() );
    } else if ( readChar == DEBUG__PRINT_REWARDS ) {
      printRewards();
    } else if ( readChar == DEBUG__COMPARE_GAZE ) {
      compareOwnToOtherPosition();
    } else if ( readChar == DEBUG__PRINT_GAZE ) {
      printGaze( true ); printGaze( false );
    } else if ( readChar == DEBUG__RESET_GAZE ) {
      resetGaze( true ); resetGaze( false );
    } else if ( readChar == MESSAGE__REQUEST_CHOICE ) {
      Serial.println( CORRECT_CHOICE );
      CORRECT_CHOICE = 0;
    } else {
      //  check to see if this is a reward message
      int index = findIndex( MESSAGES__REWARDS, N_REWARDS, readChar );
      if ( index != -1 ) {
        deliverReward( index );
      }
    }
  }

  //  allow master to trigger rewards
  if ( SHOULD_DELIVER_REWARD ) {
    deliverReward( CURRENT_REWARD_INDEX );
    SHOULD_DELIVER_REWARD = false;
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

  //  Respond to transmission of data from master.

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
    if ( message == MESSAGE__COMPARE_STATES ) {
      Wire.write( statesMatchChar() );
      return;
    }
    if ( message == MESSAGE__COMPARE_FIX_MET ) {
      Wire.write( fixMetMatchChar() );
      return;
    }
    if ( message == MESSAGE__COMPARE_GAZE ) {
      Wire.write( gazesMatchChar() );
      return;
    }
    if ( message == DEBUG__PRINT_GAZE ) {
      printGaze( true );
      printGaze( false );
      return;
    }
    //  see if this is a reward character
    int rwdIndex = findIndex( MESSAGES__REWARDS, N_REWARDS, message );
    if ( rwdIndex != -1 ) {
//      deliverReward( rwdIndex );
      SHOULD_DELIVER_REWARD = true;
      CURRENT_REWARD_INDEX = rwdIndex;
    }
    return;
  }
  //  otherwise, this is an eye position, reward size
  //  state message, or choice message
  String valueStr = "";
  while ( Wire.available() > 0 ) {
    char pos = Wire.read();
    valueStr += pos;
  }
  
  //  valueStr[0] will be the
  //  X or Y target. Update the corresponding
  //  OTHER array accordingly

  char idChar = valueStr.charAt( 0 );
  bool isRewardMessage = findIndex( MESSAGES__REWARDS, N_REWARDS, idChar ) != -1;
  if ( isRewardMessage ) {
    updateRewardSizes( valueStr );
    return;
  }
  if ( idChar == 'X' || idChar == 'Y' ) {
    updateEyePosition( valueStr, false );
  } else if ( idChar == MESSAGE__CHOICE_ID ) {
    updateChoice( valueStr );
  } else if ( idChar == MESSAGE__FIX_ID ) {
    updateFixMet( valueStr, false );
  } else {
    updateStates( valueStr, false );
  }
}

bool gazesMatch() {
  bool isWithinBounds[ 2 ] = { false, false };
  for ( int i = 0; i < 2; i++ ) {
    isWithinBounds[i] = abs( OWN[i] - OTHER[i] ) <= BOUNDS;
    if ( !isWithinBounds[i] ) {
      return false;
    }
  }
  return true;
}

char gazesMatchChar() {
  if ( gazesMatch() ) {
    return '1';
  }
  return '0';
}

void compareOwnToOtherPosition() {
  if ( gazesMatch() ) {
    Serial.println( "IB" );
  } else {
    Serial.println( "OOB" );
  }
}

bool statesMatch() {
  return STATES[0] == STATES[1];
}

char statesMatchChar() {
  if ( statesMatch() ) {
    return '1';
  }
  return '0';
}

bool fixMetMatch() {
  if ( FIX_MET[0] == 0 ) return false;
  return FIX_MET[0] == FIX_MET[1];
}

char fixMetMatchChar() {
  if ( fixMetMatch() ) return '1';
  return '0';
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

void updateStates( String incomingState, bool isOwn ) {
  int index = 1;
  if ( isOwn ) {
    index = 0;
  }
  STATES[index] = stringToInt( incomingState, 1 );
}

void updateFixMet( String incomingFix, bool isOwn ) {
  int index = 1;
  if ( isOwn ) {
    index = 0;
  }
  FIX_MET[index] = stringToInt( incomingFix, 1 );
}

int stringEyePositionToInt( String eyePosition ) {
  return stringToInt( eyePosition, 1 );
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

void updateChoice( String choice ) {
  CORRECT_CHOICE = stringToInt( choice, 1 );
}

void handleRequest() {
  if ( message == MESSAGE__COMPARE_STATES ) {
    Wire.write( statesMatchChar() );
  } else if ( message == MESSAGE__COMPARE_GAZE ) {
    Wire.write( gazesMatchChar() );
  } else if ( message == MESSAGE__REQUEST_CHOICE ) {
    Wire.write( smallIntToDecimalChar(CORRECT_CHOICE) );
    CORRECT_CHOICE = 0;
  } else if ( message == MESSAGE__COMPARE_FIX_MET )  {
    Wire.write( fixMetMatchChar() );
  } else {
    Wire.write( message );
  }
}

void deliverReward( int index ) {
  if ( DEBUG ) Serial.println("reward!");
  digitalWrite( REWARD_PINS[ index ], HIGH );
  digitalWrite( LED_BUILTIN, HIGH );
  delay( REWARD_SIZES[ index ] );
  digitalWrite( REWARD_PINS[ index ], LOW );
  digitalWrite( LED_BUILTIN, LOW );
  delay( 100 );
}

char smallIntToDecimalChar( int a ) {
  char b[2];
  String str;
  str=String( a );
  str.toCharArray( b, 2 );
  return b[0];
}

