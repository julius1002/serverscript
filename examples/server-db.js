import { init_server, add_mapping, launch_server } from "server";
import { init_postgres, query } from "postgres";

function query_handler(req, res) {
	if(req.method == "POST") {
		const query_obj = JSON.parse(req.body);
		const result = query(
				"SELECT * FROM APPUSER WHERE username = $1 AND email = $2", query_obj.username, query_obj.email
				);
		res.body = JSON.stringify(result);
	} else {
		res.status = 400;
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


init_postgres("host=localhost port=5432 dbname=FILL user=FILL");

const port = "3000";

init_server(port, "./public");
add_mapping("/query", query_handler);
launch_server();
