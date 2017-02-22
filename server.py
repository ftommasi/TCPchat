import socket
import sys
import time

probeNum = -1
delay = 0

def processRequest(request):
  global probeNum
  global delay
  ok = "200 OK: Ready"
  bad = "404 ERROR: Invalid Connection Setup Message"
  invalidMeasure = "404 ERROR: Invalid Measurement Message" 
  if request[len(request)-1] != "\n":
    return bad
  

  if request == "t\n":
    return ok
  
  request  = request[0:len(request)-2]

  params = request.split()
  if params[0] == 's':
    #verify CSP string
    if len(params) < 5:
      print "bad param num"
      return bad
    
    if params[1] != "rtt" and params[1] != "tput":
      print "bad rtt or tput command", params[1]
      return bad

    for param in params[2:]:
      try:
        if int(param) < 0:
          print "negatie value in param", param
          return bad
      except ValueError:
        print "value error in param", param
        return bad
    delay = int(params[4])
    return ok

  elif params[0] == 'm':
    #verify msp
    try:
      if int(params[1]) - 1 != probeNum:
        print "dropped packed. Prev:", probeNum, "I got", params[1]
        return invalidMeasure
      probeNum += 1

      

    except ValueError:
      print "value error in param", params[1]
      return invalidMeasure
    
    time.sleep(delay)
    
    return ok

  
  else:
    print "bad first char", params[0]
    return bad




sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = (sys.argv[1], int(sys.argv[2]))
sock.bind(server_address)
sock.listen(1)

while True:
  connection, client_address = sock.accept()
  print "Client Connected"
  CSPdone = False
  measurementDone = False
  while not CSPdone:
    data = connection.recv(33000)
    if data:
      if data == "t\n":
        CSPdone = True
      print data
      result = processRequest(data)
      connection.sendall(result + "|" + data)
      #connection.sendall(data)
   
  connection.sendall("200 OK: Closing Connection")
  connection.close()
  probeNum = -1
  delay = 0




