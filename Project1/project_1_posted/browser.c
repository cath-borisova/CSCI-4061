/* CSCI-4061 Fall 2022
 * Group Member #1: Ruth Mesfin mesfi020@umn.edu
 * Group Member #2: Alice Zhang zhan6698@umn.edu
 * Group Member #3: Katya Borisova boris040@umn.edu
 */

#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <ctype.h> // added for the use of isspace()

/* === PROVIDED CODE === */
// Function Definitions
void new_tab_created_cb(GtkButton *button, gpointer data);
int run_control();
int on_blacklist(char *uri);
int bad_format (char *uri);
void uri_entered_cb(GtkWidget* entry, gpointer data);
void init_blacklist (char *fname);

/* === PROVIDED CODE === */
// Global Definitions
#define MAX_TAB 100                 //Maximum number of tabs allowed
#define MAX_BAD 1000                //Maximum number of URL's in blacklist allowed
#define MAX_URL 100                 //Maximum char length of a url allowed

int total_num_tabs = 0;             //counts the number of tabs
char blacklist_contents[1000][101]; //2D character array holding all blacklist urls
int blacklist_length = 0;           //length of the blacklist
int status = 0;                     //status of processes
pid_t tab_processes[100];           //process ids of child processes to controller process

/* === PROVIDED CODE === */
/*
 * Name:		          new_tab_created_cb
 *
 * Input arguments:	
 *      'button'      - whose click generated this callback
 *			'data'        - auxillary data passed along for handling
 *			                this event.
 *
 * Output arguments:   void
 * 
 * Description:        This is the callback function for the 'create_new_tab'
 *			               event which is generated when the user clicks the '+'
 *			               button in the controller-tab. The controller-tab
 *			               redirects the request to the parent (/router) process
 *			               which then creates a new child process for creating
 *			               and managing this new tab.
 */
void new_tab_created_cb(GtkButton *button, gpointer data) 
{
  return;
}
 
/* === PROVIDED CODE === */
/*
 * Name:                run_control
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminates.
 * 
 */
int run_control()
{
  // (a) Init a browser_window object
	browser_window * b_window = NULL;

	// (b) Create controller window with callback function
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		       G_CALLBACK(uri_entered_cb), &b_window);

	// (c) enter the GTK infinite loop
	show_browser();
	return 0;
}

/* === STUDENTS IMPLEMENT=== */
/*  Description:                 Opens file that contains the list of forbidden uris then 
                                 strips the given uri so that it can correctly asses if uri is on black list.
    Function: on_blacklist  --> "Check if the provided URI is in th blacklist"
    Input:    char* uri     --> "URI to check against the blacklist"
    Returns:  True  (1) if uri in blacklist
              False (0) if uri not in blacklist
    Hints:
            (a) File I/O
            (b) Handle case with and without "www." (see writeup for details)
            (c) How should you handle "http://" and "https://" ??
*/ 
int on_blacklist (char *uri) {
  
  FILE * fp;
  if((fp = fopen ("blacklist", "r")) == NULL){  
    perror("Failed to open file");
    exit(1);
  }
  char* stripped_uri = uri;
  if(strstr(uri, "www.")) { // with/without "www." case
    stripped_uri += 4;
  }
 
  if(strstr(uri, "http://")) {// with/without "http://" case
    stripped_uri += 7;
  }
  else if(strstr(uri, "https://")){// with/without "https://" case
    stripped_uri += 8;
  }
  for(int i = 0; i < blacklist_length; i++){
    if(strstr(blacklist_contents[i], stripped_uri)){
      if(fclose(fp) != 0){
        perror("Failed to close file");
        exit(1);
      }
      return 1;
    }
  }
  if(fclose(fp) != 0){
    perror("Failed to close file");
    exit(1);
  }
  return 0;
}

/* === STUDENTS IMPLEMENT=== */
/*  Description:                Checks the given uri for if it has http:// or https:// to 
                                determine if it is in bad or good format
    Function: bad_format    --> "Check for a badly formatted url string"
    Input:    char* uri     --> "URI to check if it is bad"
    Returns:  True  (1) if uri is badly formatted 
              False (0) if uri is well formatted
    Hints:
              (a) String checking for http:// or https://
*/
int bad_format (char *uri) {
  char* check1 = "http://";
  char* check2 = "https://";

  // isspace checks for whitespace at the beginning of the url
  if ((strstr(uri, check1) != NULL || strstr(uri, check2) != NULL) && isspace(*uri) == 0) {
    return 0;
  }
  
  return 1;
}
/* === STUDENTS IMPLEMENT=== */
/*
 * Name:                uri_entered_cb
 *
 * Input arguments:     
 *                      'entry'-address bar where the url was entered
 *			                'data'-auxiliary data sent along with the event
 *
 * Output arguments:     void
 * 
 * Function:             When the user hits the enter after entering the url
 *			                 in the address bar, 'activate' event is generated
 *			                 for the Widget Entry, for which 'uri_entered_cb'
 *			                 callback is called. Controller-tab captures this event
 *			                 and sends the browsing request to the router(/parent)
 *			                 process.
 * Description:          First checks if function is even given data or url to continue. 
                         Then gets the 
                         actual URL entered from entry. Next the function checks if given URL is good format, 
                         isnt on the black list and that the program hasn't surpased max number of tabs to render a new tab. 
                         To render a new tab we implemented a fork process and execl statment.
 * Hints:
 *                       (a) What happens if data is empty? No Url passed in? Handle that
 *                       (b) Get the URL from the GtkWidget (hint: look at wrapper.h)
 *                       (c) Print the URL you got, this is the intermediate submission
 *                       (d) Check for a bad url format THEN check if it is in the blacklist
 *                       (e) Check for number of tabs! Look at constraints section in lab
 *                       (f) Open the URL, this will need some 'forking' some 'execing' etc. 
 * !!
 */
