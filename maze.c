#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <ncurses.h>
#define make_border(x) do {    wborder(x, '|', '|', '-', '-', '+', '+', '+', '+'); } while (0)
int w_width, w_height;
int width, height, cmd_width, cmd_height, b_read_in;
WINDOW *play_window, *cmd_window, *start_window;

void save(void)
{
  FILE *fp = fopen("out.maze", "w");
  for(int i = 1; i < height - 1; ++i) {
    for(int j = 1; j < width - 1; ++j) 
      fputc(mvwinch(play_window, i, j) & A_CHARTEXT, fp);
    fputc('\n', fp);
  }
  fclose(fp);
}

void print_start_window()
{ int y, x;
  echo();
  start_window = newwin(30, 30, 0, 0);
  make_border(start_window);
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
{
  play_window = newwin(height, width, 0, 0);
  cmd_window = newwin(cmd_height, cmd_width, 0, width);
  make_border(play_window);
  make_border(cmd_window);
  int cmd_y, cmd_x;
  getyx(cmd_window, cmd_y, cmd_x);
  mvwprintw(cmd_window, cmd_y + 1, cmd_x + 1, "[CMD]");
  wrefresh(play_window);
  wrefresh(cmd_window);
}

void free_main_windows()
{
  delwin(play_window);
  delwin(cmd_window);
}

int main(void)
{
  int y = 1, x = 1, do_print = 0;
  char bch, ch, bk = ' ';
  initscr();
  noecho();
  getmaxyx(stdscr, w_height, w_width); 
  print_start_window();
  create_main_windows();
  while (1) {
    ch = wgetch(play_window);
    switch (ch) {
      case 'i': do_print = 1 - do_print;
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
      default: bk = ch;
    }
    bch = mvwinch(play_window, y, x) & A_CHARTEXT;
    if (do_print) {
      mvwprintw(play_window, y, x, "%c", bk);
    } else mvwprintw(play_window, y, x, "%c", bch);
    wrefresh(play_window);
  }
  do_exit:
  free_main_windows();
  endwin();
  return 0;
}

