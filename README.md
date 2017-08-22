[![Build Status](https://travis-ci.org/minghui-liu/Tin-web-server.svg?branch=master)](https://travis-ci.org/minghui-liu/Tin-web-server)

# A lightweight web server written in C with some goals in mind.
* Fast and responsive
* Ability to handle large number of concurrent requests
* Support Python frameworks well

## Compilation
```
make
```
or
```
gcc tin.c -o tin -pthread
```

## Usage
Put index.html (default homepage), other pages and resources under the same directory. Then run:
```
./tin [port]
```
You will see requests coming in.

## Dynamic content
Put your CGI binary under cgi-bin directory and access it using
```
http://[your domain/ip address]:[port]/cgi-bin/[program]?[parameter1]&[parameter2]&[...]
```
For example, to run the adder cgi program that comes with Tin, go to:
```
http://[your domain/ip address]:[port]/cgi-bin/adder?1+2
```

## Contribute
Please consider contributing to our project and learn with us along the way!
