{application, manager,
 [{description, "Sample Erlang OTP Application"},
  {vsn, "0.1.0"},
  {modules, [	manager_app,
		manager_sup,
		manager_server]},
  {registered, [manager_sup]},
  {applications, [kernel, stdlib]},
  {mod, {manager_app, []}}
  ]}.
