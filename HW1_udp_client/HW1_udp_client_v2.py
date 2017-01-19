import socket 
import sys #exit

sock=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

l=3000
r=60000
m=(l+r)/2;
count=1

while 1:
    #print '#' + str(count)
    #count = count+1
    msg = '{"guess":' + str(m) + '}'
    print 'send' + msg
    sock.sendto(msg,('140.113.65.28',5566))
    recvline=sock.recvfrom(128)
    result = recvline[0].split(':')[1].split('"')[1]
    print 'receive' + recvline[0]
    if result == 'bingo!': #if recvline[0].split(':')[1].split('"')[1].find('bingo!') != -1:
        break
    elif result == 'larger': #elif recvline[0].split(':')[1].split('"')[1].find('larger') != -1:
        l = m + 1
        m = (l + r) / 2
    elif result == 'smaller': #elif recvline[0].split(':')[1].split('"')[1].find('smaller') != -1:
        r = m - 1
        m = (l + r) / 2
sock2=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
print 'send{"student_id":"0116233"}'
sock.sendto('{"student_id":"0116233"}',('140.113.65.28',m))
print 'receive' + sock.recvfrom(128)[0]
