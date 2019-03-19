#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <ncurses.h>
int width, height, b_read_in;

/* 
  Maze generator
  ----------------
  > Not yet completed. 
  > Todo: 
*/

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

int main(void)
{
  int y = 0, x = 0, do_print = 0;
  char bch, ch, bk = ' ';
  
  fputs(">>>>>\tMazeMaker v0.1alpha\t<<<<<\n", stdout);
  fputs(">>>>>\t\tHeight: ", stdout);
  scanf("%d", &height);
  fputs(">>>>>\t\tWidth: ", stdout);
  scanf("%d", &width);
  initscr();
  noecho();  
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

