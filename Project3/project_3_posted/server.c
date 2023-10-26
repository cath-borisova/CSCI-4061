#include "server.h"
#define PERM 0644

//Global Variables [Values Set in main()]
int queue_len           = INVALID;                              //Global integer to indicate the length of the queue
int cache_len           = INVALID;                              //Global integer to indicate the length or # of entries in the cache        
int num_worker          = INVALID;                              //Global integer to indicate the number of worker threads
int num_dispatcher      = INVALID;                              //Global integer to indicate the number of dispatcher threads      
FILE *logfile;                                                  //Global file pointer for writing to log file in worker


/* ************************ Global Hints **********************************/

//int ????      = 0;                            //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
int workerIndex = 0;                            //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0;                        //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int curequest= 0;                               //[multiple funct]  --> How will you update and utilize the current number of requests in the request queue?


pthread_t worker_thread[MAX_THREADS];           //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS];       //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
//pass thred ptr to correct index to arry

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;        //What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t some_content = PTHREAD_COND_INITIALIZER;  //What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t free_space = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];                    //How will you track the requests globally between threads? How will you ensure this is thread safe?
int queue_index = 0; //added by katya

//cache_entry_t* ?????;                                  //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global
cache_entry_t *cache; //cache array - katya added
int used_entries = 0; // katya added
int next_to_delete = 0; // katya added
/**********************************************************************************/


/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/


/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  /* TODO (GET CACHE INDEX) - DONE (KATYA)
  *    Description:      return the index if the request is present in the cache otherwise return INVALID
  */
  for(int i = 0; i <used_entries; i++){
    if(strcmp(request, cache[i].request) == 0){
      return i;
    }
  }
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  /* TODO (ADD CACHE) - DONE (KATYA)
  *    Description:      It should add the request at an index according to the cache replacement policy
  *                      Make sure to allocate/free memory when adding or replacing cache entries
  */
 
 // case 1: cache is full or cache is empty
  if(used_entries == cache_len){ //no more free spots, so use the first index FIFO
    free(cache[next_to_delete].content);
    free(cache[next_to_delete].request);
    cache[next_to_delete].request = (char *)malloc(BUFF_SIZE * sizeof (char));
    if(cache[next_to_delete].request == NULL){
      printf("Error out of memory, malloc failed\n");
      exit(-1);
    }
    cache[next_to_delete].content = (char *)malloc(memory_size);
    if(cache[next_to_delete].content == NULL){
      printf("Error out of memory, malloc failed\n");
      exit(-1);
    }
    strcpy(cache[next_to_delete].request, mybuf);
    memcpy(cache[next_to_delete].content, memory, memory_size);
    cache[next_to_delete].len = memory_size;

    next_to_delete = (next_to_delete + 1) % cache_len;
  } else { // case 2: cache is not empty or full

    cache[used_entries].request = (char *)malloc(BUFF_SIZE * sizeof (char)) ;
    cache[used_entries].content = (char *)malloc(memory_size);

    strcpy(cache[used_entries].request, mybuf);
    memcpy(cache[used_entries].content, memory, memory_size);
    cache[used_entries].len = memory_size;

    used_entries +=1;
  }
  return;
}

// Function to clear the memory allocated to the cache
void deleteCache(){
  /* TODO (CACHE) - DONE (KATYA)
  *    Description:      De-allocate/free the cache memory
  */
  for(int i = 0; i < cache_len; i++){
    cache[i].len = 0;
    free(cache[i].content);
    free(cache[i].request);
  }
  free(cache);
}

// Function to initialize the cache
void initCache(){
  /* TODO (CACHE) - DONE (KATYA)
  *    Description:      Allocate and initialize an array of cache entries of length cache size
  */
  cache = malloc(sizeof(cache_entry_t)*cache_len);
  if(cache == NULL){
      printf("Error out of memory, malloc failed\n");
      exit(-1);
  }
  for(int i = 0; i < cache_len; i++){
    cache[i].len = 0;
    cache[i].content = NULL;
    cache[i].request = NULL;
  }
  //set everything to NULL and 0 to avoid garbage
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  /* TODO (Get Content Type)
  *    Description:      Should return the content type based on the file type in the request
  *                      (See Section 5 in Project description for more de".gif"s)
  *    Hint:             Need to check the end of the string passed in to check for .html, .jpg, .gif, etc.
  */
  char *content_type = NULL;
  if (strncasecmp(mybuf + strlen(mybuf) - strlen(".gif"), ".gif", strlen(".gif")) == 0)
  {
    content_type = "image/gif";
  }
  if (strncasecmp(mybuf + strlen(mybuf) - strlen(".jpg"), ".jpg", strlen(".jpg")) == 0)
  {
    content_type = "image/jpeg";
  }
  if (strncasecmp(mybuf + strlen(mybuf) - strlen(".html"), ".html", strlen(".html")) == 0)
  {
    content_type = "image/html";
  }
  if (content_type != NULL) {
    return content_type;
  }
  else {
    return "text/plain";
  }
}

