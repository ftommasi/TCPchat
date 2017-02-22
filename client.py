import socket
import sys
import time

def buildCSP(mtype,probes,msgsize,serverdelay):
  request = "s "
  request += str(mtype) + " " 
  request += str(probes) + " " 
  request += str(msgsize) + " " 
  request += str(serverdelay) + "\n"
  return request




sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = (sys.argv[1],int(sys.argv[2]))
sock.connect(server_address)
print "Connected to Server"
CSPdone = False
measureDone = False
message = ""
size = ""
probes = ""
while not CSPdone:
  print "usage <mtype> <probes> <payload> <delay>"
  message = raw_input()
  tokens = message.split()
  
  mtype = tokens[0]
  probes = tokens[1]
  size = tokens[2]
  serverdelay = tokens[3]

  request = buildCSP(mtype, probes, size, serverdelay)
  sock.sendall(request+"\n")
  #responsecode = sock.recv(256)
  data = sock.recv(33000)
  temp = data.split("|")
  resp = temp[0]
  data = temp[1]
  print  data
  if resp == "200 OK: Ready":
    CSPdone = True 

startMeasure = time.time()
transTimes = []
transTput = []
for i in range(int(probes)):
  startTrans = time.time()
  message = "m " + str(i) + " " + int(size) * "f"
  sock.sendall(message)
  #responsecode = sock.recv(256)
  data = sock.recv(33000)
  temp = data.split("|")
  resp = temp[0]
  data = temp[1]
  print  data
  
  if resp == "404 ERROR: Invalud Measurement Message":
    terminate = "t\n"
    sock.sendall(terminate)
    sock.close()
    break
  endTrans = time.time()
  transTimes.append((endTrans - startTrans))
  transTput.append(len(message)/(endTrans - startTrans))

endMeasure = time.time()


print "total time:", sum(transTimes)
print "total bytes sent:", sum(transTput)

rtt = sum(transTimes)/(int(probes))
print "rtt:", rtt

tput = sum(transTput)/(int(probes))
print "tput:", tput

terminate = "t\n"
sock.sendall(terminate)
termResp = sock.recv(256)
#termEcho = sock.recv(256)
while termResp != "200 OK: Closing Connection":
  sock.sendall(terminate)
  termResp = sock.recv(256)
  #termEcho = sock.recv(256)
sock.close()

