/* CSCI-4061 Fall 2022
 * Group Member #1: Ruth Mesfin mesfi020@umn.edu
 * Group Member #2: Alice Zhang zhan6698@umn.edu
 * Group Member #3: Katya Borisova boris040@umn.edu
 */

#include "wrapper.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <signal.h>

#define MAX_TABS 100  

#define MAX_URL 100 
#define MAX_FAV 100
#define MAX_LABELS 100 //we dont check

int status = 0;                     // Status of controller process
int tabs = 0;                       // Number of tabs open

comm_channel comm[MAX_TABS];         // Communication pipes 
char favorites[MAX_FAV][MAX_URL];    // Maximum char length of a url allowed
int num_fav = 0;                     // Number of favorites

typedef struct tab_list {
  int free;
  int pid; 
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];  

/************************/
/* Simple tab functions */
/************************/

// return total number of tabs
int get_num_tabs () { 
  return tabs; 
}

// get next free tab index, otherwise returns -1 if there are no more free tabs
int get_free_tab () {
  if(get_num_tabs() + 1 == MAX_TABS){ 
    return -1;
  }else{
    for(int i = 1; i < MAX_TABS; i++){
      if(TABS[i].free == 1){
        return i;
      }
    }
  }
  return -1;
  
}

// init TABS data structure
void init_tabs () {
  int i;
  for (i=1; i<MAX_TABS; i++){
    TABS[i].free = 1;
  }
  TABS[0].free = 0;
}


/***********************************/
/* Favorite manipulation functions */
/***********************************/

// return 0 if favorite is ok, -1 otherwise
// both max limit, already a favorite (Hint: see util.h) return -1

// checks to make sure that the number of favorites does not exceed MAX_FAV
// and that the url is not already favorites
int fav_ok (char *uri) {
  if(num_fav >= MAX_FAV){
    alert("MAX FAVORITES LIMIT REACHED");
    return -1;
  } else if(on_favorites(uri) == 0){
    alert("FAV EXISTS");
    return -1;
  } else { 
    return 0; 
  }
}

// Add uri to favorites file and update favorites array with the new favorite
void update_favorites_file (char *uri) {
    FILE* favfd;
    char *fname = ".favorites";
    if((favfd = fopen(fname, "a+")) == NULL) {
      perror("Failed to open file\n");
      exit(1);
    }
    if(fprintf(favfd, "%s\n", uri )<= 0){
      perror("Could not write to favorites file\n");
    }
    sprintf(favorites[num_fav], "%s", uri);//adds to favorites array
    num_fav += 1;
    if(fclose(favfd) != 0){ 
      perror("Failed to close favorites file\n"); 
      exit(1);
    }

}

// set up favorites array
// do not need to check for duplicates since the controller
// in theory is the only one who has access to modify .favorites
void init_favorites (char *fname) {
  char url_read[1024];
  int i = 0;
  FILE* favfd;
  if(((favfd = fopen(fname, "a+")) == NULL)) {
    perror("Failed to open file\n");
    exit(1);
  }
  while(fgets(url_read, 1024, favfd)) { //reads the favorites file
    size_t length = strlen(url_read) - 1;
    if(url_read[length] == '\n'){ //removes the newline that fgets reads
      url_read[length] = '\0';
    }
    strncpy(favorites[i], url_read, strlen(url_read)); //saves the url in favorites
    i++;
  }
  num_fav = i;
  if(num_fav > MAX_FAV) {
    printf("Maximum number of favorites exceeded in favorites file\n");
    exit(1);
  }
  if(fclose(favfd) != 0){ 
      perror("Failed to close favorites file\n"); 
      exit(1);
    }
  return;
}

// Make fd non-blocking just as in class!
// Return 0 if ok, -1 otherwise
// provided code on from TAs post project release
int non_block_pipe (int fd) {
  int nFlags;
  
  if ((nFlags = fcntl(fd, F_GETFL, 0)) < 0)
    return -1;
  if ((fcntl(fd, F_SETFL, nFlags | O_NONBLOCK)) < 0)
    return -1;

  return 0;
}

/***********************************/
/* Functions involving commands    */
/***********************************/


// Checks if tab is bad and url violates constraints; if so, return.
// Otherwise, send NEW_URI_ENTERED command to the tab on inbound pipe

// checks to make sure the url length does not exceed max_url,
// the url does not have bad format, the url is not on blacklist, 
// the tab index is within the range of [1, 99] and that the
// requested tab is free
// if all that checks out, it creates a request for the tab and writes to pipe
void handle_uri (char *uri, int tab_index) {
  if(strlen(uri) > MAX_URL || bad_format(uri) == 1) {
    alert("BAD FORMAT");
  }else if(on_blacklist(uri) == 1){
    alert("BLACKLIST");
  } else if(tab_index >= MAX_TABS || tab_index <= 0){
    alert("BAD TAB");
  } else if(TABS[tab_index].free != 0){
    alert("BAD TAB");
  }
  else if(bad_format(uri) == 0 && on_blacklist(uri) == 0 && tab_index < MAX_TABS && tab_index > 0){
    struct req_t *req = malloc(sizeof(req_t));
    req_type r_type = NEW_URI_ENTERED;
    req->type = r_type;
    req->tab_index = tab_index;
    memcpy(req->uri, uri, strlen(uri));

    printf("URL selected is %s\n", uri);
    if(write(comm[tab_index].inbound[1], req, sizeof(req_t)) == -1){ 
      perror("Cannot write to pipe\n");
    }
    free(req);
  }
  return;
}

// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
void uri_entered_cb (GtkWidget* entry, gpointer data) {
  
  if(data == NULL) {	
    return;
  }
  // get the tab
  int t_index = query_tab_id_for_request(entry, data); 

  // get the url
  char *url = get_entered_uri(entry);

  handle_uri(url, t_index);
  return;
}

// Called when + tab is hit
// Check tab limit ... if ok then do some heavy lifting (see comments)
// Create new tab process with pipes

// obtains a new free tab and creates pipes for it, then ensures 
// that the read ends are non blocking
// lastly, it forks, and the child process executes the tab
// while the parent does tab bookkeeping
void new_tab_created_cb (GtkButton *button, gpointer data) {
  if (data == NULL) {
    return;
  } 

    int new_tab = get_free_tab();
    if(new_tab == -1){
      alert("Tab limit reached\n");
      return;
    }

      if(pipe(comm[new_tab].inbound)== -1){ 
        perror("Pipe creation failed\n");
        exit(1);
      }
      if(pipe(comm[new_tab].outbound)== -1){ 
        perror("Pipe creation failed\n");
        exit(1);
      }

      
      if(non_block_pipe(comm[new_tab].inbound[0]) == -1){
        printf("Non_blocking failed\n");
        exit(1);
      }
      if(non_block_pipe(comm[new_tab].outbound[0]) == -1){
        printf("Non_blocking failed\n");
        exit(1);
      }

      pid_t pid = fork();
      if(pid < 0){
        perror("Failed to fork\n");
        exit(1);
      } else if(pid == 0){
        char num[4];

        sprintf(num, "%d", new_tab);
        char fds[16];
        sprintf(fds, "%d %d %d %d", comm[new_tab].inbound[0], comm[new_tab].inbound[1], comm[new_tab].outbound[0], comm[new_tab].outbound[1]);
        if(execl("./render", "render", num, fds, NULL) == -1){ 
          perror("Execl failed\n");
        }
      } else {
        TABS[new_tab].free= 0; 
        tabs +=1; 
      }
    return;
  
}

// This is called when a favorite is selected for rendering in a tab
// Hint: you will use handle_uri ...
// However: you will need to first add "https://" to the uri so it can be rendered
// as favorites strip this off for a nicer looking menu

void menu_item_selected_cb (GtkWidget *menu_item, gpointer data) {

  if (data == NULL) {
    return;
  }

  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));


  char uri[MAX_URL];
  sprintf(uri, "https://%s", basic_uri);

//gets the tab index and then handles the request
  int tab_index = query_tab_id_for_request(menu_item, data);

  handle_uri(uri, tab_index);

  return;
}

// BIG CHANGE: the controller now runs an loop so it can check all pipes
// runs the controller process and handles 3 types of events with tabs:
  // 1. when a tab should die
  // 2. when a tab is dead
  // 3. when a tab is added to .favorites
int run_control() {
  browser_window * b_window = NULL;
  int nRead;
  req_t req;
  
  //Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);
  
  while (1) {
    process_single_gtk_event();
    for (int i=0; i<MAX_TABS; i++) { 
      if (TABS[i].free == 0){

      nRead = read(comm[i].outbound[0], &req, sizeof(req_t)); 
      if(nRead == -1 && errno != EAGAIN){ 
        perror("Failed to read\n");
        exit(1);
     }else if(nRead == -1){
       continue;
      }else{ 
        if(req.type == PLEASE_DIE){ // Case 1: PLEASE_DIE 
          
          req_t die; 
          req_type r_type = PLEASE_DIE;
          die.type = r_type;
          die.tab_index = i;
          
          
          for(int j=0; j<MAX_TABS; j++){  
            if(TABS[j].free == 0){
              if(write(comm[j].inbound[1], &die, sizeof(req_t)) == -1){ 
                perror("Cannot write to pipe\n");
              }
              tabs -= 1;
              TABS[j].free = 1;
            }
          }
          exit(1);
        } else if(req.type == TAB_IS_DEAD){ // Case 2: TAB_IS_DEAD
          tabs -= 1;
          TABS[i].free = 1;
          wait(NULL); //needs error checking?
        } else if(req.type == IS_FAV){ // Case 3: IS_FAV    
          int is_ok = fav_ok(req.uri);
          if(is_ok == 0){
            update_favorites_file(req.uri);
            add_uri_to_favorite_menu (b_window, req.uri);
          }
          
        }
      }
      }
      
      
    }
    usleep(1000); 
  }
  return 0;
}

int main(int argc, char **argv) 
{
  if (argc != 1) { 
    fprintf (stderr, "browser <no_args>\n");
    exit (0);
  }

  // initializations
  init_tabs ();
  init_favorites(".favorites"); 
  init_blacklist(".blacklist"); 

  // fork controller
  pid_t pid = fork();
  if(pid < 0){
    perror("Failed to fork\n");
    exit(1);
  } else if(pid == 0){
      if((pipe(comm[0].inbound)) == -1){ 
      perror("Pipe creation failed\n");
      exit(1);
    }
    if((pipe(comm[0].outbound)) == -1){
      perror("Pipe creation failed\n");
      exit(1);
    }
    // child creates a pipe for itself comm[0]
    int block_outread = non_block_pipe(comm[0].outbound[0]); 
    int block_inread = non_block_pipe(comm[0].inbound[0]); 
    if(block_outread == -1 || block_inread ==-1){
      printf("Non_blocking failed\n");
      exit(1);
    }else{
      // then calls run control
      run_control();
    }
    exit(status);
  } else { 
    // parent waits
    pid_t w = wait(&status);
    if(w == -1) {
      perror("Wait failed\n");
      exit(1);
    }

  }
  return 0;
}
