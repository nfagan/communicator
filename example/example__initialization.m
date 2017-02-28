messages = { ...
  struct('message', 'SYNCHRONIZE', 'char', 'S'), ...
  struct('message', 'REWARD1', 'char', 'A'), ...
  struct('message', 'REWARD2', 'char', 'B'), ...
  struct('message', 'REWARD3', 'char', 'M'), ...
  struct('message', 'REWARD4', 'char', 'N'), ...
  struct('message', 'PRINT_GAZE', 'char', 'P'), ...
  struct('message', 'COMPARE', 'char', 'C' ) ...
};

port = 'COM3';
baud_rate = 115200;

comm = Communicator( messages, port, baud_rate );

%%

comm.send_gaze( 'X', 200 );
comm.send_gaze( 'Y', 280 );

%%

comm.send( 'COMPARE' );

%%

comm.send_reward_size( 'B', 1000 );

%%
i = 1;
while i <= 100;
  pause( .001 );
  comm.send_gaze('X', i); comm.send_gaze('Y', i*2); i = i + 1;
end
%%
last = 0;
tic;
while 1
  elapsed = toc - last;
  if ( elapsed > 2 )
    comm.send( 'REWARDA' );
    last = toc;
  end
end
