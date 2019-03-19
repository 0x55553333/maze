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
  for(int i = 0; i < height; ++i) {
    for(int j = 0; j < width; ++j) 
      fputc(mvinch(i, j) & A_CHARTEXT, fp);
    fputc('\n', fp);
  }
  fclose(fp);
}

void print_start_window()
{
  start_window = newwin(30, 30, 0, 0);
  make_border(start_window);
  wprintw(start_window, "MazeMaker v0.1 alpha\n");
  wprintw(start_window, "By @rjf\n");
  wprintw(start_window, "terminal size: %dx%d\n", w_width, w_height);
  wprintw(start_window, "height (0...100): ");
  wscanw(start_window, "%d", &height);
  wprintw(start_window, "\nwidth (0...100): ");
  wscanw(start_window, "%d", &width);
  height = height > 90 ? 90 : height;
  height = height < 10 ? 10 : height;
  width = width > 90 ? 90 : width;
  width = width < 10 ? 10 : width;
  height = w_height * height / 100;
  width = w_width * width / 100;
  cmd_height = w_height - height;
  cmd_width = w_width - width;
  wprintw(start_window, "width=%d, height=%d, ok?\n", width, height);
  wgetch(start_window);
  delwin(start_window);
}

int main(void)
{
  int y = 0, x = 0, do_print = 0;
  char bch, ch, bk = ' ';
  initscr();
  getmaxyx(stdscr, w_height, w_width); 
  noecho();
  print_start_window();

  while (1) {
    if (!do_print) mvprintw(y, x, "%c", bch);
    ch = getch();
    switch (ch) {
      case 'i': do_print = 1 - do_print;
        break;
      case 'Q':
        save();
      case 'q': 
        goto do_exit;
      case 'w':
        --y; y = y < 0 ? 0 : y;
        break;
      case 'a':
        --x; x = x < 0 ? 0 : x;
        break;
      case 's':
        ++y; y = y > height ? height : y;
        break;
      case 'd':
        ++x; x = x > width ? width : x;
      break;
      default: bk = ch;
    }
    bch = mvinch(y, x) & A_CHARTEXT;
    if (do_print) {
      mvprintw(y, x, "%c", bk);
      refresh();
    }
  }
  do_exit:
  endwin();
  return 0;
}

