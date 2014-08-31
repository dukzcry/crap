-module(erl_make).

-export([make/1]).

post_process(_) -> ok.

make(Mode) ->
    case make:all([{d, Mode}]) of
    	error -> error;
	_ -> post_process(Mode)
    end.
