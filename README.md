# kuraianto

## Usage

Compile with a simple ``make``

```
usage: [options] [ip:]port [...files]
Options:
	range: value | min-max | -max
	request: [Type,uri,[Header-Name: value,]*bodySize;[repeat;]]+
	headers: [Name: value#]+
	-r range=5		Set packet size when receiving.
	-s range=5		Set packet size when sending.
	-t number=5		Maximum time to wait (s) if there is no response.
	-i range=0		Set interval (ms) between sending and receiving.
	-m number=0		Maximum size (bytes) the client can send.
	--no-output		Do not display received response.
	-g requests		Generate one or many requests.
	The next options are only applied on requests generated with -g
	-h headers		Add each listed headers to the generated requests.
	-c range=8		Set the size of sent chunk if the generated request has ``Transfer-Encoding: chunked``
```

> **kuraianto** is reading from stdin if no files are specified.  
> ``cat examples/GET_simple.req | ./kuraianto 8000`` is equivalent to ``./kuraianto 8000 examples/GET_simple.req``

## Generate requests

You can generate *Requests* in the command line by passing the ``-g`` options.  
Requests are build with the following syntax:

```bash
Type,uri,[Header-Name: value,]*bodySize[;repeat]
# A simple GET Request to index.html
#	GET,/index.html,0
# 2 HEAD and 1 GET Request to / and 4 POST Requests to /upload with an Header and a bodysize of 4242 bytes
#	HEAD,/,0;2;GET,/index,0;POST,/upload,Transfer-Encoding: chunked,4242;4
# 3 POST Requests to /upload with Headers and a bodysize of 424242 bytes
#	POST,/upload,Transfer-Encoding: chunked,X-Token: token,424242;3
```

The ``repeat`` parameter duplicate the previous request *x* amount of times.  
Headers added with ``-h`` are added to all generated requests.  
If the ``Transfer-Encoding`` header is set to ``chunked`` **kuraianto** will send chunks of the size of your ``-c`` option.

## TODO

* Avoid displaying message if the previous one was really close
