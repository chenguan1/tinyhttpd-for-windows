#!python.exe
#encoding:gb2312
#import cgi, cgitb

print "HTTP/1.0 200 OK"
print "Content-type:text/html"
print
print "<html>"
print "<head><title>Hello, python cgi</title></head>"
name = "magictong"
print "<body><h1>hello, i am ", name
print "</h1></body>"
print "</html>"