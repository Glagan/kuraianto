# kuraianto

## Usage

Compile with ``make``

> **kuraianto** is reading from stdin.
> e.g: ``cat examples/GET_simple.req | ./kuraianto 8000``

```
usage: [ip:]port [options]
Options:
	range: value | min-max | -max
	request: [Type,uri,[Header-Name: value,]*bodySize;]+
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

## Generate requests

You can generate *Requests* in the command line by passing the ``-g`` options.  
Requests are build with the following syntax:

```bash
Type,uri,[Header-Name: value,]*bodySize
# e.g POST,/upload,Transfer-Encoding: chunked,X-Token: token,4242
```

Headers added with ``-h`` are added to all generated requests.

If the ``Transfer-Encoding`` header is set to ``chunked`` **kuraianto** will send chunks of the size of your ``-c`` option.

## TODO

* Generate requests
* ``-a``
	* Read and send all files given as the value (e.g: a folder ``examples`` or list of files ``examples/1.req,examples/2.req``)