// Function to open and read the file from the disk into the memory. Add necessary arguments as needed
// Hint: caller must malloc the memory space
int readFromDisk(int fd, char *mybuf, void **memory) {
  //    Description: Try and open requested file, return INVALID if you cannot meaning error


  FILE *fp;
  if((fp = fopen(mybuf, "r")) == NULL){
     fprintf (stderr, "ERROR: Fail to open the file.\n");
    return INVALID;
  }


  fprintf (stderr,"The requested file path is: %s\n", mybuf);
  
  /* TODO ALMOST DONE - RUTH
  *    Description:      Find the size of the file you need to read, 
                          read all of the contents into a memory location and return the file size
  *    Hint:             Using fstat or fseek could be helpful here
  *                      What do we do with files after we open them?
  */


  struct stat st;
  if(stat(mybuf, &st) == -1){
    perror("Stat failed\n");
    return INVALID;
  }
  int fsize = st.st_size;  // get the length of the file

  // fprintf(stderr,"TEEEESSSTTT5 size: %d\n",fsize); // not getting size correctly
  *memory = malloc(fsize);
  if(memory == NULL) {
    printf("Error out of memory, malloc failed\n");
    exit(-1);
  }
  
  fread(*memory,fsize, 1, fp);
  if(fclose(fp)!= 0){
    fprintf (stderr, "ERROR: Fail to close the file.\n");
    return INVALID;
  }

  return fsize;
}

/**********************************************************************************/

