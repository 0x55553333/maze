#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <string.h>
#include <assert.h>
int w_width, w_height;
int width, height, cmd_width, cmd_height, b_read_in;
WINDOW *play_window, *cmd_window, *start_window;
char * menu[] = {"Write2file", "Showcreds", "Showhelp", "Toggleback", "Quitgame", "Floodfill"};
char * menu_text[] = {"export to a given file", "show credits", "list all commands", "return to game", "return to terminal", "Fill a connected region"};
char * floodfill_form_text[]  = {"start_x:", "start_y:", "target:", "replacement:", "not:", "8way:"};
ITEM **menubars;
MENU * cmd_menu;
FIELD *filename_field;
FORM *filename_form;
struct Node {
  struct Queue* q;
  struct Node* next;
  int i, j;
};

struct Queue {
  struct Node* head;
  struct Node* end;
  int size;
};

struct PriorityQueue {
  struct PriorityQueue * child;
  struct PriorityQueue * left;
  struct PriorityQueue * right;
  struct PriorityQueue * root;
  int size, i, j, w;
};

struct Node* make_node(struct Queue *q, int i, int j)
{
  struct Node *n = malloc(sizeof(struct Node));
  n->q = q;
  n->next = NULL;
  n->i = i; n->j = j;
  return n;
}

void free_node(struct Node* n)
{
  free(n);
}

struct PriorityQueue* makePriorityQueue(int i, int j, int w)
{
  struct PriorityQueue * pq = malloc(sizeof(struct PriorityQueue));
  pq->size = 1;
  pq->left = pq->right = NULL;
  pq->root = pq;
  pq->i = i; pq->j = j; pq->w = w;
  return pq;
}

struct PriorityQueue* link_pq(struct PriorityQueue* lhs, struct PriorityQueue* rhs)
{
  if (lhs->w <= rhs->w) {
    ++lhs->size;
    rhs->root = lhs->root; 
    rhs->right = lhs->child;
    rhs->left = lhs;
    lhs->child = rhs;
    if (rhs->right != NULL) rhs->right->left = rhs;
    return rhs;
  } else {
    ++rhs->size;
    lhs->root = rhs->root;
    lhs->right = rhs->child;
    lhs->left = rhs;
    rhs->child = lhs;
    if (lhs->right != NULL) lhs->right->left = lhs;
  }
  return lhs;
}

struct PriorityQueue* find_min(struct PriorityQueue* pq)
{
  if (pq->root == NULL) return pq;
  return pq->root;
}

// inserts x into h
struct PriorityQueue* insert_pq(int i, int j, int w, struct PriorityQueue* h)
{
  return link_pq(makePriorityQueue(i, j, w), h);
}

struct PriorityQueue* meld_pq(struct PriorityQueue* h1, struct PriorityQueue* h2)
{
  return link_pq(h1, h2);
}

struct PriorityQueue* rec_merge_pair(struct PriorityQueue* pq)
{
  if (pq->right == NULL) return pq;
  else {
    if (pq->right->right == NULL) return meld_pq(pq, pq->right);
    else return meld_pq(meld_pq(pq, pq->right), pq->right->right);
  }
}

struct PriorityQueue* delete_min_pq(struct PriorityQueue* pq)
{
  struct PriorityQueue *ch = pq->child, p = ch, q = ch;
  if (ch == NULL) {
    return NULL;
  } else {
   while (p != NULL) {
    q = p->right;
    p->root = p;
    p->left = p->right = NULL;
    p = q;
   }
   return rec_merge_pair(ch); 
  } 
}

struct Queue* make_queue()
{
  struct Queue *q = malloc(sizeof(struct Queue));
  q->head = q->end = NULL;
  q->size = 0;
  return q;
}

void free_queue(struct Queue* q)
{
  struct Node *n = q->head, *b;
  while (n != NULL) {
    b = n->next;
    free_node(n);
    n = b;
  }
  q->size =0;
  q->head = q->end = NULL;
  free(q);
}

void push_front(struct Queue* q, struct Node* n)
{
  n->next = q->head;
  q->head = n;
  if (q->end == NULL) q->end = n;
  ++q->size;
}

void push_back(struct Queue* q, struct Node* n)
{
  n->next = NULL;
  if (q->end != NULL) {
    q->end->next = n;
    q->end = n;
  } else {
    q->end = n;
    q->head = n;
  }
  ++q->size;
}

struct Node* front(struct Queue* q)
{
  return q->head;
}

struct Node* back(struct Queue* q)
{
  return q->end;
}

void pop_front(struct Queue* q)
{
  if (q->head == NULL) return;
  if (q->end == q->head) q->end = q->head->next;
  q->head = q->head->next;
  --q->size;
}

void pop_back(struct Queue* q, struct Node* n)
{
  if (q->end == NULL) return;
  if (q->end == q->head) q->head = q->end->next;
  q->end = q->end->next;
  --q->size;
}

inline static void clear_window_contents(WINDOW* w)
{
  wclear(w);
  box(w, 0, 0);
}


