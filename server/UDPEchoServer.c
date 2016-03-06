#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <math.h>

#define ECHOMAX 255     /* Longest string to echo */

typedef struct{
    unsigned int requestID;
    unsigned int userID;
    unsigned int  password;
    enum{Login, Logout, Query, Borrow, Return} requestType;
    char isbn[13];
} ClientMessage;

typedef struct{
    unsigned int requestID;
    unsigned int userID;
    unsigned int password;
    enum {Okay, ISBNError, AllGone, NoInventory, InvalidLogin} respType;
    char isbn[13];
    char authors[100];
    char title[100];
    unsigned int edition;
    unsigned int year;
    char publisher[100];
    unsigned int inventory;
    unsigned int available;
} ServerMessage;


void DieWithError(char *errorMessage);  /* External error handling function */
int isISBNValid(char isbn[]);
int findBook(char isbn[], char isbn_list[][13], int record_number);
int isValidLogin(int user_id, int user_password);
int ifLoggedIn(int user_id, int current_user[]);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    int modify_time = 0;
    int current_user[100];       // max concurrent user 99

//======================================================================
//  Prepare socket

    if (argc != 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    
    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    
    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */
    
    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

//=========================================================================
//  Read books from file

	FILE * book_db_file;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int record_number = 0;

    book_db_file = fopen("book_db.txt", "r+");
    if (book_db_file == NULL)
        DieWithError("Book Database not found!");

    while ((read = getline(&line, &len, book_db_file)) != -1)  // count the number of books in the database file
    {
    	record_number++;
    }
	printf("number of books: %d\n", record_number);
	fclose(book_db_file);

//===============================================
//  Declare lists that will be used to store book info

	char isbn_list[record_number][13];
	char authors_list[record_number][100];
	char title_list[record_number][100];
	unsigned int edition_digit_list[record_number][2];  // assume the edition number is under 2 digit (0 ~ 99)
	unsigned int edition_list[record_number];

	unsigned int year_digit_list[record_number][4];
	unsigned int year_list[record_number];

	char publisher_list[record_number][100];

	unsigned int inventory_digit_list[record_number][2];  // assume inventory is under 2 digit (0 ~ 99)
	unsigned int inventory_list[record_number];

	unsigned int available_digit_list[record_number][2];  // assume available is under 2 digit (0 ~ 99)
	unsigned int available_list[record_number];

//================================================
//  Initialize lists
//  For every text list, initialize everything to '|', which represents empty charactor.
//  For every int list, initialize every digit to -1, which means 'no digit'.

	for(int j = 0; j < record_number; j++)  
    {
    	for(int k = 0 ; k < 100; k++)
    	{
    		authors_list[j][k] = '|';     
    	}

    	for(int k = 0 ; k < 100; k++)
    	{
    		title_list[j][k] = '|';
    	}
	
		edition_digit_list[j][0] = -1;
		edition_digit_list[j][1] = -1;

		year_digit_list[j][0] = -1;
		year_digit_list[j][1] = -1;
		year_digit_list[j][2] = -1;
		year_digit_list[j][3] = -1;

    	for(int k = 0 ; k < 100; k++)
    	{
    		publisher_list[j][k] = '|';
    	}

    	inventory_digit_list[j][0] = -1;
    	inventory_digit_list[j][1] = -1;

    	available_digit_list[j][0] = -1;
    	available_digit_list[j][1] = -1;
	}

//====================================================
//  Read book info in

    book_db_file = fopen("book_db.txt", "r+");

    for(int i = 0; i < record_number; i++)   // read line by line
    {
    	read = getline(&line, &len, book_db_file);   
    	//printf("len: %zu \n", len);
        //printf("Retrieved line of length %zu :\n", read);
        //printf("%s", line);

    	int len = strlen(line);         // line length 
    	int index = 0;                  // which column is being read
    	int head = 0;                   // the start index the current column
    	for (int j = 0; j < len; j++)   // travers a line
    	{  
    		if(line[j] == '|')     // '|' indicate the start of a new column
    		{
    			index++;				
    			head = j + 1;	   // The start index of a column is the charactor after '|'
    		}
    		else
    		{
		    	switch(index) 
			    {
			        case 1 :	// column 1, ISBN
			            isbn_list[i][j - head] = line[j];
			            break;
			        
			        case 2 :	// column 2, authors
			        	authors_list[i][j - head] = line[j];
			            break;

			        case 3 :	// column 3, title
			            title_list[i][j - head] = line[j];
			            break;

			        case 4 :	// column 4, edition
			            edition_digit_list[i][j - head] = line[j] - '0';
			            break;

			        case 5 :	// column 5, year
			            year_digit_list[i][j - head] = line[j] - '0';
			            break;

	        		case 6 :	// column 6, publisher
			            publisher_list[i][j - head] = line[j];
			            break;

			        case 7 :	// column 7, inentory
			            inventory_digit_list[i][j - head] = line[j] - '0';
			            break;

			        case 8 :	// column 8, available
			            available_digit_list[i][j - head] = line[j] - '0';
			            break;

			        default :
			        ;
		    	}
		    }
    	}  
    	//memset(isbn, 0, sizeof(isbn));
    }
	free(line);    // release heap

//=======================================================
//  List database content loaded into server's memory

    for(int j = 0; j < record_number; j++)
    {
    	for(int k = 0 ; k < 13; k++)
    	{
    		printf("%c",isbn_list[j][k]);
    	}
    	printf(" | ");


    	for(int k = 0 ; k < 100; k++)
    	{
    		if(authors_list[j][k] != '|')
    		{
    			printf("%c",authors_list[j][k]);
    		}
    	}
    	printf(" | ");


    	for(int k = 0 ; k < 100; k++)
    	{
    		if(title_list[j][k] != '|')
    		{
    			printf("%c",title_list[j][k]);
    		}
    	}
    	printf(" | ");


    	int edition;
    	if(edition_digit_list[j][1] != -1)
    	{
    		edition = edition_digit_list[j][0] * 10 + edition_digit_list[j][1];
    	}
    	else
    	{
    		edition = edition_digit_list[j][0];
    	}
    	edition_list[j] = edition;
    	printf("edition: %d | ", edition);


    	int year = year_digit_list[j][0] * 1000 + year_digit_list[j][1] * 100 + year_digit_list[j][2] * 10 + year_digit_list[j][3];
    	year_list[j] = year;
    	printf("year: %d | ", year);


    	for(int k = 0 ; k < 100; k++)
    	{
			if(publisher_list[j][k] != '|')
			{
    			printf("%c",publisher_list[j][k]);
			}
    	}
    	printf(" | ");


    	int inventory;
    	if(inventory_digit_list[j][1] != -1)
    	{
    		inventory = inventory_digit_list[j][0] * 10 + inventory_digit_list[j][1];
    	}
    	else
    	{
    		inventory = inventory_digit_list[j][0];
    	}
    	inventory_list[j] = inventory;
    	printf("inventory number: %d | ", inventory);
	

    	int available;
    	if(available_digit_list[j][1] != -1)
    	{
    		available = available_digit_list[j][0] * 10 + available_digit_list[j][1];
    	}
    	else
    	{
    		available = available_digit_list[j][0];
    	}
    	available_list[j] = available;
    	printf("available number: %d\n", available);
	}
	fclose(book_db_file);

//===========================================================
//  initialize login list
    
	for(int i = 0; i< 100; i++)      
	{
		current_user[i] = -1;
	}

	ClientMessage * client_message = malloc(sizeof(ClientMessage));

//==========================================================================
//  Wait for connection

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);
        
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, client_message, sizeof(*client_message), 0,(struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

//==========================================================================
//  initialize the server_message

        int is_login = 0;
        is_login = ifLoggedIn(client_message->userID, current_user);  // check if user already logged in

        ServerMessage server_message;
		memset(&server_message, 0, sizeof(server_message));    /* Zero out structure */
		server_message.requestID = client_message->requestID;
		printf("=========================================\n");
		printf("requestID: %d\n", client_message->requestID);
		server_message.userID = client_message->userID;
		server_message.password = client_message->password;
		server_message.respType = Okay;
		for(int i = 0; i < 13; i++)
		{
			server_message.isbn[i] = '0';
		}

//==============================================================
//	respond

        if(client_message->requestType == Login)
        {
        	if(isValidLogin(client_message->userID, client_message->password) == 1)  // if it's a valid login
        	{
        		server_message.respType = Okay;    // response Okay
        		printf("User <%d> login success.\n", client_message->userID);

        		printf("Current online users: ");    // list current online users
        		for(int i = 0 ; i < 100; i++)
        		{
        			if(current_user[i] == -1)
        			{
        				current_user[i] = client_message->userID;
        				printf(" <%d> \n", current_user[i] );
        				printf("Online user numbers: %d\n", i + 1);
        				break;
        			}
        			else
        			{	
        				printf(" <%d>", current_user[i] );
        			}
        		}
	        	printf("=========================================\n");
        	}
        	else    // if not valid login
        	{
        		server_message.respType = InvalidLogin;  // response invalid login
        		printf("Login attempt with id <%d> failed.\n", client_message->userID);
				printf("Current online users: ");
        		for(int i = 0 ; i < 100; i++)
        		{
	    		    if(current_user[i] == -1)
	    			{
	    				current_user[i] = client_message->userID;
	    				printf("\nOnline user numbers: %d\n", i);
	    				break;
	    			}
	    			else
	    			{	
	    				printf(" <%d> ", current_user[i] );
	    			}
	    		}
	    		printf("=========================================\n");
        	}
        }
        else if(client_message->requestType == Logout)  // if logout
        {
        	int shift = 0;
        	for(int i = 0; i < 99; i++)     // max concurrent user = 99
        	{
        		if(current_user[i] == client_message->userID)
        		{
        			shift = 1;
        		}
        		if(shift == 1)
        		{
        			current_user[i] = current_user[i + 1];  // remove user from the list
        		}
        	}
        	printf("User <%d> logout\n", client_message->userID);
        	printf("Current online users: ");     // list current online users
    		for(int i = 0 ; i < 99; i++)
    		{
    		    if(current_user[i] == -1)
    			{
    				printf("\nOnline user numbers: %d\n", i);
    				break;
    			}
    			else
    			{	
    				printf(" <%d> ", current_user[i] );
    			}
    		}
    		printf("=========================================\n");
        }
        else  // other requests
        {
	    	if(is_login == 1)  // if already logged in
	    	{
	    		if(isISBNValid(client_message->isbn) == 1)   // if requested ISBN is valid
	    		{
	    			int book_index = findBook(client_message->isbn, isbn_list, record_number); // find book id
	    			if( book_index != -1 )  // if requested book does exist
	    			{
	    				printf("Request Book ISBN: ");
	    				for(int i = 0; i < 13; i++)   // prepare response message
	    				{	
	    					server_message.isbn[i] = isbn_list[book_index][i];
	    					printf("%c", server_message.isbn[i]);
	    				}
	    				for(int i = 0; i < 100; i++)
	    				{
	    					server_message.authors[i] = authors_list[book_index][i];
	    					server_message.title[i] = title_list[book_index][i];
	    					server_message.publisher[i] = publisher_list[book_index][i];
	    				}
	    				server_message.edition = edition_list[book_index];
	    				server_message.year = year_list[book_index];
	    				server_message.inventory = inventory_list[book_index];
	    				server_message.available = available_list[book_index];

		    			switch(client_message->requestType) 
				    	{
					        case Query :	
					            printf(" | Query from userID: %d\n", client_message->userID);
					            break;

			    			case Borrow :	
					            printf(" | Borrow from userID: %d", client_message->userID);
					            if(inventory_list[book_index] > 0) // if has inventory
					            {
						            if(available_list[book_index] > 0)  // if has available copy
						            {
						            	printf(" | available number: %d ->", available_list[book_index]);
						            	available_list[book_index] = available_list[book_index] - 1;  // update
						            	printf(" %d\n", available_list[book_index]);
						            	server_message.available = available_list[book_index];

						            	modify_time++;
						            	printf("DB modify times: %d\n", modify_time);
						            }
						            else
						            {
						            	printf(" | available all gone.\n");
						            	server_message.respType = AllGone;
						            }
						        }
						        else
						        {
						        	printf(" | no inventory\n");
						        	server_message.respType = NoInventory;
						        }
					            break;

					        case Return :	
					            printf(" | Return from userID: %d | available number: %d ->", client_message->userID, available_list[book_index]);
					            available_list[book_index] = available_list[book_index] + 1;  // update
					            printf(" %d\n", available_list[book_index]);
					            server_message.available = available_list[book_index];

					            modify_time++;
								printf("DB modify times: %d\n", modify_time);

					            break;

					        default :
					        ;
				    	}
				    }
				    else
				    {
				    	printf("ISBN not found\n");
				    	server_message.respType =ISBNError;
				    }
	    		}
	    		else
	    		{
	    			printf("ISBN not valid\n");
	    			server_message.respType = ISBNError;
	    		}
	    		printf("=========================================\n");
	    	}
	    	else
	    	{
	    		printf("You are not logged in. \n\n\n");
	    	}
	    }

//=========================================================================
//  Send out the server_message as response

		int server_message_length = sizeof(server_message);

        if(sendto(sock, (struct ServerMessage*)&server_message,server_message_length, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != server_message_length)
        {
            DieWithError("sendto() sent a different number of bytes than expected");
        }

        if(modify_time == 5)  // if modify db 5 times
        {
        	FILE * book_db_file;
		    char * line = NULL;
		    size_t len = 0;
		    ssize_t read;

		    book_db_file = fopen("book_db.txt", "w+");
		    if (book_db_file == NULL)
		        DieWithError("Book Database not found!");

		    for(int i = 0; i < record_number; i++)   // write datas back into the db
		    {
		    	fprintf(book_db_file, "|");
		    	for(int j = 0 ; j < 13; j++)
		    	{
		    		fprintf(book_db_file, "%c", isbn_list[i][j]);
		    	}

		    	fprintf(book_db_file, "|");
		    	for(int j = 0; j < 100; j++)
		    	{
		    		if(authors_list[i][j] != '|')
		    		{
		    			fprintf(book_db_file, "%c", authors_list[i][j]);
		    		}
		    		else
		    		{
		    			break;
		    		}
		    	}

		    	fprintf(book_db_file, "|");
		    	for(int j = 0; j < 100; j++)
		    	{
		    		if(title_list[i][j] != '|')
		    		{
		    			fprintf(book_db_file, "%c", title_list[i][j]);
		    		}
		    		else
		    		{
		    			break;
		    		}
		    	}

		    	fprintf(book_db_file, "|");
		    	fprintf(book_db_file, "%d", edition_list[i]);

	    		fprintf(book_db_file, "|");
	    		fprintf(book_db_file, "%d", year_list[i]);

	    		//printf("fff  %d ffff\n", year_digit_list[i][0]);

		    	fprintf(book_db_file, "|");
		    	for(int j = 0; j < 100; j++)
		    	{
		    		if(publisher_list[i][j] != '|')
		    		{
		    			fprintf(book_db_file, "%c", publisher_list[i][j]);
		    		}
		    		else
		    		{
		    			break;
		    		}
		    	}

		    	fprintf(book_db_file, "|");
		    	fprintf(book_db_file, "%d", inventory_list[i]);

		    	fprintf(book_db_file, "|");
		    	fprintf(book_db_file, "%d", available_list[i]);

		    	fprintf(book_db_file, "|\n");
		    }

        	printf("Modify 5 times, write into db.\n");
        	modify_time = 0;
        	fclose(book_db_file);

//========================================================================
//  prepare to read from db again

        	for(int j = 0; j < record_number; j++)  
		    {
		    	for(int k = 0 ; k < 100; k++)
		    	{
		    		authors_list[j][k] = '|';     
		    	}

		    	for(int k = 0 ; k < 100; k++)
		    	{
		    		title_list[j][k] = '|';
		    	}
			
				edition_digit_list[j][0] = -1;
				edition_digit_list[j][1] = -1;

				year_digit_list[j][0] = -1;
				year_digit_list[j][1] = -1;
				year_digit_list[j][2] = -1;
				year_digit_list[j][3] = -1;

		    	for(int k = 0 ; k < 100; k++)
		    	{
		    		publisher_list[j][k] = '|';
		    	}

		    	inventory_digit_list[j][0] = -1;
		    	inventory_digit_list[j][1] = -1;

		    	available_digit_list[j][0] = -1;
		    	available_digit_list[j][1] = -1;
			}

		//====================================================
		//   Read from db again

		    book_db_file = fopen("book_db.txt", "r+");

		    for(int i = 0; i < record_number; i++)   // read line by line
		    {
		    	read = getline(&line, &len, book_db_file);   

		    	int len = strlen(line);         // line length 
		    	int index = 0;                  // which column is being read
		    	int head = 0;                   // the start index the current column
		    	for (int j = 0; j < len; j++)   // travers a line
		    	{  
		    		if(line[j] == '|')     // '|' indicate the start of a new column
		    		{
		    			index++;				
		    			head = j + 1;	   // The start index of a column is the charactor after '|'
		    		}
		    		else
		    		{
				    	switch(index) 
					    {
					        case 1 :	// column 1, ISBN
					            isbn_list[i][j - head] = line[j];
					            break;
					        
					        case 2 :	// column 2, authors
					        	authors_list[i][j - head] = line[j];
					            break;

					        case 3 :	// column 3, title
					            title_list[i][j - head] = line[j];
					            break;

					        case 4 :	// column 4, edition
					            edition_digit_list[i][j - head] = line[j] - '0';
					            break;

					        case 5 :	// column 5, year
					            year_digit_list[i][j - head] = line[j] - '0';
					            break;

			        		case 6 :	// column 6, publisher
					            publisher_list[i][j - head] = line[j];
					            break;

					        case 7 :	// column 7, inentory
					            inventory_digit_list[i][j - head] = line[j] - '0';
					            break;

					        case 8 :	// column 8, available
					            available_digit_list[i][j - head] = line[j] - '0';
					            break;

					        default :
					        ;
				    	}
				    }
		    	}  
		    }
			free(line);    // release heap

		//=======================================================
		//  List database content loaded into server's memory

		    for(int j = 0; j < record_number; j++)
		    {
		    	for(int k = 0 ; k < 13; k++)
		    	{
		    		printf("%c",isbn_list[j][k]);
		    	}
		    	printf(" | ");


		    	for(int k = 0 ; k < 100; k++)
		    	{
		    		if(authors_list[j][k] != '|')
		    		{
		    			printf("%c",authors_list[j][k]);
		    		}
		    	}
		    	printf(" | ");


		    	for(int k = 0 ; k < 100; k++)
		    	{
		    		if(title_list[j][k] != '|')
		    		{
		    			printf("%c",title_list[j][k]);
		    		}
		    	}
		    	printf(" | ");


		    	int edition;
		    	if(edition_digit_list[j][1] != -1)
		    	{
		    		edition = edition_digit_list[j][0] * 10 + edition_digit_list[j][1];
		    	}
		    	else
		    	{
		    		edition = edition_digit_list[j][0];
		    	}
		    	edition_list[j] = edition;
		    	printf("edition: %d | ", edition);


		    	int year = year_digit_list[j][0] * 1000 + year_digit_list[j][1] * 100 + year_digit_list[j][2] * 10 + year_digit_list[j][3];
		    	year_list[j] = year;
		    	printf("year: %d | ", year);


		    	for(int k = 0 ; k < 100; k++)
		    	{
					if(publisher_list[j][k] != '|')
					{
		    			printf("%c",publisher_list[j][k]);
					}
		    	}
		    	printf(" | ");


		    	int inventory;
		    	if(inventory_digit_list[j][1] != -1)
		    	{
		    		inventory = inventory_digit_list[j][0] * 10 + inventory_digit_list[j][1];
		    	}
		    	else
		    	{
		    		inventory = inventory_digit_list[j][0];
		    	}
		    	inventory_list[j] = inventory;
		    	printf("inventory number: %d | ", inventory);
			

		    	int available;
		    	if(available_digit_list[j][1] != -1)
		    	{
		    		available = available_digit_list[j][0] * 10 + available_digit_list[j][1];
		    	}
		    	else
		    	{
		    		available = available_digit_list[j][0];
		    	}
		    	available_list[j] = available;
		    	printf("available number: %d\n", available);
			}
			fclose(book_db_file);
        }
    }
}


