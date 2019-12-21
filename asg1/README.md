make to complie the file
open a terminal to run httpserver by type ./httpserver
open another terminal to get and put the file by type the command

command 

	curl -T file http://localhost:8080 --request-target filename_27_character
	put a file on the server

	curl http://localhost:8080 --request-target filename_27_character
	get a file from the server