/*
  Recursive floodfill. 
  if not is turned on, fills everything _besides_ t with r.
  if do8way is torned on, does 8-way floodfill instead of 4-way.
*/
void floodfill(WINDOW *win, int y, int x, int t, int r, int not, int do8way)
{
  static int d[8][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {-1, 1}, {1, -1}};
  int dn = do8way ? 8 : 4, curr_txt, curr_clr;
  if (y < 0 || y >= height - 1 || x < 0 || x >= width - 1) return;
  curr_txt = mvwinch(win, y, x) & A_CHARTEXT;
  curr_clr = mvwinch(win, y, x) & A_COLOR; 
  if (curr_txt == r) return;
  if (not) {
    if (curr_txt == t) return;
    mvwprintw(win, y, x, "%c", r);
  } else {
    if (curr_txt != t) return;
    mvwprintw(win, y, x, "%c", r);
  }
  for(int i = 0; i < dn; ++i) {
    floodfill(win, y + d[i][0], x + d[i][1], t, r, not, do8way);
  }
}

int bfs(WINDOW *win, int sy, int sx, int ty, int tx, int t, int is8way)
{
   static int d[8][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {-1, 1}, {1, -1}};
   struct Queue * q = make_queue();
   int visited[getmaxy(win)][getmaxx(win)], 
       dist[getmaxy(win)][getmaxx(win)];
   int dn = is8way ? 8 : 4;
   memset(visited, 0, sizeof(visited));
   dist[sy][sx] = 0;
   push_back(q, make_node(q, sy, sx));
   while (q->size != 0) {
    struct Node * n = front(q); pop_front(q);
    if (!visited[n->i][n->j]) {
      visited[n->i][n->j] = 1;
      if (n->i == ty && n->j == tx) break;
      for(int i = 0; i < dn; ++i) {
        int i_next = n->i + d[i][0], j_next = n->j + d[i][1];
        if (!visited[i_next][j_next] && (mvwinch(win, i_next, j_next) & A_CHARTEXT) == t) {
          dist[i_next][j_next] = dist[n->i][n->j] + 1;
          struct Node * next_n = make_node(q, i_next, j_next);
          push_back(q, next_n);
        }
      }
    }
    free_node(n);
   }
   free_queue(q);
   return visited[ty][tx] ? dist[ty][tx] : -1;
}

int dijkstra(WINDOW *win, int sy, int sx, int ty, int tx, int t, int is8way, int (*get_weight)(int i, int j, int ni, int nj))
{
  static int d[8][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {-1, 1}, {1, -1}};
  int dist[getmaxy(win)][getmaxx(win)], visited[getmaxy(win)][getmaxx(win)];
  struct PriorityQueue * pq = makePriorityQueue(sy, sx, get_weight(sy, sx));
  int dn = is8way ? 8 : 4;
  memset(visited, 0, sizeof(visited));
  for(int i = 0; i < getmaxy(win); ++i) 
    for(int j = 0; j < getmaxx(win); ++j)
      dist[i][j] = 0xfffff;
  while (pq->root->size != 0) {
    PriorityQueue * n = pq->root; 
    pq = delete_min_pq(pq);
    if (!visited[n->i][n->j]) {
      visited[n->i][n->j] = 1;
      for(int i = 0; i < dn; ++i) {
        int next_i = n->i + d[i][0], next_j = n->j + d[i][1];
        if (dist[next_i][next_j] >= dist[n->i][n->j] + get_weight(n
      }
    }
    free(n);
  }
  return 0;
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
{
  play_window = newwin(height, width, 0, 0);
  cmd_window = newwin(cmd_height, cmd_width, 0, width);
  box(play_window, 0, 0);
  box(cmd_window, 0, 0);
  int cmd_y, cmd_x;
  getyx(cmd_window, cmd_y, cmd_x);
  menubars = (ITEM**) calloc(sizeof(menu)/sizeof(char*)+1, sizeof(ITEM*));
  for(int i = 0; i < sizeof(menu)/sizeof(char*); ++i) {
    menubars[i] = new_item(menu[i], menu_text[i]);
  }
  menubars[sizeof(menu)/sizeof(char*)] = NULL; // guard
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

void handle_floodfill_form()
{
  int y = 1, foc = 0;
  char ch;
  unpost_menu(cmd_menu);
  clear_window_contents(cmd_window);
  wborder(cmd_window, 1, 3, 1, getmaxx(cmd_window)/2-1, 0, 0, 0, 0);
  wborder(cmd_window, 1, 3, getmaxx(cmd_window)/2, getmaxx(cmd_window)-1, 0, 0, 0, 0);
  attron(COLOR_PAIR(1));
  mvwprintw(cmd_window, 2, 2, "Select x");
  attroff(COLOR_PAIR(1));
  mvwprintw(cmd_window, 2, getmaxx(cmd_window)/2+1, "Go back");
  while (1) {
    int ch = wgetch(cmd_window);
    if (ch == KEY_LEFT || ch == KEY_RIGHT) {
      if (!foc) {
        foc = 1 - foc;
        attroff(COLOR_PAIR(1));
        mvwprintw(cmd_window, 2, 2, "Select x");
        attron(COLOR_PAIR(1));
        mvwprintw(cmd_window, 2, getmaxx(cmd_window)/2+1, "Go back");
        attroff(COLOR_PAIR(1));
      } else {
        attron(COLOR_PAIR(1));
        mvwprintw(cmd_window, 2, 2, "Select x");
        attroff(COLOR_PAIR(1));
        mvwprintw(cmd_window, 2, getmaxx(cmd_window)/2+1, "Go back");
      }
    } else if (ch == KEY_ENTER) {
      break;
    }
  }
go_back_floodfill:
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
          case 5: handle_floodfill_form(); break;
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

