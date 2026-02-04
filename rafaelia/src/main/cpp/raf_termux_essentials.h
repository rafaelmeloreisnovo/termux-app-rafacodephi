/* raf_termux_essentials.h
   Núcleo determinístico: shell + 50 ferramentas essenciais (sem libc)
*/
#ifndef RAF_TERMUX_ESSENTIALS_H
#define RAF_TERMUX_ESSENTIALS_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

#ifndef RAF_ESSENTIAL_MAX
#define RAF_ESSENTIAL_MAX 50u
#endif

#ifndef RAF_SESSION_BAR_MAX
#define RAF_SESSION_BAR_MAX 64u
#endif

typedef struct {
  u32 id;
  u16 name_len;
  u16 class_id;
  u32 cap_flags;
  u32 weight;
} raf_essential_t;

typedef struct {
  raf_essential_t slots[RAF_ESSENTIAL_MAX];
  u32 count;
  u32 seal;
} raf_essential_table_t;

typedef struct {
  u32 ids[RAF_ESSENTIAL_MAX];
  u32 weights[RAF_ESSENTIAL_MAX];
  u32 head;
  u32 count;
  u32 limit;
} raf_bitraf_store_t;

typedef struct {
  u32 weights[RAF_SESSION_BAR_MAX];
  u32 cursor;
  u32 count;
  u32 seal;
} raf_session_bar_t;

const raf_essential_table_t *RmR_essentials_table(void);
void RmR_essentials_store_init(raf_bitraf_store_t *st, u32 limit);
void RmR_essentials_store_push(raf_bitraf_store_t *st, u32 id, u32 weight);
void RmR_session_bar_init(raf_session_bar_t *bar);
void RmR_session_bar_balance(raf_session_bar_t *bar, const raf_bitraf_store_t *st);

#endif
