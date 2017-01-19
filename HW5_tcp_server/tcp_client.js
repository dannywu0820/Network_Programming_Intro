var fs = require('fs');
var file_path = process.argv[2];
var server_host = process.argv[3]|| "127.0.0.1";
var server_port = process.argv[4]|| 5566;
//var client = 
var clients=[]; //handle all users' udp connection 

var getIndexbyPort = function(port){
	for(var i=0;i<clients.length;i++)
		if(port == clients[i].socket.address().port) return i;
	return false;
}

var getIndexbyName = function(name){
	for(var i=0;i<clients.length;i++)
		if(name == clients[i].name) return i;
	return false;
}

var BuildConnection = function(commands){
	try{
		var client_name = commands.init.pop();
		if(client_name){
			//build socket. push to clients
			var client_socket = new require('net').Socket();
			
			client_socket.connect( server_port, server_host);
			clients.push({ "name" : client_name , "socket" : client_socket});
			
			BuildConnection(commands);
			
			client_socket.on('data', function(message) {
				process.stdout.write(message.toString());
				
				//continue process commands
				command_processing(commands);
			});
			
			client_socket.on('end', function() {});
			
			
		}
		else command_processing(commands);
	}
	catch(e){
		console.log(e);
	}
};

//porcessing commands
var command_processing = function(commands){
	try{
		var row = commands.rows.pop();
		if(row){
			var index = getIndexbyName(row.client);
			console.log(clients[index].name+"> "+row.action);
			clients[index].socket.write(row.action);
		}
	}
	catch(e){
		console.log(e);
	}
};

//check file_path
fs.access(file_path, fs.R_OK, function (err) {
	if(err) console.log('wrong file path');
	else{
		//read file
		var commands =  JSON.parse(fs.readFileSync(file_path).toString());
		commands.init.reverse();
		commands.rows.reverse();
		
		//build connection 
		BuildConnection(commands);
	}
});