# kuraianto

## Usage

Compile with ``make``

> **kuraianto** is reading from stdin.
> e.g: ``cat examples/GET_simple.req | ./kuraianto 8000``

```
usage: [ip:]port [options]
Options:
	range: value | min-max | -max
	-r range=5		Set packet size when receiving.
	-s range=5		Set packet size when sending.
	-t number=5		Maximum time to wait (s) if there is no response.
	-i range=0		Set interval (ms) between sending and receiving.
	-m number=0		Maximum size (bytes) the client can send.
	--no-output		Do not display received response
```