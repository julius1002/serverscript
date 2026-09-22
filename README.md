# serverscript: fork of ⚡️ QuickJS - A mighty JavaScript engine

## Overview

This project is a _fork_ of the _fork_ quickjs-ng the [original QuickJS project] by Fabrice Bellard and Charlie Gordon.

The purpose of this project is to embed webframework capabilities directly into the language.

## Example

If you run examples/server.js it runs a a very basic webserver and you can define handlers directly without any dependencies.
The webserver is actually the quite popular civetweb webserver (https://github.com/civetweb/civetweb).

Code looks like this:
```
import { init_server, add_mapping, launch_server } from "server";

function my_handler(req, res) {
	if(req.method == "GET") {
		var result = { 
						"id": 10, "name": "Max", "age": 25,
			            "lastname": "Mustermann"
					 };

		res.body = JSON.stringify(result);
	} else {
		res.status = 404;
		res.reason = "Bad Request";
		return;
	}

	const header = { name: "Authorization", value: "Icandoanything"};
	const header2 = { name: "Cookieeees", value: "yes"};
	res.headers = [];
	res.headers.push(header);
	res.headers.push(header2);
	res.status = 200;
}


const port = "3000";

init_server(port, "./public");
add_mapping("/greetings", my_handler);
launch_server();
```

## Future work

I plan to implement also database capabilities. Though right now, this language is just meant for prototyping and not for productive usage.
