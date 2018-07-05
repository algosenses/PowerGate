//////////////////////////////////////////////////////////
//Name:     Jacob Brown             //
//Student #:    100 762 690             //
//Course:   COMP3203                //
//Title:    Authentication like KERBEROS Source     //
//Date:     31 October 2014             //
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//                              //
//Description:  This is a KERBEROS like implementation      //
//      developed by student, Jacob Brown, at       //
//      Carleton University. KERBEROS was dev-      //
//      eloped by MIT. This implementation us-      //
//      es sha256 for the hashing algorithm a-      //
//      nd BBC as the encryption for salt val-      //
//      ues and any information needed to be        //
//      encrypted. Developed for COMP3203       //
//      Fundamentals of computer networks.      //
//                              //
//////////////////////////////////////////////////////////////

#include "authentication.h"     // The header File including the prototypes
#include <string.h>             // Used for strcmp(), strlen()
#include <time.h>               // Used for time() initializing srand()
#include <stdio.h>              // Used for standard in/out functions
#include <stdlib.h>             // Used for all standard Library calls
//#include <unistd.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

////////////////////////////////////////////////////////////////////////////////
int generate_salt();
/*	Computes the hash value of some input
	and returns the output hash value. 
	Returns 0 if failed to hash and 1 if
	function runs successful. Takes 4 
	arguments: First - The input string to
	be hashed (password), Second - The
	output hashed value, Third - The
	Value of the key will be assigned to 
	this variable, Fourth - The salt Value
	is assigned to this variable */
int hash_value(char*, char*, char*, int*);
int compare_hash_values(char *, char *);
void get_current_time(char *);
int generate_key(char*);
////////////////////////////////////////////////////////////////////////////////

#define HASH_LENGTH_256 32      // Constant used for lengths of 256-bit hashing
#define HASH_RANGE      177     // Constant used for modulus to remain inside ascii values
#define HASH_CHAR_RANGE 36      // number of possible characters for hash

/*  Generates a random one time use
    salt value. Takes no arguments. */
int generate_salt(){
    srand(time(NULL));
    long r = 0;
    for (int i = 0; i < 64; i++){
        r += rand() % 100;
    }
    return abs(r);
}

/*  Computes the hash value of some input
    and returns the output hash value.
    Returns 0 if failed to hash and 1 if
    function runs successful.  */
static int hash_value(char *input, char *output, char *key_value, int *salt_value){
    /*  Array of characters for the hash function  */
    char values[HASH_CHAR_RANGE] = {'A','B','C','D','E','F',
                                    'G','H','I','J','K','L',
                                    'M','N','O','P','Q','R',
                                    'S','T','U','V','W','X',
                                    'Y','Z','1','2','3','4',
                                    '5','6','7','8','9','0'};

    /*  Variable to be used for function  */
    int len = strlen(input);
    int total[HASH_LENGTH_256];
    int i,j;
    int salt;
    char key[HASH_LENGTH_256+1];                // The empty key array for function

    key[HASH_LENGTH_256] = 0x00;
    //printf("salt value is: %d \n", *salt_value);

    if((strlen(key_value) == 32)){          // This means a key has been provided
        strncpy(key, key_value, 32);        // Key value is assigned var for function to use
        //printf("key is provided: %s\n", key);
        if(salt_value >= 0){                    // Confirms that a salt value was provided as well
            salt = *salt_value;             // Assigns the provided salt value to variable
            //printf("Salt used: %d\n", salt);
        } else {
            printf("Error: No salt value provided with key\n");  // Error message for missing salt
            exit(1);
        }
    } else{
        salt = generate_salt();                 // One time Salt value is obtained (to be stored for client)
        *salt_value = salt;                     // Salt value assigned to var for client use
        generate_key(key);                      // Key value is generated
        strncpy(key_value, key, 32);            // Key value is assigned to var for client use
        //printf("RTN Key: %s\n", key_value);
    }
    if(input[strlen(input) - 1] == '\n'){       // Remove newline character from string if present
        input[strlen(input) - 1] = '\0';
    }
    //printf("input is: %s\n", input);
    // generate the hash combined with salt and message
    if(strlen(key) > 0){                // Confirms output was loaded with key

        /*  Here we do a number of rounds to complete the Hash */
        //printf("key is: %s\n", key);
        /*  First round we add the ascii value to the salt value
            modulus the HASH_CHAR RANGE_RANGE, which is 36 and add
            this value to running totals total[i] */
        for(i = 0; i < HASH_LENGTH_256; i ++){
            total[i]  = ((int)key[i] + salt) % HASH_CHAR_RANGE;
//          printf("sizeof key = %lu\n", sizeof(key));
        // ROUND 1
        }
        //printf("key is: %s\n", key);
        /*  Second round */
        //printf("second time we handle input\n");
        for(i = 0; i < HASH_LENGTH_256; i ++){
            total[i]  = (total[i] + ((int)input[i % len] + salt)
                            % HASH_CHAR_RANGE) % HASH_CHAR_RANGE;   // ROUND 2
        }
        //printf("key is: %s\n", key);
        /*  Third round */
        //printf("third round of manipulation\n");
        for(i = 0; i < HASH_LENGTH_256; i ++){
            total[i]  = (total[HASH_LENGTH_256 - i - 1] + ((int)input[i % len] + salt)
                            % HASH_CHAR_RANGE) % HASH_CHAR_RANGE;   // ROUND 3
        }
        //printf("key is: %s\n", key);
        /*  Assign the character equivalent of each total to the
            output array that will be our HASH VALUE  */
        //printf("fourth round to assign chars\n");
        for(i = 0; i < HASH_LENGTH_256; i ++){
            output[i] = values[total[i]];   // The Final saved HASH VALUE
        }
        //printf("Finished hashing\n");
    } else{                                 // If output failed to load key return 0
        return 0;
    }
    return 1;
}

