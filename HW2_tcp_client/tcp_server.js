var HOST = process.argv[2] || '140.113.65.28';
var PORT = process.argv[3] || 5566;
var net = require('net');
var f2048= require("./f2048");

var server = net.createServer();

server.on('connection',  function(socket) {
	console.log("connect "+socket.remoteAddress+":"+socket.remotePort+" "+Date());
	var game = false; //2048 game object 
	var cheat=false;
	//catch data
	socket.on('data', function(message){
		// response object
		var response={"status" : 1, "message": null};
		try{
			//input error_check
			var message = message.toString().replace(/\'/g,'"').replace(/\;/g,' ').replace(/\0/g,' ');
			console.log("receive: "+message.toString());
			
			//input parse
			var request= JSON.parse(message.toString());
	
			//do action
			if(request.action=="New") game = f2048();
			else if(typeof(game) !="object" ) throw("Could not find the game"); //check if initialize the game 2048
			else if(request.action=="End") throw("end");
			else if(request.action=="moveUp") response.status = game.moveUp();
			else if(request.action=="moveDown") response.status = game.moveDown();
			else if(request.action=="moveLeft") response.status = game.moveLeft();
			else if(request.action=="moveRight") response.status = game.moveRight();
			else if(request.action=="unDo") response.status = game.unDo();
			else if(request.action=="whosyourdaddy") cheat=true;
			else throw("Wrong JSON content");
			
			//check action's status, 0:fail 1:success 
			if(response.status){
				response.status=1;
				if(cheat){
					cheat=false;
					response.message="2048,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
				}
				else response.message=game.map.toString();
			}
			else {
				response.status=0;
				response.message="Game not change";
			}
		}
		catch(e){
			if(e=="end"){
				game = false;
				response.message="The game has closed";
			}
			else{	
				console.log(e);
				response.status=0;
				response.message = 'error: '+e;
			}
		}
		//send response back
		socket.write(JSON.stringify(response));
	});
	
	//disconnection
	socket.on('end', function() {
		console.log("disconnect "+socket.remoteAddress+":"+socket.remotePort+" "+Date());
	});
});

server.listen(PORT, HOST);
console.log('TCP server listening on ' + HOST + ":" +PORT);