int isISBNValid(char isbn[])
{
	int sum = 0;
	for (int i = 1; i<= 12; i++)
	{
         if(i % 2 != 0)
         {
         	sum = sum + (isbn[i - 1] - '0');
         }
         else
         {
         	sum = sum + (isbn[i - 1] - '0') * 3;
         }
	}
	sum = sum + (isbn[12] - '0');
	//printf("\nsum: %d\n", sum);
	if(sum % 10 == 0)
		return 1;
	else
		return 0;
}

int findBook(char isbn[], char isbn_list[][13], int record_number)
{
	for(int i = 0 ; i < record_number ; i++)
	{
		for(int j = 0; j < 13; j++)
		{
			if(isbn[j] != isbn_list[i][j])
			{
				break;
			}
			if(j == 12)
			{
				return i;
			}
		}
	}

	return -1;
}

int isValidLogin(int user_id, int user_password)
{
	FILE * user_db_file;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    user_db_file = fopen("user_db.txt", "r");
    if (user_db_file == NULL)
        DieWithError("User Database not found!");

    while ((read = getline(&line, &len, user_db_file)) != -1)  // count the number of books in the database file
    {  
    	int len = strlen(line);         // line length 
    	int flag = 0;
    	int line_id = 0;
    	int line_password = 0;
    	int divider = 0;
    	for(int i = len - 2 ; i >= 0; i--)   // start from len - 2, last element is 'return'
    	{
    		if(line[i] == '|') 
    		{
    			divider = i;
    			flag++;
    		}
    		else
    		{
    			if(flag == 0)
    			{
    				line_password += (line[i] - '0') * pow((float)10, (float)(len - i - 2));
    			}
    			else
    			{
    				line_id += (line[i] - '0') * pow((float)10, (float)(divider - i - 1));
				}
    		}
    	}
    	if(line_password == user_password && line_id == user_id)
    	{
    		return 1;
    	}
    }
	return -1;
}

int ifLoggedIn(int user_id, int current_user[])
{
	for(int i = 0; i < 99; i++)
	{
		if(current_user[i] == user_id)
		{
			return 1;
		}
	}
	return -1;
}


