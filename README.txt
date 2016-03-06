================= Book Loan Management System =================
- How to run?
	In server directory, compile the UDPEchoServer.c, you will get a file called 'a.out'. 
	Run it with a port number as parameter, for example: "~yourname$ ./a.out 2333".

	In client directory, compile the UDPEchoClient.c, you will get a file called 'a.out'.
	Run it with IP address and port number, for example: "~yourname$ ./a.out 127.0.0.1 2333".

- What do you expect?
	Once start the server, the server will list all books found in book_db.txt and wait for client request.
	When user try to login, the server will check user_db.txt to see if the login information is valid or not.
	For each request, server will list request type and the requesing user id.
	After five times of records modification, the server will write the records back into book_db.txt, and then read from it again.

	Once start the client, user can choose to login or exit program.
	Once login, user has options of query, borrow, return, and logout.
	Each successful query, borrow, and return request will show user the detail information of the book.
	Failed operation will show user the failure reason.

- Components
    \-Project
	  \-Server
		 -Server source file
		 -book_db.txt
		 -user_db.txt
	  \-Client
	  	 -Client source file