// Function to receive the path)request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/

  request_t *tempreq = malloc(sizeof(request_t));
  if(tempreq == NULL){
      printf("Error out of memory, malloc failed\n");
      exit(-1);
    }
  tempreq->request = calloc(1, BUFF_SIZE);

  /* TODO (B.I) - DONE(ALICE)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int id = -1;
  id = *((int *) arg);

  //intalize array 1-max pass pointers to array
  //get argument in array int 
  fprintf(stderr,"Dispatcher\t	       [%d] Started\n", id);//segf

  while (1) {

    /* TODO (FOR INTERMEDIATE SUBMISSION)
    *    Description:      Receive a single request and print the conents of that request
    *                      The TODO's below are for the full submission, you do not have to use a 
    *                      buffer to receive a single request 
    *    Hint:             Helpful Functions: int accept_connection(void) | int get_request(int fd, char *filename
    *                      Recommend using the request_t structure from server.h to store the request. (Refer section 15 on the project write up)
    */

    /* TODO (B.II) - DONE (ALICE)
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void) //utils.h => Line 24
    */
    
    tempreq->fd = accept_connection();
    // if return value is neg, request should be ignored
    if (tempreq->fd < 0){
      printf("error\n");
      free(tempreq);
      continue;
    }



    /* TODO (B.III) - DONE (ALICE)
    *    Description:      Get request from the client
    *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
    */
 
    int res = get_request(tempreq->fd, tempreq->request); // ?? not sure code from Katya
    fprintf(stderr, "Dispatcher Received Request: fd[%d] request[%s]\n", tempreq->fd, tempreq->request);
    if (res == 0) {
      printf("Success getting request from client\n");
    }
    else {
      free(tempreq);
      continue;
    }
    /* TODO (B.IV) - DONE (KATYA)
    *    Description:      Add the request into the queue
    */

        //(1) Copy the filename from get_request into allocated memory to put on request queue 
        char *alloc_req = malloc(sizeof(tempreq->fd));
        if(alloc_req == NULL){
          printf("Error out of memory, malloc failed\n");
          exit(-1);
        }
        //(2) Request thread safe access to the request queue
        pthread_mutex_lock(&lock);

        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull  
        //while Q has something
        while(curequest == MAX_QUEUE_LEN){
          pthread_cond_wait(&some_content, &lock);
        }
      
        //(4) Insert the request into the queue
        req_entries[dispatcherIndex].fd = tempreq->fd;
        req_entries[dispatcherIndex].request = tempreq->request;
        curequest++;
        //(5) Update the queue index in a circular fashion - dont get, assuming smth with modulo? but then it messes up my use of the var before
        dispatcherIndex = (dispatcherIndex + 1) % queue_len;

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
        
        pthread_cond_signal(&free_space); //wake up consumers
        pthread_mutex_unlock(&lock);
        
        free(alloc_req);
 }
  free(tempreq);
  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) { 
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/


  #pragma GCC diagnostic ignored "-Wunused-variable"      //TODO --> Remove these before submission and fix warnings
  #pragma GCC diagnostic push                             //TODO --> Remove these before submission and fix warnings


  // Helpful/Suggested Declarations
  int num_request = 0;                                    //Integer for tracking each request for printing into the log
  bool cache_hit  = false;                                //Boolean flag for tracking cache hits or misses if doing 
  int filesize    = 0;                                    //Integer for holding the file size returned from readFromDisk or the cache
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char mybuf[BUFF_SIZE];                                  //String to hold the file path from the request

  #pragma GCC diagnostic pop                              //TODO --> Remove these before submission and fix warnings

 
  /* TODO (C.I)- DONE(RUTH)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int id = -1;
  id = *((int *) arg);

  while (1) {
    /* TODO (C.II)- DONE(RUTH)
    *    Description:      Get the request from the queue and do as follows
    */

          //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock

          pthread_mutex_lock(&lock);

          //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
 
          while(curequest == 0){ //wait while empty -- sleep
           pthread_cond_wait(&free_space, &lock);
          }
          
          //(3) Now that you have the lock AND the queue is not empty, read from the request queue
          
          fd = req_entries[workerIndex].fd;
          char* req = req_entries[workerIndex].request + 1;
          strcpy(mybuf, req);
          curequest--;
          
          //(4) Update the request queue remove index in a circular fashion
          
          workerIndex = (workerIndex + 1) % queue_len;
          
          //(5) Check for a path with only a "/" if that is the case add index.html to it
          if(strcmp(mybuf,"/")==0){ //html just in testing
            //where is the index.html - are we supposed to make it?
            strcat(mybuf,"index.html");
          }

          //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
          
          pthread_cond_signal(&some_content); //wake up producers
          pthread_mutex_unlock(&lock);
          

    /* TODO (C.III)- DONE(RUTH)
    *    Description:      Get the data from the disk or the cache 
    *    Local Function:   int readFromDisk(//necessary arguments//);
    *                      int getCacheIndex(char *request);  
    *                      void addIntoCache(char *mybuf, char *memory , int memory_size);  
    */
    /*If the
    request is in the cache (Cache HIT), it will get the result from the cache and return it to the user. If the
    request is not in the cache (Cache MISS), it will get the result from disk as usual, put the entry in the
    cache and then return result to the user. */

    
    int num_bytes_or_error;
    int in_cache = getCacheIndex(mybuf);
    if(in_cache == INVALID){//MISS
      cache_hit = false;
      num_bytes_or_error = readFromDisk(fd, mybuf, &memory);
      if(num_bytes_or_error == -1){
        //error checking
        if(pthread_mutex_unlock(&cache_lock) < 0){
          fprintf(stderr, "failed to unlock cache\n");
          exit(-1);
        }
        
       }else{//malloced in read from disk
        addIntoCache(mybuf, memory, num_bytes_or_error);
      }
      
    }else{//HIT
      cache_hit = true;
      int indx = getCacheIndex(mybuf);

      num_bytes_or_error = cache[indx].len;
      memory = malloc (num_bytes_or_error);
      if(memory == NULL) {
        printf("Error out of memory, malloc failed\n");
        exit(-1);
      }
      memcpy(memory, cache[indx].content, num_bytes_or_error);
    }

   
    // /* TODO (C.IV)- DONE(RUTH)
    // *    Description:      Log the request into the file and terminal
    // *    Utility Function: LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit);
    // *    Hint:             Call LogPrettyPrint with to_write = NULL which will print to the terminal
    // *                      You will need to lock and unlock the logfile to write to it in a thread safe manor
    // */
    
    
    pthread_mutex_lock(&log_lock);

    LogPrettyPrint(logfile, id, num_request, fd, mybuf, num_bytes_or_error, cache_hit);
    LogPrettyPrint(NULL, id, ++num_request, fd, mybuf, num_bytes_or_error, cache_hit);

    pthread_mutex_unlock(&log_lock);

    /* TODO (C.V)- DONE(RUTH)
    *    Description:      Get the content type and return the result or error
    *    Utility Function: (1) int return_result(int fd, char *content_type, char *buf, int numbytes); //look in utils.h 
    *                      (2) int return_error(int fd, char *buf); //look in utils.h 
    */
    if(num_bytes_or_error == -1){
      fprintf(stderr, "ERROR num_bytes_or_error = -1 in Worker \n");
      return_error(fd, "ERROR num_bytes_or_error in Worker\n");
    }else{
      fprintf(stderr, "Success in worker \n");
      char* content_type = getContentType(mybuf);
      if(return_result(fd, content_type, memory, num_bytes_or_error) != 0){
        fprintf(stderr, "Result errors\n");
      }
    }
    
    free(memory);
  }

  return NULL;

}

