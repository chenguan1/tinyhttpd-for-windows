#!python.exe
#encoding:gb2312
#import cgi, cgitb

import sys
line = sys.stdin.readline()
print "HTTP/1.0 200 OK"
print "Content-type:text/html"
print
print "<html>"
print "<head><title>Hello, python cgi</title></head>"
print "<body><h1>you commit: ", line
print "</h1></body>"
print "</html>"