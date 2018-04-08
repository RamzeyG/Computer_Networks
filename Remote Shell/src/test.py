#         Ramzey Ghanaim
#             CMPE 156
#               Lab1
#              test.py
# The purpose of this lab is to test the server and client
# programs I created by acting as a "bot" that starts both
# programs, and sends commands to the client's input as
# a user would do. the "bot" then verifies the result sent 
# back to the client from the server mathes the expected
# result. If the result does not match what is expected,
# the program tells the user and quits the server, client
# and itself

import os
import subprocess
import pexpect
import sys

#get the current directory to test pwd
cwd = os.getcwd()

#start up the server
proc1 = subprocess.Popen(['./myserver', '1234'])

#start the client
client = pexpect.spawn('./myclient 127.0.0.1 1234')

#we first expect "client $" to be displayed
try:
	x = client.expect('\nclient \$ ',timeout=5) #"client $" should be dispayed within 5 seconds
except:									  #error if not proper response from server
	if x == 0: 
		print("Error: did not get prmpt\n")
	if x == 1:
		print("Error: timeout\n")
	client.kill(0)
	proc1.kill(0)
	sys.exit()

#create a newfile called "newfile" and fill it with 
#the string "hello"
client.sendline('echo hello > newfile')

#display the contents of the new file
client.sendline('cat newfile')

#verify the file was successfully made and contains "hello"
try:
	i = client.expect('hello', timeout=5) #timeout for 5 seconds
except: #Exit if hello is not the response from server
	if i == 0:
		print("ERROR: did not get \"hello\" from newfile");
	if i == 1:
		print("Error: timout on cat newfile");
	client.kill(0)
	proc1.kill(0)
	sys.exit()
#remove the file
client.sendline('rm newfile')

#display the current directory
client.sendline('pwd')

#verify if it is correct
try:
	z = client.expect(cwd, timeout=5)
except:
	if z == 0:
		print("ERROR: didn't get right output for command: pwd\n")
	if z ==1:
		print("Error: timout on pwd  command\n");
	print(client.before) #print everyting previous
	client.kill(0)
	proc1.kill(0)
	sys.exit()

#print client.before
#client.interact()
#we are done, exxit the client
client.sendline('exit')

#if we get to here, all the commands worked
print("All Commands were successful")

#server process is still running so kill it
os.system('killall myserver')