/**********************************************************************************/

int main(int argc, char **argv) {
  
  /********************* Dreturn resulfO NOT REMOVE SECTION - TOP     *********************/
  // Error check on number of arguments
  if(argc != 7){
    printf("usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
    return -1;
  }


  int port            = -1;
  char path[PATH_MAX] = "no path set\0";
  num_dispatcher      = -1;                               //global variable
  num_worker          = -1;                               //global variable
  queue_len           = -1;                               //global variable
  cache_len           = -1;                               //global variable


  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/
  /* TODO (A.I) - DONE (ALICE)
  *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length (6) cache_size
  */

  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);
  cache_len = atoi(argv[6]);

  /* TODO (A.II) - DONE(KATYA)
  *    Description:     Perform error checks on the input arguments
  *    Hints:           (1) port: {Should be >= MIN_PORT and <= MAX_PORT} | (2) path: {Consider checking if path exists (or will be caught later)}
  *                     (3) num_dispatcher: {Should be >= 1 and <= MAX_THREADS} | (4) num_workers: {Should be >= 1 and <= MAX_THREADS}
  *                     (5) queue_length: {Should be >= 1 and <= MAX_QUEUE_LEN} | (6) cache_size: {Should be >= 1 and <= MAX_CE}
  */
  if(port < MIN_PORT || port > MAX_PORT){
    printf("Invalid port\n");
    return -1;
  }
  if(strcmp(path, "") == 0){ //check if path exists
    printf("Path is empty\n");
    return -1;
  }
  if(num_dispatcher < 1 || num_dispatcher > MAX_THREADS){
    printf("Invalid number of dispatchers\n");
    return -1;
  }
  if(num_worker < 1 || num_worker > MAX_THREADS){
    printf("Invalid number of workers\n");
    return -1;
  }
  if(queue_len < 1 || queue_len > MAX_QUEUE_LEN){
    printf("Invalid queue length\n");
    return -1;
  }
  if(cache_len < 1 || cache_len > MAX_CE){
    printf("Invalid cache length\n");
    return -1;
  }
  /********************* DO NOT REMOVE SECTION - TOP    *********************/
  printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n", port, path, num_dispatcher, num_worker, queue_len, cache_len);
  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/


  /* TODO (A.III) - DONE (KATYA)
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use "web_server_log" as the name, what open flags do you want?
  */

  logfile = fopen("web_server_log", "w+");
  if(logfile == NULL){
    perror("File does not exist\n");
  }

  /* TODO (A.IV) - DONE (KATYA)
  *    Description:      Change the current working directory to server root directory
  *    Hint:             Check for error!
  */
  if(chdir(path) == -1){
    perror("Path does not exist\n");
  } //check for error


  /* TODO (A.V) - DONE (KATYA)
  *    Description:      Initialize cache  
  *    Local Function:   void    initCache();
  */
  initCache();


  /* TODO (A.VI) - DONE (KATYA)
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */

  init(port);

  /* TODO (A.VII) - DONE (KATYA)
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */
  for(int i = 0; i < MAX_THREADS; i++){
    threadID[i] = i;
  }
  for (int i = 0; i < num_dispatcher; i++){
    pthread_create(&(dispatcher_thread[i]), NULL, dispatch, &threadID[i]);

  }

  for(int i = 0; i < num_worker; i++){
    pthread_create(&(worker_thread[i]), NULL, worker, &threadID[i]);
  }
 
  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  int i;
  for(i = 0; i < num_worker; i++){
    fprintf(stderr, "Worker\t                       [%d] Started\n",i);
    if((pthread_join(worker_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join worker thread %d.\n", i);
    }
  }
  for(i = 0; i < num_dispatcher; i++){
    fprintf(stderr, "Dispatcher\t[%d] Started\n",i);
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join dispatcher thread %d.\n", i);
    }
  }
  fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
  deleteCache(); // delete the cache
  fflush(stdout);
  return 0;
}



