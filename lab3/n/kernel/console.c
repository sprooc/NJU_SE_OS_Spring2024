
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                              console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
        回车键: 把光标移到第一列
        换行键: 把光标前进到下一行
*/
// #include "type.h"
// #include "const.h"
// #include "protect.h"
// #include "string.h"
// #include "proc.h"
// #include "tty.h"
// #include "console.h"
// #include "global.h"
// #include "keyboard.h"
// #include "proto.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"
PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);
PRIVATE int mode = NORMAL_MODE;
PRIVATE int search_size;
/*======================================================================*
                           init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty) {
  int nr_tty = p_tty - tty_table;
  p_tty->p_console = console_table + nr_tty;

  int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

  int con_v_mem_size = v_mem_size / NR_CONSOLES;
  p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
  p_tty->p_console->v_mem_limit = con_v_mem_size;
  p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

  /* 默认光标位置在最开始处 */
  p_tty->p_console->cursor = p_tty->p_console->original_addr;

  if (nr_tty == 0) {
    /* 第一个控制台沿用原来的光标位置 */
    p_tty->p_console->cursor = disp_pos / 2;
    disp_pos = 0;
  } else {
    out_char(p_tty->p_console, nr_tty + '0');
    out_char(p_tty->p_console, '#');
  }

  set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
                           is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con) {
  return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
                           out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch) {
  if (mode == SC_MODE_WAIT && ch != ESC_CHAR) {
    return;
  }
  u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

  switch (ch) {
  case ESC_CHAR:
    if (mode == NORMAL_MODE) {
      mode = SC_MODE_INPUT;
      search_size = 0;
    } else if (mode == SC_MODE_WAIT) {
      mode = NORMAL_MODE;
      back_to_normal(p_con);
    }
    break;
  case '\t':
    if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_WIDTH) {
      for (int i = 0; i < TAB_WIDTH; i++) {
        *(p_vmem++) = ' ';
        *(p_vmem++) = BLUE;
      }
      p_con->cursor += TAB_WIDTH;
    }
    break;
  case '\n':
    if (mode == SC_MODE_INPUT) {
      mode = SC_MODE_WAIT;
      find_matched(p_con);
    } else if (p_con->cursor <
               p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
      p_con->cursor =
          p_con->original_addr +
          SCREEN_WIDTH *
              ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
    }
    break;
  case '\b':
    if (p_con->cursor > p_con->original_addr) {
      int b_size = 1;
      if (*(p_vmem - 1) == BLUE) {
        b_size = TAB_WIDTH;
      }
      for (int i = 0; i < b_size; i++) {
        p_con->cursor--;
        *(--p_vmem) = DEFAULT_CHAR_COLOR;
        *(--p_vmem) = ' ';
      }
    }
    break;
  default:
    if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
      *p_vmem++ = ch;
      if (mode == SC_MODE_INPUT) {
        search_size++;
        *p_vmem++ = RED;
      } else {
        *p_vmem++ = DEFAULT_CHAR_COLOR;
      }
      p_con->cursor++;
    }
    break;
  }

  while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
    scroll_screen(p_con, SCR_DN);
  }

  flush(p_con);
}

PUBLIC void find_matched(CONSOLE *p_con) {
  char KMP_nxt[search_size + 1];
  KMP_nxt[1] = 0;
  u8 *word_begin = (u8 *)(V_MEM_BASE + p_con->cursor * 2 - search_size * 2 - 2);
  for (int i = 2, j = 0; i <= search_size; i++) {
    while (j > 0 && *(word_begin + i * 2) != *(word_begin + (j + 1) * 2))
      j = KMP_nxt[j];
    if (*(word_begin + i * 2) == *(word_begin + (j + 1) * 2))
      j++;
    KMP_nxt[i] = j;
  }
  u8 *head_addr = (u8 *)(V_MEM_BASE + p_con->original_addr * 2 - 2);
  int size = p_con->cursor - p_con->original_addr - search_size;

  for (int i = 1, j = 0; i <= size; i++) {
    while (j > 0 &&
           (j == size || *(head_addr + i * 2) != *(word_begin + (j + 1) * 2)))
      j = KMP_nxt[j];
    if (*(head_addr + i * 2) == *(word_begin + (j + 1) * 2)) {
      j++;
    }
    if (j == search_size) {
      for (int k = 0; k < search_size; k++) {
        *(head_addr + i * 2 + 1 - k * 2) = RED;
      }
    }
  }
  flush(p_con);
}

PUBLIC void back_to_normal(CONSOLE *p_con) {
  u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
  for (int i = 0; i < search_size; i++) {
    p_con->cursor--;
    *(--p_vmem) = DEFAULT_CHAR_COLOR;
    *(--p_vmem) = ' ';
  }
  search_size = 0;
  u8 *head_addr = (u8 *)(V_MEM_BASE + p_con->original_addr * 2);
  while (head_addr < p_vmem) {
    if (*(head_addr + 1) == RED) {
      *(head_addr + 1) = DEFAULT_CHAR_COLOR;
    }
    head_addr += 2;
  }


}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con) {
  set_cursor(p_con->cursor);
  set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
                            set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
  disable_int();
  out_byte(CRTC_ADDR_REG, CURSOR_H);
  out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
  out_byte(CRTC_ADDR_REG, CURSOR_L);
  out_byte(CRTC_DATA_REG, position & 0xFF);
  enable_int();
}

/*======================================================================*
                          set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
  disable_int();
  out_byte(CRTC_ADDR_REG, START_ADDR_H);
  out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
  out_byte(CRTC_ADDR_REG, START_ADDR_L);
  out_byte(CRTC_DATA_REG, addr & 0xFF);
  enable_int();
}

/*======================================================================*
                           select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
  if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
    return;
  }

  nr_current_console = nr_console;

  set_cursor(console_table[nr_console].cursor);
  set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
                           scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
        SCR_UP	: 向上滚屏
        SCR_DN	: 向下滚屏
        其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction) {
  if (direction == SCR_UP) {
    if (p_con->current_start_addr > p_con->original_addr) {
      p_con->current_start_addr -= SCREEN_WIDTH;
    }
  } else if (direction == SCR_DN) {
    if (p_con->current_start_addr + SCREEN_SIZE <
        p_con->original_addr + p_con->v_mem_limit) {
      p_con->current_start_addr += SCREEN_WIDTH;
    }
  } else {
  }

  set_video_start_addr(p_con->current_start_addr);
  set_cursor(p_con->cursor);
}

PUBLIC void clear_screan(CONSOLE* p_con) {
  mode = NORMAL_MODE;
  search_size = 0;
  u8* curse_addr = (u8*)(V_MEM_BASE + p_con->cursor * 2);
  u8* ptr = (u8*)(V_MEM_BASE + p_con->original_addr * 2);
  while (ptr < curse_addr) {
    *(ptr++) = ' ';
    *(ptr++) = DEFAULT_CHAR_COLOR;
  }
  p_con->cursor = p_con->original_addr;
  flush(p_con);
}