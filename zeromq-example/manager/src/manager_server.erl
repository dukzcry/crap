-module(manager_server).

-behaviour(gen_server).

%% API
-export([
	 start_link/0,
	 test_call/1,
	 test_cast/1,
	 send/1
	]).

%% gen_server callbacks
-export([init/1, handle_call/3, handle_cast/2, handle_info/2,
	 terminate/2, code_change/3]).

-define(SERVER, ?MODULE). 

-record(state, {zmq_context=undefined, listen_socket=undefined, 
    listen_socket_async=undefined, multi_message=[]}).

%%%===================================================================
%%% API
%%%===================================================================

%%--------------------------------------------------------------------
%% @doc
%% Starts the server
%%
%% @spec start_link() -> {ok, Pid} | ignore | {error, Error}
%% @end
%%--------------------------------------------------------------------
start_link() ->
    gen_server:start_link({local, ?SERVER}, ?MODULE, [], []).

test_call(Msg) ->
    gen_server:call(?SERVER, {test, Msg}).

test_cast(Msg) ->
    gen_server:cast(?SERVER, {test, Msg}).

send(Msg) ->
    gen_server:call(?SERVER, {message, Msg}).

%%%===================================================================
%%% gen_server callbacks
%%%===================================================================

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Initializes the server
%%
%% @spec init(Args) -> {ok, State} |
%%                     {ok, State, Timeout} |
%%                     ignore |
%%                     {stop, Reason}
%% @end
%%--------------------------------------------------------------------
init(_Args) ->
    {ok, Context} = erlzmq:context(),
    {ok, Listener} = erlzmq:socket(Context, [rep, {active_pid, self()}]),
    {ok, Listener_async} = erlzmq:socket(Context, pub),
    Status = erlzmq:bind(Listener, "tcp://127.0.0.1:5560"),
    Status = erlzmq:bind(Listener_async, "tcp://127.0.0.1:5561"),
    {Status, #state{zmq_context=Context, listen_socket=Listener, 
        listen_socket_async=Listener_async}}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling call messages
%%
%% @spec handle_call(Request, From, State) ->
%%                                   {reply, Reply, State} |
%%                                   {reply, Reply, State, Timeout} |
%%                                   {noreply, State} |
%%                                   {noreply, State, Timeout} |
%%                                   {stop, Reason, Reply, State} |
%%                                   {stop, Reason, State}
%% @end
%%--------------------------------------------------------------------
%handle_call({test, Message}, _From, State) ->
%    io:format("Call: ~p~n", [Message]),
%    Reply = ok,
%    {reply, Reply, State};
handle_call({message, Msg}, _From, State) ->
    erlzmq:send(State#state.listen_socket_async, 
        unicode:characters_to_binary(Msg)),
    {reply, ok, State};
handle_call(_Request, _From, State) ->
    Reply = ok,
    {reply, Reply, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling cast messages
%%
%% @spec handle_cast(Msg, State) -> {noreply, State} |
%%                                  {noreply, State, Timeout} |
%%                                  {stop, Reason, State}
%% @end
%%--------------------------------------------------------------------
handle_cast({test, Message}, State) ->
    io:format("Cast: ~p~n", [Message]),
    {noreply, State};
handle_cast(_Msg, State) ->
    {noreply, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling all non call/cast messages
%%
%% @spec handle_info(Info, State) -> {noreply, State} |
%%                                   {noreply, State, Timeout} |
%%                                   {stop, Reason, State}
%% @end
%%--------------------------------------------------------------------
handle_info({zmq, _Socket, Msg, [rcvmore]}, State)
    when State#state.multi_message == [] ->
    {noreply, State#state{multi_message = [binary_to_term(Msg)]}};
handle_info({zmq, _Socket, Msg, [rcvmore]}, State) ->
    Message = lists:append(State#state.multi_message, [binary_to_term(Msg)]),
    {noreply, State#state{multi_message = Message}};
handle_info({zmq, _Socket, Msg, []}, State) ->
    Message = lists:append(State#state.multi_message, [binary_to_term(Msg)]),
    %Format = fun(X) -> 
    %    io:format("~ts~n", [X])
    %end,
    %lists:foreach(Format, Message),
    send_parts(State#state.listen_socket, [<<"ok">>,<<"done">>]),
    {noreply, State#state{multi_message = []}};
handle_info(_Info, State) ->
    {noreply, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% This function is called by a gen_server when it is about to
%% terminate. It should be the opposite of Module:init/1 and do any
%% necessary cleaning up. When it returns, the gen_server terminates
%% with Reason. The return value is ignored.
%%
%% @spec terminate(Reason, State) -> void()
%% @end
%%--------------------------------------------------------------------
terminate(_Reason, #state{zmq_context=Context, listen_socket=Listener, 
    listen_socket_async=Listener_async}) ->
    erlzmq:close(Listener),
    erlzmq:close(Listener_async),
    erlzmq:term(Context),
    ok;
terminate(_Reason, _State) ->
    ok.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Convert process state when code is changed
%%
%% @spec code_change(OldVsn, State, Extra) -> {ok, NewState}
%% @end
%%--------------------------------------------------------------------
code_change(_OldVsn, State, _Extra) ->
    {ok, State}.

%%%===================================================================
%%% Internal functions
%%%===================================================================
send_parts(Socket, [Part]) ->
    erlzmq:send(Socket, Part);
send_parts(Socket, [Part|Rest]) ->
    erlzmq:send(Socket, Part, [sndmore]),
    %timer:sleep(5000),
    send_parts(Socket, Rest);
send_parts(Socket, Msg) ->
    erlzmq:send(Socket, Msg).
