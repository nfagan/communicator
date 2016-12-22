classdef Communicator

  properties
    messages;
    chars;
    communicator;
  end

  methods

    %{
        initialize
    %}

    function obj = Communicator(messages, port)
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
      obj = obj.serialinit(port);
    end

    %{
        handle serial communication
    %}

    %   initialize connection

    function obj = serialinit(obj, port)
      obj.communicator = serial(port);
      obj.start();
    end

    %   send a message

    function send(obj, message)            
      index = strcmp(obj.messages, message);

      if ~any(index); error('The message ''%s'' has not been defined', message); end;

      char = obj.chars{index};

      fprintf(obj.communicator,'%s',char);
    end

    %   receive a message

    function response = receive(obj, n)
      if ( nargin < 2 ); n = 1; end;
      response = fscanf(obj.communicator, '%s', n);
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