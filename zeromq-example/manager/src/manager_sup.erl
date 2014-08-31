-module(manager_sup).
-behaviour(supervisor).

%% API
-export([start_link/0]).

%% Supervisor callbacks
-export([init/1]).

-define(SERVER, ?MODULE).

start_link() ->
    supervisor:start_link({local, ?SERVER}, ?MODULE, []).

init([]) ->
    Server = {manager_server, {manager_server, start_link, []},
	      permanent, 2000, worker, [manager_server]},
    Children = [Server],
    RestartStrategy = {one_for_one, 3, 1},
    {ok, {RestartStrategy, Children}}.
