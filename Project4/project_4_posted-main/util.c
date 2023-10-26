#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int master_fd;
pthread_mutex_t accept_con_mutex = PTHREAD_MUTEX_INITIALIZER;


/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port) {
   int sd;
   struct sockaddr_in addr;
  //  int ret_val;
  //  int flag;
   

   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/
   
   // TODO: Create a socket and save the file descriptor to sd (declared above)
   // This socket should be for use with IPv4 and for a TCP connection.

   sd = socket(AF_INET, SOCK_STREAM, 0);

    if(sd < 0){
    printf("ERROR: Failed to create socket\n");
    exit(1);
   }
    
   // TODO: Change the socket options to be reusable using setsockopt(). 
   int enable = 1;
    int sso = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(int)); 
    if(sso == -1){
      perror("ERROR: Setsocketopt failed\n");
      exit(1);
    }
    
   // TODO: Bind the socket to the provided port.

    addr.sin_family = AF_INET; //copied 64-66 from lec slides
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    int bind_res = bind(sd, (struct sockaddr*)&addr, sizeof(addr));

   // TODO: Mark the socket as a pasive socket. (ie: a socket that will be used to receive connections)
    if(bind_res < 0) {
      printf("ERROR: bind() failed\n");
      exit(1);
    }
   
   
   // We save the file descriptor to a global variable so that we can use it in accept_connection().
   master_fd = sd;

   // listen 20 because from handout:
   // The server should accept a backlog of up to 20 connection requests
   listen(master_fd, 20);
   printf("UTILS.O: Server Started on Port %d\n",port);

}





/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
     DO NOT use the file descriptor on your own -- use
     get_request() instead.
   - if the return value is negative, the thread calling
     accept_connection must should ignore request.
***********************************************/
int accept_connection(void) {
   int newsock;
   struct sockaddr_in new_recv_addr;
   uint addr_len;
   addr_len = sizeof(new_recv_addr);
   
   
   
   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/
   
   
   
   // TODO: Aquire the mutex lock
    if(pthread_mutex_lock(&accept_con_mutex) != 0){
      printf("ERROR: Unable to aqcuire lock\n");
      return -1;
    }

   // TODO: Accept a new connection on the passive socket and save the fd to newsock
    newsock = accept(master_fd, (struct sockaddr*)&new_recv_addr, &addr_len); //copied from slides

    if(newsock == -1){  
      printf("ERROR: New connection attempt failed\n");
      exit(1);
    }

   // TODO: Release the mutex lock
    if(pthread_mutex_unlock(&accept_con_mutex) != 0){
      printf("ERROR: Unable to unlock lock\n");
      exit(1);
    }

   // TODO: Return the file descriptor for the new client connection
   return newsock;
}





/**********************************************
 * get_request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        from where you wish to get a request
      - filename is the location of a character buffer in which
        this function should store the requested filename. (Buffer
        should be of size 1024 bytes.)
   - returns 0 on success, nonzero on failure. You must account
     for failures because some connections might send faulty
     requests. This is a recoverable error - you must not exit
     inside the thread that called get_request. After an error, you
     must NOT use a return_request or return_error function for that
     specific 'connection'.
************************************************/
int get_request(int fd, char *filename) {

      /**********************************************
    * IMPORTANT!
    * THIS FUNCTION DOES NOT NEED TO BE COMPLETE FOR THE INTERIM SUBMISSION, BUT YOU WILL NEED
    * CODE IN IT FOR THE INTERIM SUBMISSION!!!!! 
    **********************************************/
    
    
   char first_line[2048];
   int fd2 = dup(fd);

   if(fd2 < 0) {
    printf("ERROR: dup() failed\n");
    exit(1);
   }

   FILE *fp = fdopen(fd2, "r");
   if(fp == NULL){
    if(close(fd2) != 0){
      perror("ERROR: Unable to close fd\n");
    } 
    printf("ERROR: fdopen() failed\n");
    exit(1);
   }

    // INTERIM TODO: Read the request from the file descriptor into the buffer
  if(fgets(first_line, sizeof(first_line), fp) == NULL){
    printf("ERROR: fgets failed\n");
    exit(1);
  }
    // INTERIM TODO: PRINT THE REQUEST TO THE TERMINAL
  printf("Interim results: %s\n", first_line);
  const char s[2] = " ";

  // use strtok() to separate the first line by spaces
  char *req_type = strtok(first_line, s);
  char *filename_passedin = strtok(NULL, s);
  char *http_check = strtok(NULL, s);
  int format_correct = 0;

  // account for trailing whitespace from fgets() for ending string
  http_check[8] = '\0';

  // TODO: Ensure that the incoming request is a properly formatted HTTP "GET" request
  // The first line of the request must be of the form: GET <file name> HTTP/1.0 
  // check formatting
  // check request type format
  int r, r1, r2;
  if((r = strcmp(req_type, "GET")) != 0) {
    printf("ERROR: Request type format incorrect %d\n", r);
    printf("length of req type %ld\n", strlen(req_type));
    format_correct ++;
  }
  // check http format (2 cases)
  if(((r1 = strcmp(http_check, "HTTP/1.1") != 0) && (r2 = strcmp(http_check, "HTTP/1.0")) != 0)) {
    printf("ERROR: Http format incorrect\n");
    format_correct ++;
  }

  // check maxlen of filename is not > 1023 bytes
  if(strlen(filename_passedin) > 1023){
    printf("ERROR: Filename exceeded maximum length\n");
    format_correct ++;
  }
  
  // TODO: Ensure the file name does not contain with ".." or "//"
  if(strstr(filename_passedin, "..") != NULL || strstr(filename_passedin, "//") != NULL){
    format_correct ++;
  }
  if(format_correct > 0){
    printf("ERROR: Invalid request and/or filename\n\n");
    if(fclose(fp) != 0){
      perror("ERROR: Unable to close FILE\n");
    }
    return -1;
  }
  // TODO: Extract the file name from the request -- did above
  // TODO: Copy the file name to the provided buffer
  strcpy(filename, filename_passedin);
  if(fclose(fp) != 0){
    perror("ERROR: Unable to close FILE\n");
  }
  return 0;

}





