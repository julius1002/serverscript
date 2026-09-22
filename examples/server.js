import { init_server, add_mapping, launch_server } from "server";

function my_handler(req, res) {
	if(req.method == "GET") {
		var result = { 
						"id": 10, "name": "Max", "age": 25,
			            "lastname": "Mustermann"
					 };

		res.body = JSON.stringify(result);
	} else if(req.method == "POST") {
		console.log(req.body);
		console.log("^^was posted");
		res.body = "You posted";
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
