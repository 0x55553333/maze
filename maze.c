#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <string.h>
int w_width, w_height;
int width, height, cmd_width, cmd_height, b_read_in;
WINDOW *play_window, *cmd_window, *start_window;
char * menu[] = {"Write2file", "Showcreds", "Showhelp", "Toggleback", "Quitgame"};
char * menu_text[] = {"export to a given file", "show credits", "list all commands", "return to game", "return to terminal"};
ITEM **menubars;
MENU * cmd_menu;

FIELD *filename_field;
FORM *filename_form;

inline static void clear_window_contents(WINDOW* w)
{
  wclear(w);
  box(w, 0, 0);
}

void make_file_form()
{
  filename_field = new_field(1, 10, 4, 18, 0, 0); 
  set_field_back(filename_field, A_UNDERLINE);  /* Print a line for the option  */
  field_opts_off(filename_field, O_AUTOSKIP);   /* Don't go to next field when this */
  filename_form = new_form(&filename_field);
  set_form_win(filename_form, cmd_window);
}

char * trim(const char * in)
{
  size_t new_size = 0, in_size = strlen(in), ptr = 0;
  for(size_t i = 0; i < in_size; ++i) 
    if (in[i] != ' ' && in[i] != '\t') 
      ++new_size;
  char * out = malloc(new_size + 1);
  memset(out, '\0', new_size);
  for(size_t i = 0; i < in_size; ++i) 
    if (in[i] != ' ' && in[i] != '\t') 
      out[ptr++] = in[i];
  return out;
}

int blk_color;
void save(void)
{
  FILE *fp = fopen("out.maze", "w");
  for(int i = 1; i < height - 1; ++i) {
    for(int j = 1; j < width - 1; ++j) {
      if (mvwinch(play_window, i, j) & A_COLOR) 
        fputc('0', fp);
      else
        fputc(mvwinch(play_window, i, j) & A_CHARTEXT, fp);
    }
    fputc('\n', fp);
  }
  fclose(fp);
}

void save2file(FILE *fp)
{
  for(int i = 1; i < height - 1; ++i) {
    for(int j = 1; j < width - 1; ++j) {
      fputc(mvwinch(play_window, i, j) & A_CHARTEXT, fp);
    }
    fputc('\n', fp);
  }
}

void print_start_window()
{ int y, x;
  echo();
  start_window = newwin(30, 30, 0, 0);
  box(start_window, 0, 0);
  getyx(start_window, y, x);
  ++y; ++x;
  mvwprintw(start_window, y, x, "MazeMaker v0.1 alpha");
  ++y;
  mvwprintw(start_window, y, x, "By @rjf");
  ++y;
  mvwprintw(start_window, y, x, "terminal size: %dx%d", w_width, w_height);
  ++y;
  mvwprintw(start_window, y, x, "height (0...100): ");
  wscanw(start_window, "%d", &height);
  ++y;
  mvwprintw(start_window, y, x, "width (0...100): ");
  wscanw(start_window, "%d", &width);
  ++y;
  height = height > 90 ? 90 : height;
  height = height < 10 ? 10 : height;
  width = width > 90 ? 90 : width;
  width = width < 10 ? 10 : width;
  height = w_height * height / 100;
  width = w_width * width / 100;
  cmd_height = w_height - height;
  cmd_width = w_width - width;
  ++y;
  mvwprintw(start_window, y, x, "width=%d, height=%d, ok? [y/n]", width, height, cmd_width, cmd_height );
  if (wgetch(start_window) != 'y') {
    delwin(start_window);
    endwin();
    exit(1);
  } 
  noecho();
  delwin(start_window);
}

void create_main_windows()
{ char * menu[] = {"Write2file", "Showcreds", "Showhelp", "Toggleback", "Quitgame"};
  play_window = newwin(height, width, 0, 0);
  cmd_window = newwin(cmd_height, cmd_width, 0, width);
  box(play_window, 0, 0);
  box(cmd_window, 0, 0);
  int cmd_y, cmd_x;
  getyx(cmd_window, cmd_y, cmd_x);
  menubars = (ITEM**) calloc(sizeof(menu)/sizeof(char*), sizeof(ITEM*));
  for(int i = 0; i < sizeof(menu)/sizeof(char*); ++i) {
    menubars[i] = new_item(menu[i], menu_text[i]);
  }
  cmd_menu = new_menu(menubars);
  set_menu_win(cmd_menu, cmd_window);
  set_menu_sub(cmd_menu, derwin(cmd_window, 6, 38, 3, 1));
  set_menu_mark(cmd_menu, " * ");
  post_menu(cmd_menu);
  make_file_form();
  wrefresh(play_window);
  wrefresh(cmd_window);
}

void free_main_windows()
{
  unpost_menu(cmd_menu);
  for(int i = 0; i < sizeof(menu)/sizeof(char*); ++i) free_item(menubars[i]);
//  free_field(filename_field);
//  free_form(filename_form);
  free_menu(cmd_menu);
  free(menubars);
  delwin(play_window);
  delwin(cmd_window);
  endwin();
}

