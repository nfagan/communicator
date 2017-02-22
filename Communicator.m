classdef Communicator

  properties
    messages;
    chars;
    communicator;
    const = struct( ...
      'chars', struct( ...
          'GAZE_START', 'E', 'GAZE_END', 'T' ...
        , 'RWD_SIZE_START', 'U', 'RWD_SIZE_END', 'V' ...
        ) ...
      );
  end

  methods

    %{
        initialize
    %}

    function obj = Communicator(messages, port, baud_rate)
      err = ['Messages must be a cell array of structures with ''message''' ...
          , ' and ''char'' fields'];
      %   make sure messages are formatted correctly
      assert(iscell(messages), err);
      assert(isstruct(messages{1}), err);
      %   extract character ids and messages as separate vars
      chars = cellfun(@(x) x.char, messages, 'UniformOutput', false);
      messages = cellfun(@(x) x.message, messages, 'UniformOutput', false);
      %   make sure there's a character for every message
      assert(all(size(chars) == size(messages)), err);
      obj.messages = messages; obj.chars = chars;
      obj = obj.serialinit( port, baud_rate );
    end

    %{
        handle serial communication
    %}

    function obj = serialinit(obj, port, baud_rate)
      
      %   SERIALINIT -- Initialize the serial connection to the arduino.
      %
      %     Specify a port on which to connect, and optionally specify the
      %     baud_rate.
      %
      %     IN:
      %       - `port` (char) -- String representation of the port on which
      %         to connect. E.g., 'COM3'.
      %       - `baud_rate` (number) -- Serial transmission rate.
      %         Transmission rates between computers must match.
      
      obj.communicator = serial( port );
      obj.communicator.BaudRate = baud_rate;
      obj.start();
    end

    function send(obj, message)
      
      %   SEND -- send the character or characters associated with 
      %     `message`.
      %
      %   IN:
      %       - `message` (char) -- Human-readable message associated with
      %         the machine-readable char.
      
      index = strcmp( obj.messages, message );
      if ~any(index); error('The message ''%s'' has not been defined', message); end;
      char = obj.chars{ index };
      fprintf( obj.communicator, '%s', char );
    end
    
    function send_reward_size(obj, channel, value)
      
      %   SEND_REWARD_SIZE -- Update the reward size associated with the
      %     specified channel.
      %
      %     IN:
      %       - `channel` (char) -- Channel identifier.
      %       - `value` (int) -- New reward size, in ms.
      
      RWD_START = obj.const.chars.RWD_SIZE_START;
      RWD_END = obj.const.chars.RWD_SIZE_END;
      str = sprintf( '%s%s%d%s', RWD_START, upper(channel), round(value) ...
        , RWD_END );
      fprintf( obj.communicator, '%s', str );
    end
    
    function send_gaze(obj, dimension, values)
      
      %   SEND_GAZE -- send gaze data in either the 'X' or 'Y' dimension.
      %
      %     Values are rounded, converted to a string, and bookended by the
      %     `gaze_start` and `gaze_stop` characters.
      %
      %     IN:
      %       - `dimension` ('X' or 'Y') -- specify which dimension the
      %         value corresponds to.
      %       - `values` (number) -- numeric x or y position.
      
      GAZE_START = obj.const.chars.GAZE_START;
      GAZE_END = obj.const.chars.GAZE_END;
      str = sprintf( '%s%s%d%s', GAZE_START, upper(dimension), round(values), ...
        GAZE_END );
      fprintf( obj.communicator, '%s', str );
    end

    %   receive a message

    function response = receive(obj, n)
      if ( nargin < 2 ); n = 1; end;
      response = fscanf( obj.communicator, '%s', n );
    end
    
    function responses = receive_all(obj)
      
      %   RECEIVE_ALL -- Concatenate all available responses into a single
      %     response.
      %
      %     OUT:
      %       `responses` (char) -- character vector of responses received
      %       since the last call to receive_all() or receive();
      
      N = obj.communicator.BytesAvailable;
      responses = '';
      if ( N == 0 ), return; end;      
      while ( N > 0 )
        responses = sprintf( '%s%s', responses, ...
          fscanf(obj.communicator, '%s', 1) );
        N = obj.communicator.BytesAvailable;
      end
    end

    %   start the serial communicator

    function start(obj)
      fopen(obj.communicator);
    end

    %   stop the serial communicator

    function stop(obj)
      fclose(obj.communicator);
    end

    %   display status

    function state = status(obj)
      state = obj.communicator.status;
    end

    %   wait for a message to become available
    
    function response = await(obj, n)
      if ( nargin < 2 ), n = 1; end;
      
      while ( obj.communicator.BytesAvailable == 0 )
        continue;
      end
      
      response = receive( obj, n );
    end
  end

end