/*  Determines if two hash values are the
    same. Function returns 0 if false, and
    one if true. Takes two arguments. First
    is the first hash value as type const
    unsigned char and second is the second
    value of the same type */
int compare_hash_values(char *one, char *two)
{
    int result = 1;
    int i = 0;
    for(i = 0; i < HASH_LENGTH_256; i++){
        if(one[i] == two[i]){
            result = result & 1;                // Bitwise AND used to maintain 1 when valid
        } else{
            result = 0;                         // Bitwise AND against 0 results in invalid
        }
    }
    return result;                              // Return the result 0 invalid 1 valid
}

/*  sets a char pointer to the current
    time based on the host system  */
void get_current_time(char *tme){
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime );
    timeinfo = localtime ( &rawtime );
    printf ( "|  Time stamp\t: %s", asctime (timeinfo) );

}

/*  Takes a char by reference and
    assigns a generated random key
    to be used inside hash_value */
int generate_key(char *key){
    srand(time(NULL));

    /*  An array of chars storing the alphabetical
        characters for the english language is
        used to generate a key to be used for the
        hash function. Called within the hash
        function and assigned to the third variable
        passed to hash_value() */
    char letters[26] = {'a','b','c','d','e','f',
                        'g','h','i','j','k','l',
                        'm','n','o','p','q','r',
                        's','t','u','v','w','x',
                        'y','z'};
    int i;
    for(i = 0; i < HASH_LENGTH_256; i++){
        int r = rand();
        key[i] = letters[r % 26];               // Random values from a to z generate the key
    }
    //printf("key generated: %s\n", key);
    return 1;
}

#define MAX   (512)

int authenticate(const char* username, const char* passwd, const char* srvIp, int port)
{
    if (username == nullptr || username[0] == '\0' ||
        passwd == nullptr || passwd[0] == '\0' ||
        srvIp == nullptr || port <= 0) {
        return -1;
    }

    SOCKET          sock;                                   // Socket to be created
    struct          sockaddr_in  thisAddr;      // Struct for socket
    char            rcvBuff[MAX];                           // Buffer for receiving from server
    int             rcvMsgSize;                             // Int to store the size of the msg received
    int             authenticated = 0;

    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET){                                    // Verify socket was created successfully
        printf("Failed to create socket..\n");
        return -1;
    }

    memset(&thisAddr, 0, sizeof(thisAddr));
    thisAddr.sin_family = AF_INET;
    //    thisAddr.sin_addr.s_addr = inet_addr(serverIp);
    thisAddr.sin_addr.S_un.S_addr = inet_addr(srvIp);
    thisAddr.sin_port = htons(port);                            // htons used for sending to account for network bit order

    if (connect(sock, (struct sockaddr *)&thisAddr, sizeof(thisAddr)) == INVALID_SOCKET) {
        printf("Failed to connect to socket..\n");
        closesocket(sock);
        return -1;
    }

    char    output[33]; output[32] = 0x00;
    char    password[32] = { 0 };
    char    aKey[33]; aKey[32] = 0x00;
    int     aSalt;
    int     i = 0, j = 0;
    int     failed = 0;

    strcpy(password, passwd);  // password is then loaded into a var named password

    send(sock, username, strlen(username), 0); // the buffer is then send across the socket to the server to be processed

    memset(rcvBuff, 0, sizeof(rcvBuff));                            // reset the buffers each time loop is ran for new request to be sent
    // here we wait for any incoming data from the server
    if (rcvMsgSize = recv(sock, rcvBuff, MAX, 0) < 0) {
        printf("recvfrom() failed\n");                          // Handle is any errors occur during the receive
        closesocket(sock);
        return -1;
    }

    int len = strlen(rcvBuff);
    if (len > 32 && rcvBuff[len - 1] == 'F' && rcvBuff[len - 2] == 'O' && rcvBuff[len - 3] == 'E') {
        rcvBuff[len - 3] = '\0';
        strncpy(aKey, rcvBuff, sizeof(char)* 32);
        sscanf(rcvBuff + 32, "%d", &aSalt);
        failed = 0;
    }
//    printf("reply from server:\n%s\n", rcvBuff);                // The reply from the server is display for clients convenience

    if (!failed){
        int result = hash_value(password, output, aKey, &aSalt);         // Here we has the provided values with our password
        if (result){
            // The hashed result is then send across the socket to the server to be processed
            send(sock, output, sizeof(char)* 32, 0);
            // after the result is sent we wait for a reply form the server saying whether they match or dont
            // If they match we authenticate, If not we don't.
            memset(rcvBuff, 0, sizeof(rcvBuff));                            // reset the buffers each time loop is ran for new request to be sent
            // here we wait for any incoming result from the server
            if (rcvMsgSize = recv(sock, rcvBuff, MAX, 0) < 0) {
                printf("recvfrom() failed\n");                          // Handle is any errors occur during the receive
                closesocket(sock);
                return -1;
            }
            if (strcmp(rcvBuff, "1") == 0){                             // If true we have been authenticated
                authenticated = 1;
            } else{
                authenticated = 0;
            }

        }
    }

    closesocket(sock);                                                  // Close socket and end program

    WSACleanup();

    return authenticated;
}
