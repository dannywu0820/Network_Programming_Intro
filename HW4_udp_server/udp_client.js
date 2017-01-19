var fs = require('fs');
var dgram = require('dgram');
var file_path = process.argv[2];
var server_host = process.argv[3]|| "140.113.235.131";
var server_port = process.argv[4]|| 5566;
var accounts=[]; //handle all users' udp connection 
var commands=[]; //handle all commands in file
var count=1;

var send_to_server =function (message,host,port,socket){
	socket.send(message, 0, message.length, port, host, function(err, bytes) {
		if (err) console.log(err);
	});
};

var getIndex = function (account_name){
	for(var i=0 ; i< accounts.length;i++){
		if(account_name ==  accounts[i].account_name) return i;
	}
	return false;
};

var closeAllSockets = function(){
	for(var i=0 ; i< accounts.length;i++) accounts[i].socket.close();
	//console.log("all accounts closed");
};

//porcessing commands
var command_processing = function(){
	try{
		var command = commands.pop();
		var message ={}; // message that sent to server
		
		console.log((count++)+". "+command); //show the command
		command = command.split(" ");
		
		if(command[1]=="init"){
			var socket = dgram.createSocket('udp4');
			socket.on('message', function(msg, remote){
				try{
					var response= JSON.parse(msg.toString());
					if(response.message!=="ok"){
						if(response.message==="end") closeAllSockets();
						else console.log(response.message); 
					}
					if(response.message!="end") command_processing(); //execute next command
				}
				catch(e){
					console.log(e);
				}
			});
			message.account_name = command[0];
			message.action = command[1];
			message.account_id = command[2];
			accounts.push({"account_name":command[0],"account_id": command[2], "socket": socket});
		}
		else if(command[1]=="save"){
			message.action = command[1];
			message.money = parseInt(command[2]);
		}
		else if(command[1]=="withdraw"){
			message.action = command[1];
			message.money = parseInt(command[2]);
		}
		else if(command[1]=="remit"){
			message.action = command[1];
			message.money = parseInt(command[2]);
			message.destination_name = command[3];
		}
		else if(command[1]=="show") message.action = command[1];
		else if(command[0]=="bomb") throw "bomb";
		else if(command[0]=="end") throw "end"; 
		else throw "wrong command";
		
		var account_index  = getIndex(command[0]);	
		if(account_index!==false) send_to_server(JSON.stringify(message),server_host,server_port,accounts[account_index].socket);
		else throw"no account";
	}
	catch(e){
		if(e=="end"){
			var message ={"action" : "end"};
			send_to_server(JSON.stringify(message),server_host,server_port,accounts[0].socket);
		}
		else if(e=="bomb"){
			var message ={"action" : "bomb"};
			send_to_server(JSON.stringify(message),server_host,server_port,accounts[0].socket);
		}
		else console.log(e);
	}
};

//check file_path
fs.access(file_path, fs.R_OK, function (err) {
	if(err) console.log('wrong file path');
	else{
		//read file
		commands = fs.readFileSync(file_path).toString().replace(/\r/g,' ').split("\n");
		
		commands.pop();
		commands = commands.reverse();
		//process commands
		command_processing();
	}
});






