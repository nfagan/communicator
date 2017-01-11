messages = { ...
  struct('message', 'SYNCHRONIZE', 'char', 'S'), ...
  struct('message', 'REWARDA', 'char', 'A'), ...
  struct('message', 'REWARDB', 'char', 'B'), ...
  struct('message', 'COMPARE', 'char', 'P') ...
};

port = 'COM3';
baud_rate = 115200;

comm = Communicator( messages, port, baud_rate );

%%

comm.send_gaze( 'X', 200 );
comm.send_gaze( 'Y', 250 );

%%

comm.send( 'REWARDA' );
