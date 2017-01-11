messages = { ...
  struct('message', 'TEST', 'char', 'EX123T'), ...
  struct('message', 'REWARDA', 'char', 'A'), ...
  struct('message', 'REWARDB', 'char', 'B') ...
};

port = 'COM3';
baud_rate = 115200;

comm = Communicator( messages, port, baud_rate );

%%

comm.send_gaze( 'X', 200 );
comm.send_gaze( 'Y', 250 );

%%

comm.send( 'REWARDA' );