void uri_entered_cb(GtkWidget* entry, gpointer data)
{
  
  char *url = get_entered_uri(entry);
  char *empty = "";
  if(data == NULL || strcmp(empty, url) == 0){
    char *no_url = "ERROR No URL entered.";
    alert(no_url);
    return;
  }
  if(strlen(url) > MAX_URL) {
    char *max_url_len = "ERROR Maximum URL lenth exceeded.";
    alert(max_url_len);
  }
  else {
    // Intermediate submission: 
  printf("URL entered: %s\n", url);

  //format check
  int format = bad_format(url);
  if(format == 1){
    char *bad_format_error = "Bad format!";
    alert(bad_format_error);
  } else {

    //blacklist check
    int allowed = on_blacklist(get_entered_uri(entry));
    if(allowed == 1){
      char *blacklist_error = "URL is on Black list";
      alert(blacklist_error);
    } else {
      
      //number tab check
      if(total_num_tabs >= MAX_TAB){
        char *max_tabs_error = "Too many tabs";
        alert(max_tabs_error);
      } else {

          //fork process
          pid_t pid = fork();
          tab_processes[total_num_tabs] = pid;
          total_num_tabs += 1;
          if(pid == -1){
            perror("Failed to fork");
            exit(1);
          } else if(pid == 0){
              //stores # tabs in a string (size 4 to accomodate 3 digits + newline)
              char str[4];
              sprintf(str, "%d", total_num_tabs);
              //execl statement to call render for a new tab
              execl("./render", "render", str, get_entered_uri(entry), NULL);
              exit(0);
          }
        }
      }

    }
  }
  
  return;
}
/* === STUDENTS IMPLEMENT=== */
/*  Description:                  init_blacklist() opens the blacklist file in order to read all of its
                                  contents and stores them in a global array (blacklist_contents). 
                                  "www." is also stripped from all the urls.
    Function: init_blacklist --> "Open a passed in filepath to a blacklist of url's, read and parse into an array"
    Input:    char* fname    --> "file path to the blacklist file"
    Returns:  void
    Hints:
            (a) File I/O (fopen, fgets, etc.)
            (b) If we want this list of url's to be accessible elsewhere, where do we put the array?
*/
void init_blacklist (char *fname) {
  FILE *fd = NULL;
  int i = 0;
  char blacklist[101];
  
  if((fd = fopen(fname, "r")) == NULL){
    perror("Failed to open file");
    exit(1);
  } 

  while(fgets(blacklist, 101, fd)) {
    if(strstr(blacklist, "www.")){
      char* actual = blacklist + 4;
      strncpy(blacklist_contents[i], actual, strlen(blacklist) - 4);
    } else {
      strncpy(blacklist_contents[i], blacklist, strlen(blacklist));
    }
    i++;
  }
  blacklist_length = i;
  
  if(fclose(fd) != 0){
    perror("Failed to close file");
    exit(1);
  }
  return;
}

/* === STUDENTS IMPLEMENT=== */
/*  Description: Checks to make sure the blacklist file was entered as an argument,
                then forked the controller. Before the controller exits, it kills
                all of the open tabs and then waits on them. Lastly, the main program 
                waits on the controller.
    Function: main 
    Hints:
            (a) Check for a blacklist file as argument, fail if not there [PROVIDED]
            (b) Initialize the blacklist of url's
            (c) Create a controller process then run the controller
                (i)  What should controller do if it is exited? Look at writeup (KILL! WAIT!)
            (d) Parent should not exit until the controller process is done 
*/
int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "browser <blacklist_file>\n");
    exit(1);
  }
  init_blacklist("blacklist");

  // print error and exit program if blacklist length exceeds limit
  // we decided to exit the program rather than use first MAX_BLACKLIST #
  // of URLs since the writeup let us choose
  if(blacklist_length > MAX_BAD) {
    printf("ERROR maximum blacklist length exceeded\n");
    exit(1);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("Failed to fork");
    exit(1);
  } else if(pid == 0) {
    run_control();
    for(int i = 0; i < total_num_tabs; i++){
      tab_processes[i] = kill(tab_processes[i], SIGKILL);
      tab_processes[i] = wait(&status);
    }
    exit(status);
  } else {
    wait(&status);
    printf("Killed\n");
  }
  return 0;
}
