#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

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

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[ECHOMAX+1];      /* Buffer for receiving echoed string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of received response */

    int user_request_id = 0;
    int user_id;
    int user_password;
    int user_request_type;
    char user_isbn[] = {'9','7','8','0','1','3','3','3','5','4','6','9','0'};


    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];           /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]);  /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServPort);     /* Server port */

    printf("\n\n\n\n\n\n\n\n\n\n============ Welcome to Book Loan System ============\n");

    int end_program = 0;
    int is_login = 0;
    while(end_program != 1)    // start main loop
    {
//========================================================================
//  if not logged in

        if(is_login == 0)
        {
            printf("What do you want to do?\n");
            printf("1. Login\n");
            printf("2. Exit\n");
            printf("Please type the number of command you want.\n");
            int selection;
            scanf("%d", &selection);

            if(selection == 1)         // if choose login
            {
                printf("\nUser ID: \n");
                scanf("%d", &user_id);
                printf("Password: \n");
                scanf("%d", &user_password);

                ClientMessage client_message;     // setup login message

                client_message.requestID = user_request_id;
                client_message.userID = user_id;
                client_message.password = user_password;
                client_message.requestType = Login;

                ServerMessage * server_message = malloc(sizeof(ServerMessage));

                int client_message_length = (sizeof(client_message));  // send the login message
                if (sendto(sock, (struct ClientMessage*)&client_message, client_message_length, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != client_message_length)
                {
                    DieWithError("sendto() sent a different number of bytes than expected");
                }
                fromSize = sizeof(fromAddr);

                respStringLen = recvfrom(sock, server_message, sizeof(*server_message), 0, (struct sockaddr *) &fromAddr, &fromSize);     // get login response from server

                if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
                {
                    fprintf(stderr,"Error: received a packet from unknown source.\n");
                    exit(1);
                }
                user_request_id = user_request_id + 1;

                if(server_message->respType == Okay)    // if valid login
                {
                    printf("Successfully login as: %d\n", user_id);
                    is_login = 1;
                }
                else
                {
                    printf("failed login as: %d, wrong user id or password?\n", user_id);
                }
                
            }
            else if(selection == 2)    // exit program
            {
                end_program = 1;
            }
            else
            {
                printf("Unknown command\n\n\n===========================================\n");
                char c;
                while((c = getchar()) != '\n' && c != EOF);    // clear scanf buffer
            }
        }
        else
        {
//===============================================================
//  if already logged in

            ClientMessage client_message;
            client_message.requestID = user_request_id;
            client_message.userID = user_id;
            printf("\n\n\n===============================================\n");
            printf("What do you want to do?\n");
            printf("1. Query\n");
            printf("2. Borrow\n");
            printf("3. Return\n");
            printf("4. Logout\n");
            printf("Please type the number of command you want:\n");
            int selection;
            int if_logout = 0;
            int if_wrong_command = 0;
            
            scanf("%d", &selection);   // choose what to do
            switch(selection) 
            {
                case 1 :    // query
                    client_message.requestType = Query;
                    printf("Query==========================================\n");
                    break;
                
                case 2 :    // borrow
                    client_message.requestType = Borrow;
                    printf("Borrow==========================================\n");
                    break;

                case 3 :    // return
                    client_message.requestType = Return;
                    printf("Return:==========================================\n");
                    break;

                case 4 :    // logout
                    if_logout = 1;
                    printf("Logout:==========================================\n");
                    break;

                default :   // unknown command
                    if_wrong_command = 1;
                    printf("Unknown command.\n\n\n");
                    char c;
                    while((c = getchar()) != '\n' && c != EOF);
            }

            if(if_logout == 0 && if_wrong_command == 0)     // if not logout or wrong command
            {
                printf("Please type the ISBN number:\n");    // type in ISBN
                char input_isbn;
                scanf("%s", &input_isbn);
                memcpy(&client_message.isbn, &input_isbn, 13);

                if(isISBNValid(client_message.isbn) != 1)    // validate the ISBN
                {
                    printf("Invalid ISBN\n\n\n");
                    continue;
                }

            //===============================================================
            //  send request
                client_message.requestID = user_request_id;

                int client_message_length = (sizeof(client_message));
                if (sendto(sock, (struct ClientMessage*)&client_message, client_message_length, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != client_message_length)
                {
                    DieWithError("sendto() sent a different number of bytes than expected");
                }

              
            //===============================================================
            //  receive from server

                /* Recv a response */
                fromSize = sizeof(fromAddr);
                ServerMessage * server_message = malloc(sizeof(ServerMessage));
                respStringLen = recvfrom(sock, server_message, sizeof(*server_message), 0, (struct sockaddr *) &fromAddr, &fromSize);

                if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
                {
                    fprintf(stderr,"Error: received a packet from unknown source.\n");
                    exit(1);
                }
                user_request_id = user_request_id + 1;
            //===============================================================
            //  show response

                printf("================================================\n"); 
                switch(server_message->respType) 
                {
                    case Okay :      // if Okay, display all information of the book
                        printf("Status: Okay \nISBN: \n");   // ISBN
                        for(int i = 0; i < 13; i++)
                        {
                            printf("%c", server_message->isbn[i]);
                        }
                        printf("\nTitle: ");                 // Title
                        for(int i = 0; i < 100; i++)
                        {
                            if(server_message->title[i] != '|')
                            {
                                printf("%c", server_message->title[i]);
                            }
                            else
                            {
                                break;
                            }
                        }
                        printf("\nAuthors: ");               // Authors
                        for(int i = 0; i < 100; i++)
                        {
                            if(server_message->authors[i] != '|')
                            {
                                printf("%c", server_message->authors[i]);
                            }
                            else
                            {
                                break;
                            }
                        }
                        printf("\nPublisher:");              // Publisher
                        for(int i = 0; i < 100; i++)
                        {
                            if(server_message->publisher[i] != '|')
                            {
                                printf("%c", server_message->publisher[i]);
                            }
                            else
                            {
                                break;
                            }
                        }
                        printf("\n");
                        printf("Edition: %d\n", server_message->edition);    // edition
                        printf("Year: %d\n", server_message->year);          // year
                        printf("In inventory: %d\n", server_message->inventory);          // inventory 
                        printf("Available for borrow: %d\n", server_message->available);  // available
                        break; 
                    
                    case ISBNError :
                        printf("Status: ISBNError \nISBN not found or goes wrong in transmission.\n");
                        break; 

                    case AllGone :
                        printf("Status: AllGone. No available copy now. \n");
                        break; 

                    case NoInventory :
                        printf("Status:NoInventory. Don't have any copy in inventory. \n");
                        break; 

                    case InvalidLogin :
                        printf("Status: InvalidLogin \n");
                        break; 
                 
                    default : 
                        printf("Status: unknown response \n");
                }
            }
            else if(if_logout == 1)   // if user choose to logout
            {
                client_message.requestType = Logout;
                client_message.userID = user_id;
                int client_message_length = (sizeof(client_message));
                if (sendto(sock, (struct ClientMessage*)&client_message, client_message_length, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != client_message_length)
                {
                    DieWithError("sendto() sent a different number of bytes than expected");
                }
            //===============================================================
            //  receive from server

                fromSize = sizeof(fromAddr);
                ServerMessage * server_message = malloc(sizeof(ServerMessage));
                respStringLen = recvfrom(sock, server_message, sizeof(*server_message), 0, (struct sockaddr *) &fromAddr, &fromSize);

                if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
                {
                    fprintf(stderr,"Error: received a packet from unknown source.\n");
                    exit(1);
                }
                user_request_id = user_request_id + 1;

                if(server_message->respType == Okay) // if server confirm the logout
                {
                    is_login = 0;
                    printf("User <%d> logout successfully.\n\n\n", user_id);
                }
                else
                {
                    printf("Logout error, please try again. \n");
                }
            }
            else
            {
                ;
            }
        }
    }
    close(sock);
    exit(0);
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

    if(sum % 10 == 0)
        return 1;
    else
        return 0;
}
