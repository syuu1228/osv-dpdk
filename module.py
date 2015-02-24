from osv.modules import api

default = api.run("/l2fwd --no-shconf -c f -n 2 --log-level 8 -m 1073741824 -- -p 3")
