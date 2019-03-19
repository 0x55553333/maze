#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <ncurses.h>
#include <menu.h>
int w_width, w_height;
int width, height, cmd_width, cmd_height, b_read_in;
WINDOW *play_window, *cmd_window, *start_window;
char * menu[] = {"Write2file", "Showcreds", "Showhelp", "Toggleback", "Quitgame"};
ITEM **menubars;
MENU * cmd_menu;
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
    menubars[i] = new_item(menu[i], menu[i]);
  }
  cmd_menu = new_menu(menubars);
  set_menu_win(cmd_menu, cmd_window);
  set_menu_sub(cmd_menu, derwin(cmd_window, 6, 38, 3, 1));
  set_menu_mark(cmd_menu, " * ");
  post_menu(cmd_menu);
  wrefresh(play_window);
  wrefresh(cmd_window);
}

void free_main_windows()
{
  unpost_menu(cmd_menu);
  for(int i = 0; i < sizeof(menu)/sizeof(char*); ++i) free_item(menubars[i]);
  free_menu(cmd_menu);
  free(menubars);
  delwin(play_window);
  delwin(cmd_window);
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
      case KEY_ENTER:
      case '\n':
        idx = item_index(current_item(cmd_menu));
        printw("%d toggled\n", idx);
    }
    wrefresh(cmd_window);
  }
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
        handle_cmd_window();
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
  endwin();
  return 0;
}