/**********************************************
 * return_result
   - returns the contents of a file to the requesting client
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the result of a request
      - content_type is a pointer to a string that indicates the
        type of content being returned. possible types include
        "text/html", "text/plain", "image/gif", "image/jpeg" cor-
        responding to .html, .txt, .gif, .jpg files.
      - buf is a pointer to a memory location where the requested
        file has been read into memory (the heap). return_result
        will use this memory location to return the result to the
        user. (remember to use -D_REENTRANT for CFLAGS.) you may
        safely deallocate the memory after the call to
        return_result (if it will not be cached).
      - numbytes is the number of bytes the file takes up in buf
   - returns 0 on success, nonzero on failure.
************************************************/
int return_result(int fd, char *content_type, char *buf, int numbytes) {
  // TODO: Prepare the headers for the response you will send to the client.
   // REQUIRED: The first line must be "HTTP/1.0 200 OK"
   // REQUIRED: Must send a line with the header "Content-Length: <file length>"
   // REQUIRED: Must send a line with the header "Content-Type: <content type>"
   // REQUIRED: Must send a line with the header "Connection: Close"
   
   // NOTE: The items above in angle-brackes <> are placeholders. The file length should be a number
   // and the content type is a string which is passed to the function.
   
   /* EXAMPLE HTTP RESPONSE
    * 
    * HTTP/1.0 200 OK
    * Content-Length: <content length>
    * Content-Type: <content type>
    * Connection: Close
    * 
    * <File contents>
    */
    char header[2048];
    if( (sprintf(header,
      "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\nConnection: Close\n\n",
      content_type, numbytes)) < 0){  
        printf("ERROR: Failed to sprintf\n");
        exit(1);
      }

    // TODO: Send the HTTP headers to the client
     // IMPORTANT: Add an extra new-line to the end. There must be an empty line between the 
    // headers and the file contents, as in the example above.

    if(send(fd, header, strlen(header), 0)< 0){  
        printf("ERROR: Failed to send the HTTP headers to the client\n");
        exit(1);
      }
    
    // TODO: Send the file contents to the client
    if(send(fd, buf, numbytes, 0) < 0){  
        printf("ERROR: Failed to send the file contents to the client\n");
        exit(1);
      }
    // TODO: Close the connection to the client
    if(close(fd) < 0){ 
      perror("ERROR: Fail to close the file.\n");
      exit(1);

    }
    
    return 0;
}





/**********************************************
 * return_error
   - returns an error message in response to a bad request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the error
      - buf is a pointer to the location of the error text
   - returns 0 on success, nonzero on failure.
************************************************/
int return_error(int fd, char *buf) {

   // TODO: Prepare the headers to send to the client
   // REQUIRED: First line must be "HTTP/1.0 404 Not Found"
   // REQUIRED: Must send a header with the line: "Content-Length: <content length>"
   // REQUIRED: Must send a header with the line: "Connection: Close"
   
   // NOTE: In this case, the content is what is passed to you in the argument "buf". This represents
   // a server generated error message for the user. The length of that message should be the content-length.
   
   // IMPORTANT: Similar to sending a file, there must be a blank line between the headers and the content.
   
   
   
   /* EXAMPLE HTTP ERROR RESPONSE
    * 
    * HTTP/1.0 404 Not Found
    * Content-Length: <content length>
    * Connection: Close
    * 
    * <Error Message>
    */
  
    char header[2048];
	  if((sprintf(header, 
      "HTTP/1.1 404 Not Found\nContent-Type: text/plain\nContent-Length: %ld\nConnection: Close\n\n",
		  strlen(buf))) < 0){  
        printf("ERROR: Failed to sprintf\n");
        exit(1);
      }
    
    // TODO: Send headers to the client
    if(send(fd, header, strlen(header), 0) < 0){  
        printf("ERROR: Failed to send the file contents to the client\n");
        exit(1);
      }
    
    // TODO: Send the error message to the client
    if(send(fd, buf, strlen(buf), 0) < 0){  
        printf("ERROR: Failed to send the error message to the client\n");
        exit(1);
      }

    // TODO: Close the connection to the client
    if(close(fd) < 0){ 
      perror("ERROR: Fail to close the file.\n");
      exit(1);

    }
    
    return 0;
}
