#include <stdio.h>
#include <ncurses.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct directory_ent {
	char name[256];
	int type;
} dir_ent_t;


typedef struct directory {
	dir_ent_t * entries;
	int num_entries;
} directory_t;

#define BACKGROUND_COLOR COLOR_YELLOW
#define FOREGROUND_COLOR 0
#define LINENUM_COLOR COLOR_WHITE
#define TEXT_COLOR COLOR_WHITE

/*
		// DT_BLK      This is a block device.
		// DT_CHR      This is a character device.
		// DT_DIR      This is a directory.
		// DT_FIFO     This is a named pipe (FIFO).
		// DT_LNK      This is a symbolic link.
		// DT_REG      This is a regular file.
		// DT_SOCK     This is a UNIX domain socket.
		// DT_UNKNOWN  The file type could not be determined.
*/


directory_t * loadDirectory() {
	directory_t * dir = malloc(sizeof(directory_t));
	int i = 0, total = 0;
	DIR *dp;
	struct dirent *ep;

	dp = opendir ("./");
	if (dp == NULL)
	{
		perror("Couldn't open the directory");
		return NULL;
	}

	while ((ep = readdir(dp)) != NULL) {
		total++;
	}

	dir->num_entries = total;
	seekdir(dp, 0);
	
	dir->entries = malloc(sizeof(dir_ent_t) * total);
	
	while ((ep = readdir(dp)) != NULL){
		dir->entries[i].type = ep->d_type;
		strcpy(dir->entries[i].name, ep->d_name);
		i++;
	}
	
	// Cleanup
	closedir (dp);
	
	return dir;
	
}

int print_page(WINDOW *w_file, FILE *fp, int start_row) {
    int max_x, max_y;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
	int lines = 0;
	int total_lines = 0;

	while (EOF != (fscanf(fp, "%*[^\n]"), fscanf(fp, "%*c"))) {
    	total_lines++;
	}

	fseek(fp, 0, SEEK_SET);	

	int line_temp = total_lines;
	int line_width = 0;

	while (line_temp > 0) {
		line_width++;
		line_temp /= 10;
	}

    getmaxyx(stdscr, max_y, max_x);
    fseek(fp, 0, SEEK_SET);
    for (int k = 0; k < start_row; k++)
    {
        read = getline(&line, &len, fp);
		if (read == -1)
		{
	    	break;
		}
    }

    for (int p = 0; p < max_y-6; p++) 
    {
		read = getline(&line, &len, fp);
		if (read == -1) {
            break;
        }

		wattron(w_file, COLOR_PAIR(1));
		int temp = p+start_row;
		int lines = 0;

		while (temp > 0) {
			lines++;
			temp /= 10;
		}
		
		for (int z = 0; z < line_width-lines; z++)
		{
			wprintw(w_file, " ");
		}

		wprintw(w_file, " %d ", p+start_row);
		wattroff(w_file, COLOR_PAIR(1));
        wprintw(w_file, "%s", line);
    }
    refresh();
}


void paintList(WINDOW * w_list, directory_t * dir, int i)
{	
	werase(w_list);	
	wbkgd(w_list, COLOR_PAIR(3));
	for(int k=0; k<dir->num_entries; k++)
    {
		if (dir->entries[k].type == DT_DIR) wattron(w_list, COLOR_PAIR(1));	
		if( i == k ) wattron( w_list, A_STANDOUT ); // highlights the first item. 
		mvwprintw(w_list, k+1, 2, "%s", dir->entries[k].name);
		wattroff(w_list, COLOR_PAIR(1));
		wattroff(w_list, A_STANDOUT );
    }	
}

void paintFile(WINDOW * w_file, directory_t * dir, int i, int current_col) 
{			
	FILE *fp;
	wbkgd(w_file, COLOR_PAIR(3));
	fp = fopen(dir->entries[i].name, "r");
	werase(w_file);
	if (fp != NULL) {
    	print_page(w_file, fp, current_col);
	}
}

void paintPath(WINDOW * w_path, directory_t * dir)
{
 	char cwd[PATH_MAX];
	werase(w_path);
	wbkgd(w_path, COLOR_PAIR(3)); // Set colors
	getcwd(cwd, sizeof(cwd));
	mvwprintw(w_path, 1, 1, "%s", cwd);
}


int main()
{
  	WINDOW *w_list, *w_file, *w_path;
	directory_t * dir = loadDirectory(); 

 	int ch, i = 0, width = 7;
	int curr_x, curr_y;
  	int goto_prev = FALSE, y, x;
  	int wid = 32;

	initscr(); // initialize Ncurses
	
	w_list = newwin(LINES - 6, wid-1, 5, 1 ); // create a new window 
	w_file = newwin(LINES - 6, COLS - wid - 2, 5, wid + 1);
	w_path = newwin(3, COLS-2, 1, 1);

	int current_col = 0;
	noecho(); // disable echoing of characters on the screen
	keypad(w_list, TRUE); // enable keyboard input for the window.
		     
  	curs_set( 0 ); // hide the default screen cursor.

	start_color();

	init_pair(1, LINENUM_COLOR, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, BACKGROUND_COLOR);
	init_pair(3, TEXT_COLOR, FOREGROUND_COLOR);
	init_pair(4, COLOR_WHITE, COLOR_BLACK);
	
	
	
	do 
	{
		switch( ch ) {
			case KEY_UP:
        		i--;
        		i = ( i<0 ) ? dir->num_entries-1 : i;
        		break;
			case KEY_DOWN:
        		i++;
        		i = ( i>=dir->num_entries ) ? 0 : i;
				break;
			case 'q':
				exit(0);
				break;
			case 'j':
				current_col = (current_col > 0) ? current_col-1 : current_col;
				break;
			case 'k':
				current_col++;
				break;
			case KEY_RIGHT:
				if (dir->entries[i].type == DT_DIR) {
					chdir(dir->entries[i].name);
					dir = loadDirectory();
					i = 0;
					current_col = 0;
				}
				break;
		}

		bkgd(COLOR_PAIR(2));	
		paintFile(w_file, dir, i, current_col);
		paintList(w_list, dir, i);
		paintPath(w_path, dir);
    	
    	wrefresh( w_list );
    	wrefresh( w_file );
		wrefresh( w_path );
	
	    //refresh();
	} while ((ch = wgetch(w_list)) != 'q');

  delwin(w_list);
  delwin(w_file);
  endwin();
}
