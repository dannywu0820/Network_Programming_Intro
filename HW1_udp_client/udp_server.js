var HOST = process.argv[2] || '127.0.0.1';
var PORT = process.argv[3] || 5566;
var answer = process.argv[4] || 56666;

var S1 = require('dgram').createSocket('udp4');
var S2 =  require('dgram').createSocket('udp4');


//S1 part 
S1.on('listening', function () {
    var address = this.address();
    console.log('S1 listening on ' + address.address + ":" + address.port);
});

S1.on('message', function (message, remote) {
	console.log(remote.address + ":" + remote.port +" "+Date());
	var response={"result" : null};
	try{
		//input error_check
		var message = message.toString().replace(/\'/g,'"').replace(/\;/g,' ').replace(/\0/g,' ');
		console.log("S1 receive: "+message);
		//input parse
		var request= JSON.parse(message);
		//compare
		if(request.guess && ! isNaN(request.guess)) response.result = (answer==request.guess) ? 'bingo!' : (answer > request.guess) ? 'larger' : 'smaller' ; 
		else response.result = 'Wrong JSON content';
	}
	catch(e){
		console.log(e);
		response.result = 'parse_error: '+e;
	}
	//send response
	message = JSON.stringify(response);
	console.log("S1 send back: "+message);
	this.send(message, 0, message.length, remote.port, remote.address, function(err) {
		if (err) throw err;
	});	
});

S1.bind(PORT, HOST);


//S2 part
S2.on('listening', function () {
    var address = this.address();
    console.log('S2 listening on ' + address.address + ":" + address.port);
});

S2.on('message', function (message, remote) {
	var response={"result" : null};
	try{
		//input error_check
		var message = message.toString().replace(/\'/g,'"').replace(/\;/g,' ').replace(/\0/g,' ');
		console.log("S2 receive: "+message);
		//input parse
		var request= JSON.parse(message.toString());
		//input check
		if(request.student_id) response.result = "Congrats! "+ request.student_id;
		else response.result = 'Wrong JSON content';
	}
	catch(e){
		console.log(e);
		response.result = 'parse_error: '+e;
	}
	
	//send response
	var message = JSON.stringify(response);
	console.log("S2 send back: "+ message);
	this.send(message, 0, message.length, remote.port, remote.address, function(err) {if (err) throw err; });	
});

S2.bind(answer, HOST);