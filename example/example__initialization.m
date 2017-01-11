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

%%
i = 1;
while i <= 100;
  pause( .001 );
  comm.send_gaze('X', i); comm.send_gaze('Y', i*2); i = i + 1;
end
