
module.exports = function() {
  return new f2048();
};

//init
var f2048 = function(){
	this.map=[0,0,0,0,
			  0,0,0,0,
			  0,0,0,0,
			  0,0,0,0];
	this.history=[];
	this.capacity=16;
	random_brick(this);
	random_brick(this);
};

//show the content of this.map
f2048.prototype.show = function(){
	var text="";
	for(x=0;x<4;x++){
		for(y=0;y<4;y++){
			for(i=5-this.map[4*x+y].toString().length;i>0;i--){
				text+=" ";
			}
			text+=this.map[4*x+y];
		}
		text+="\n";
	}
	console.log(text);
};

//move action
f2048.prototype.moveUp = function(){
	return move("up",this);
};

f2048.prototype.moveDown = function(){
	return move("down",this);
};

f2048.prototype.moveRight = function(){
	return move("right",this);
};

f2048.prototype.moveLeft = function(){
	return move("left",this);
};

//undo
f2048.prototype.unDo = function(){
	if(this.history.length>0){
		var count=0;
		var arr = this.history.pop().split(",");
		for(i=0;i<16;i++){
			if(parseInt(arr[i])==0)count++;
			this.map[i]=parseInt(arr[i]);
		}
		this.capacity=count;
		return true;
	}
	else return false;
};

/*
*  All following functions are private. You can not directly call them without f2048 object
*/

// random integer form <low> to <high>
var random_int = function(low,high){
	return Math.floor(low+(high-low+1)*Math.random());
};

// random nummber of a initial brick
var random_num =function(){
	return random_int(0,1)==0? 2:4;
};

//create new brick in random if there have any free spaces
var random_brick =function(obj){
	if(obj.capacity>0){
		var index=random_int(0,obj.capacity-1);
		for(i=0;i<16;i++){
			if(index==0&&obj.map[i]==0){
				obj.map[i]=random_num();
				obj.capacity--;
				break;
			}
			else if(index>0&&obj.map[i]==0) index--;
			else;
		}
	}
};

//move action
var move = function(direc,obj){
	obj.history.push(obj.map.toString());
	if(direc=="up"||direc=="left"){
		for(x=0; x<4;x++)
			for(y=0;y<4;y++)
				if(obj.map[4*x+y]!=0) Merge(x,y,direc,obj);
	}
	else if(direc=="down"||direc=="right"){
		for(x=3; x>=0;x--)
			for(y=3;y>=0;y--)
				if(obj.map[4*x+y]!=0) Merge(x,y,direc,obj);
	}
	
	Shuffle(direc,obj);
	//check if the move works
	if(obj.map.toString() != obj.history[obj.history.length -1]){
		random_brick(obj);
		return true;
	}
	else {
		obj.unDo();
		return false;
	}
};

//merge bricks
var Merge = function(x,y,direc,obj){
	var index=4*x+y;
	if(direc=="up"){
		for(i=4*(x+1)+y;i<16;i+=4){
			if(obj.map[index]==obj.map[i]){
				obj.map[index]*=2;
				obj.map[i]=0;
				obj.capacity++;
				break;
			}
			else if(obj.map[i]==0) continue;
			else break;
		}
	}
	else if(direc=="down"){
		for(i=4*(x-1)+y;i>=0;i-=4){
			if(obj.map[index]==obj.map[i]){
				obj.map[index]*=2;
				obj.map[i]=0;
				obj.capacity++;
				break;
			}
			else if(obj.map[i]==0) continue;
			else break;
		}
	}
	else if(direc=="right"){
		for(i=4*x+y-1;i>=4*x;i--){
			if(obj.map[index]==obj.map[i]){
				obj.map[index]*=2;
				obj.map[i]=0;
				obj.capacity++;
				break;
			}
			else if(obj.map[i]==0) continue;
			else break;
		}
	}
	else if(direc=="left"){
		for(i=4*x+y+1;i<4*(x+1);i++){
			if(obj.map[index]==obj.map[i]){
				obj.map[index]*=2;
				obj.map[i]=0;
				obj.capacity++;
				break;
			}
			else if(obj.map[i]==0) continue;
			else break;
		}
	}
	else ;	
};

//shuffle bricks
var Shuffle=function(direc,obj){
	if(direc=="up"){
		for(i=0; i<16;i++)
			if(obj.map[i]==0)
				for(j=i+4;j<16;j+=4)
					if(obj.map[j]!=0){
						obj.map[i]=obj.map[j];
						obj.map[j]=0;
						break;
					}
	}
	else if(direc=="down"){
		for(i=15; i>=0;i--)
			if(obj.map[i]==0)
				for(j=i-4;j>=0;j-=4)
					if(obj.map[j]!=0){
						obj.map[i]=obj.map[j];
						obj.map[j]=0;
						break;
					}
	}
	else if(direc=="right"){
		for(i=15; i>=0;i--)
			if(obj.map[i]==0)
				for(j=i-1;j>=(Math.floor(i/4))*4;j--)
					if(obj.map[j]!=0){
						obj.map[i]=obj.map[j];
						obj.map[j]=0;
						break;
					}
	}
	else if(direc=="left"){
		for(i=0; i<16;i++)
			if(obj.map[i]==0)
				for(j=i+1;j<(Math.floor(i/4)+1)*4;j++)
					if(obj.map[j]!=0){
						obj.map[i]=obj.map[j];
						obj.map[j]=0;
						break;
					}
	}
	else;
};