void handle_filename_form()
{ int ch;
  unpost_menu(cmd_menu);
  post_form(filename_form);
  while (1) {
    ch = wgetch(cmd_window);
    switch (ch) {
      case 'q': goto go_back; 
      case (char)KEY_ENTER: 
      case '\n':
        {
          form_driver(filename_form, REQ_VALIDATION); // force sync field buffer
          char * sanitized_filename = trim(field_buffer(filename_field, 0));
          FILE *fp = fopen(sanitized_filename, "w");
          save2file(fp);
          fclose(fp);
          wprintw(cmd_window, "\nwritten to %s\n", sanitized_filename);
          wprintw(cmd_window, "Press any key to go back...");
          free(sanitized_filename);
          wgetch(cmd_window);
          goto go_back;
        }
      default: form_driver(filename_form, ch);
    } 
  }
go_back:
  unpost_form(filename_form);
  clear_window_contents(cmd_window);
  post_menu(cmd_menu);
}

void handle_cmd_window()
{
  int idx;
  char ch;
  while (1) {
    ch = wgetch(cmd_window);
    switch (ch) {
      case 'Q':
      case 'q':
        free_main_windows();
        exit(1);
      case 't': return;
      case 'w': 
        menu_driver(cmd_menu, REQ_UP_ITEM);
        break;
      case 's':
        menu_driver(cmd_menu, REQ_DOWN_ITEM);
        break;
      case (char)KEY_ENTER:
      case '\n':
        idx = item_index(current_item(cmd_menu));
        switch (idx) {
          case 0: /* Write2file */
            handle_filename_form(); break;
          case 1: /* Showcreds */
            unpost_menu(cmd_menu); 
            mvwprintw(cmd_window, 1, 1, "MazeMaker v0.1a"); 
            mvwprintw(cmd_window, 2, 1, "Press any key to go back");
            wgetch(cmd_window);
            clear_window_contents(cmd_window);
            post_menu(cmd_menu);
            break;
          case 2: /* Showhelp */
            unpost_menu(cmd_menu);
            mvwprintw(cmd_window, 1, 1, "Q\tSave & quit");
            mvwprintw(cmd_window, 2, 1, "q\tQuit");
            mvwprintw(cmd_window, 3, 1, "b\tToggle/untoggle block");
            mvwprintw(cmd_window, 4, 1, "w\tMove cursor up");
            mvwprintw(cmd_window, 5, 1, "s\tMove cursor down");
            mvwprintw(cmd_window, 6, 1, "a\tMove cursor left");
            mvwprintw(cmd_window, 7, 1, "d\tMove cursor right");
            mvwprintw(cmd_window, 8, 1, "z\tMove cursor southeast");
            mvwprintw(cmd_window, 9, 1, "x\tMove cursor northwest");
            mvwprintw(cmd_window, 10, 1, "c\tMove curosr northeast");
            mvwprintw(cmd_window, 11, 1, "v\tMove cursor southwest");
            mvwprintw(cmd_window, 12, 1, "t\tToggle between windows");
            mvwprintw(cmd_window, 13, 1, "i\tStart/end drawing");
            mvwprintw(cmd_window, 14, 1, "Use any other key to specify color");
            wgetch(cmd_window);
            clear_window_contents(cmd_window);
            post_menu(cmd_menu);
            break;
          case 3: /* Toggleback */
            return;
          case 4: free_main_windows(); exit(0); break;
        }
        printw("%d toggled\n", idx);
    }
    wrefresh(cmd_window);
  }
}

void loop()
{
  int y = 1, x = 1, do_print = 0;
  char bch, ch, bk = ' ';
    while (1) {
    ch = wgetch(play_window);
    switch (ch) {
      case 'i': do_print = 1 - do_print;
                if (!do_print && blk_color) wattroff(play_window, COLOR_PAIR(1));
                else if (blk_color) wattron(play_window, COLOR_PAIR(1));
        break;
      case 'Q':
        save();
      case 'q':
        goto do_exit;
      case 'w':
        --y; y = y < 1 ? 1 : y;
        break;
      case 'a':
        --x; x = x < 1 ? 1 : x;
        break;
      case 's':
        ++y; y = y > height-2 ? height-2 : y;
        break;
      case 'd':
        ++x; x = x > width-2 ? width-2 : x;
        break;
      case 'b':
        blk_color = 1 - blk_color;
        if (!blk_color) wattroff(play_window, COLOR_PAIR(1));
        else wattron(play_window, COLOR_PAIR(1));
        break;
      case 't':
        handle_cmd_window(); break;
      case 'z':
        ++x; ++y; break;
      case 'x':
        --x; --y; break;
      case 'c':
        ++x; --y; break;
      case 'v':
        --x; ++y; break;
      default:
        wattroff(play_window, COLOR_PAIR(1));
        bk = ch;
    }
    bch = mvwinch(play_window, y, x) & A_CHARTEXT;
    if (do_print) {
      if (!blk_color)
        mvwprintw(play_window, y, x, "%c", bk);
    } else mvwprintw(play_window, y, x, "%c", bch);
    wrefresh(play_window);
  }
  do_exit:
  free_main_windows();
}

int main(void)
{
  int y = 1, x = 1, do_print = 0;
  char bch, ch, bk = ' ';
  initscr(); start_color();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  attroff(COLOR_PAIR(1));
  noecho();
  getmaxyx(stdscr, w_height, w_width); 
  print_start_window();
  create_main_windows();
  loop();
  return 0;
}

