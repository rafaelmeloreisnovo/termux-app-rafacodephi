/* rafaelia_bitraf_core.c
   Núcleo C: BITRAF (D/I/P/R) + slot10 + base20 + dual parity + atrator 42
   - Sem libc (freestanding-friendly)
   - Append-only: não reescreve payload, só adiciona "pontos"
*/

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

#ifndef NULL
#define NULL ((void*)0)
#endif

/* ==== Arquitetura/Barramento (detecção compile-time) ==== */
#define RAF_ARCH_UNKNOWN 0u
#define RAF_ARCH_X86_64  1u
#define RAF_ARCH_X86_32  2u
#define RAF_ARCH_ARM64   3u
#define RAF_ARCH_ARM32   4u
#define RAF_ARCH_RISCV64 5u
#define RAF_ARCH_RISCV32 6u

static u32 raf_arch_id(void){
#if defined(__x86_64__) || defined(_M_X64)
  return RAF_ARCH_X86_64;
#elif defined(__i386__) || defined(_M_IX86)
  return RAF_ARCH_X86_32;
#elif defined(__aarch64__)
  return RAF_ARCH_ARM64;
#elif defined(__arm__) || defined(_M_ARM)
  return RAF_ARCH_ARM32;
#elif defined(__riscv) && (__riscv_xlen == 64)
  return RAF_ARCH_RISCV64;
#elif defined(__riscv) && (__riscv_xlen == 32)
  return RAF_ARCH_RISCV32;
#else
  return RAF_ARCH_UNKNOWN;
#endif
}

static u8 raf_bus_bits(void){
  return (u8)(sizeof(void*) * 8u);
}

/* ==== Backend mínimo (pluga em stdout/UART/MMIO) ==== */
struct RMR_API {
  u32 (*write)(void *ctx, const u8 *buf, u32 len);
  void (*panic)(void *ctx, const char *msg);
  void *ctx;
};

static struct RMR_API g_api;

static void rmr_bind_api(const struct RMR_API *api){
  if(api) g_api = *api;
}

static void rmr_panic(const char *msg){
  if(g_api.panic) g_api.panic(g_api.ctx, msg);
  for(;;) { /* loop */ }
}

static void rmr_write_bytes(const u8 *buf, u32 len){
  if(g_api.write) (void)g_api.write(g_api.ctx, buf, len);
}

/* ==== Hooks low-level (ARQU) ==== */
struct RAF_HOOKS {
  u32 (*read)(void *ctx, u8 *buf, u32 len);
  u32 (*write)(void *ctx, const u8 *buf, u32 len);
  void (*barrier)(void *ctx);
  void *ctx;
};

static struct RAF_HOOKS g_hooks;

static void raf_bind_hooks(const struct RAF_HOOKS *hooks){
  if(hooks) g_hooks = *hooks;
}

static u32 raf_hook_read(u8 *buf, u32 len){
  if(g_hooks.read) return g_hooks.read(g_hooks.ctx, buf, len);
  return 0u;
}

static u32 raf_hook_write(const u8 *buf, u32 len){
  if(g_hooks.write) return g_hooks.write(g_hooks.ctx, buf, len);
  return 0u;
}

static void raf_hook_barrier(void){
  if(g_hooks.barrier) g_hooks.barrier(g_hooks.ctx);
}

/* ==== util sem libc ==== */
static void rmr_memset(void *dst, u8 v, u32 n){
  u8 *d=(u8*)dst;
  while(n--) *d++=v;
}
static void rmr_memcpy(void *dst, const void *src, u32 n){
  u8 *d=(u8*)dst; const u8*s=(const u8*)src;
  while(n--) *d++=*s++;
}

/* ==== BITRAF: estados ==== */
typedef enum {
  RAF_D = 0, /* Data */
  RAF_I = 1, /* Informação */
  RAF_P = 2, /* Processo */
  RAF_R = 3  /* Retorno/Resultado */
} raf_state_t;

/* ==== “Pontos” (append-only) ==== */
typedef struct {
  u32 idx;        /* índice do ponto */
  u8  state;      /* RAF_D/I/P/R */
  u8  slot10;     /* 0..9 */
  u8  sym20;      /* 0..19 (base20) */
  u8  p0;         /* parity 0 */
  u8  p1;         /* parity 1 */
  u8  ecc0;       /* ECC (baixa/lo) */
  u8  ecc1;       /* ECC (alta/hi) */
  u16 noise;      /* ruído absorvido (contagem/assinatura simples) */
  u16 crc16;      /* checksum leve (opcional) */
  u32 aux;        /* campo livre (ex: endereço lógico, flags, etc.) */
} raf_point_t;

/* ==== Anel de pontos ==== */
#ifndef RAF_POINTS_MAX
#define RAF_POINTS_MAX 4096u
#endif

#define RAF_AREAS_MAX 8u

typedef struct {
  u32 id;
  u32 cycles;
  u32 p0_acc;
  u32 p1_acc;
  u32 ecc_acc;
  u32 noise_acc;
  u32 attract_acc;
  u32 last_idx;
  u32 last_aux;
  u16 last_crc16;
  u8  last_state;
  u8  last_slot10;
  u8  last_sym20;
  u8  last_p0;
  u8  last_p1;
  u8  last_ecc0;
  u8  last_ecc1;
} raf_area_t;

typedef struct {
  u32 tick;
  u32 steps;
  u32 points;
  u32 errors;
  u32 last_arch;
  u32 last_bus;
  u32 last_crc16;
  u32 last_attract;
} raf_telemetry_t;

typedef struct {
  u32 matrix[RAF_AREAS_MAX][RAF_AREAS_MAX];
  u32 checksum;
  u32 seal;
  u32 version;
} raf_manifest_t;

typedef struct {
  raf_point_t pts[RAF_POINTS_MAX];
  u32 head;     /* próximo a escrever */
  u32 count;    /* total escrito (pode saturar) */
  u32 attract;  /* contador de atrator 42 */
  u32 noise_acc;/* acumulador de ruído global */
  u32 arch_id;  /* arquitetura */
  u8  bus_bits; /* 32/64 */
  u8  last_area;
  u8  has_last_area;
  u8  _pad0;
  raf_area_t areas[RAF_AREAS_MAX];
  u32 area_matrix[RAF_AREAS_MAX][RAF_AREAS_MAX];
  raf_telemetry_t telemetry;
  raf_manifest_t manifesto;
} raf_store_t;

/* ==== CRC16 mínimo (polinômio simples) ==== */
static u16 raf_crc16(const u8 *data, u32 len){
  u16 crc = 0xA5A5u;
  for(u32 i=0;i<len;i++){
    crc ^= (u16)data[i] << 8;
    for(u8 b=0;b<8;b++){
      if(crc & 0x8000u) crc = (u16)((crc<<1) ^ 0x1021u);
      else             crc = (u16)(crc<<1);
    }
  }
  return crc;
}

/* ==== Paridade (dupla) ==== */
/* p0: XOR de bits pares; p1: XOR de bits ímpares (por posição) */
static void raf_dual_parity_u64(u64 x, u8 *p0, u8 *p1){
  u64 even = x & 0x5555555555555555ULL; /* bits 0,2,4.. */
  u64 odd  = x & 0xAAAAAAAAAAAAAAAAULL; /* bits 1,3,5.. */
  /* reduz XOR para 1 bit */
  even ^= even>>32; even ^= even>>16; even ^= even>>8; even ^= even>>4; even ^= even>>2; even ^= even>>1;
  odd  ^= odd >>32; odd  ^= odd >>16; odd  ^= odd >>8; odd  ^= odd >>4; odd  ^= odd >>2; odd  ^= odd >>1;
  *p0 = (u8)(even & 1u);
  *p1 = (u8)(odd  & 1u);
}

/* ==== “9→1” (redução digital / coerência) ==== */
static u8 raf_digital_root_9to1(u32 v){
  /* digital root (0..9), mapeia 9->1 e 0->0 */
  if(v==0) return 0;
  u32 r = v % 9u;
  return (r==0u)? 1u : (u8)r;
}

/* ==== Base20: normaliza 0..19 ==== */
static u8 raf_base20(u32 v){ return (u8)(v % 20u); }

/* ==== slot10: normaliza 0..9 ==== */
static u8 raf_slot10(u32 v){ return (u8)(v % 10u); }

/* ==== ECC simples (paridades por posição) ==== */
static u8 raf_ecc32(u32 v){
  u8 ecc = 0u;
  for(u8 bit=0u; bit<6u; bit++){
    u32 p = 0u;
    for(u8 i=0u; i<32u; i++){
      u32 pos = (u32)i + 1u;
      if(pos & (1u << bit)){
        p ^= (v >> i) & 1u;
      }
    }
    ecc |= (u8)((p & 1u) << bit);
  }
  return ecc;
}

static void raf_ecc64(u64 v, u8 *ecc0, u8 *ecc1){
  u32 lo = (u32)(v & 0xFFFFFFFFu);
  u32 hi = (u32)(v >> 32);
  if(ecc0) *ecc0 = raf_ecc32(lo);
  if(ecc1) *ecc1 = raf_ecc32(hi);
}

/* ==== Manifesto determinístico (matriz estática) ==== */
static void raf_manifest_init(raf_manifest_t *m){
  if(!m) rmr_panic("raf_manifest NULL");
  rmr_memset(m, 0, sizeof(*m));
  m->version = 1u;
  m->seal = 0x5241464Du; /* "RAFM" */
  for(u32 i=0u;i<RAF_AREAS_MAX;i++){
    for(u32 j=0u;j<RAF_AREAS_MAX;j++){
      u32 v = (u32)(i + 1u) * 10u;
      v += (u32)(j + 1u) * 20u;
      v ^= (u32)((i ^ j) * 42u);
      m->matrix[i][j] = v;
    }
  }
  {
    u8 buf[RAF_AREAS_MAX * RAF_AREAS_MAX * 4u];
    u32 k = 0u;
    for(u32 i=0u;i<RAF_AREAS_MAX;i++){
      for(u32 j=0u;j<RAF_AREAS_MAX;j++){
        u32 v = m->matrix[i][j];
        buf[k++] = (u8)v;
        buf[k++] = (u8)(v >> 8);
        buf[k++] = (u8)(v >> 16);
        buf[k++] = (u8)(v >> 24);
      }
    }
    m->checksum = (u32)raf_crc16(buf, k);
  }
}

/* ==== Atrator 42 (estado âncora) ==== */
static u8 raf_is_attractor42(const raf_point_t *p){
  /* “42” como fechamento: combinações coerentes e repetíveis.
     Aqui: digital-root(state+slot10+sym20+paridades+noise) == 6 e (slot10+sym20)%? = 2
     Você pode trocar a regra sem quebrar o store (é só função). */
  u32 mix = (u32)p->state + (u32)p->slot10 + (u32)p->sym20 + (u32)p->p0 + (u32)p->p1 + (u32)p->noise;
  u8 dr = raf_digital_root_9to1(mix);
  u8 gate = (u8)((p->slot10 + p->sym20) % 10u);
  return (dr == 6u && gate == 2u) ? 1u : 0u;
}

/* ==== Área (8 domínios) ==== */
static u8 raf_area_index(const raf_point_t *p){
  u32 mix = (u32)p->state + (u32)p->slot10 + (u32)p->sym20 +
            (u32)p->p0 + (u32)p->p1 + (u32)p->ecc0 + (u32)p->ecc1 +
            (u32)p->noise + (u32)(p->aux & 0xFFu);
  return (u8)(mix & 7u);
}

static void raf_area_update(raf_store_t *st, const raf_point_t *p, u8 area){
  raf_area_t *a = &st->areas[area];
  a->id = area;
  a->cycles++;
  a->p0_acc += (u32)p->p0;
  a->p1_acc += (u32)p->p1;
  a->ecc_acc += (u32)p->ecc0 + (u32)p->ecc1;
  a->noise_acc += (u32)p->noise;
  if(raf_is_attractor42(p)) a->attract_acc++;
  a->last_idx = p->idx;
  a->last_aux = p->aux;
  a->last_crc16 = p->crc16;
  a->last_state = p->state;
  a->last_slot10 = p->slot10;
  a->last_sym20 = p->sym20;
  a->last_p0 = p->p0;
  a->last_p1 = p->p1;
  a->last_ecc0 = p->ecc0;
  a->last_ecc1 = p->ecc1;

  if(st->has_last_area){
    st->area_matrix[st->last_area][area] += 1u;
  } else {
    st->has_last_area = 1u;
  }
  st->last_area = area;
}

static void raf_store_init(raf_store_t *st){
  if(!st) rmr_panic("raf_store NULL");
  rmr_memset(st, 0, sizeof(*st));
  st->arch_id = raf_arch_id();
  st->bus_bits = raf_bus_bits();
  st->telemetry.last_arch = st->arch_id;
  st->telemetry.last_bus = st->bus_bits;
  raf_manifest_init(&st->manifesto);
}

/* ==== Cria “ponto” (append-only) ==== */
static void raf_append_point(
  raf_store_t *st,
  raf_state_t state,
  u32 slot_seed,
  u32 sym_seed,
  u64 payload64,
  u16 noise_hint,
  u32 aux
){
  if(!st) rmr_panic("raf_store NULL");
  raf_point_t *p = &st->pts[st->head];

  p->idx   = st->count;
  p->state = (u8)state;
  p->slot10= raf_slot10(slot_seed);
  p->sym20 = raf_base20(sym_seed);

  raf_dual_parity_u64(payload64, &p->p0, &p->p1);
  raf_ecc64(payload64, &p->ecc0, &p->ecc1);

  /* ruído absorvido: acumula + mistura leve (não é crypto; é telemetria determinística) */
  st->noise_acc += (u32)noise_hint + (u32)(payload64 ^ (payload64>>32));
  p->noise = (u16)(st->noise_acc & 0xFFFFu);

  p->aux = aux;

  /* checksum do ponto (sem depender de struct packing externo) */
  {
    u8 buf[26];
    /* serialização mínima fixa */
    buf[0]=(u8)(p->idx); buf[1]=(u8)(p->idx>>8); buf[2]=(u8)(p->idx>>16); buf[3]=(u8)(p->idx>>24);
    buf[4]=p->state; buf[5]=p->slot10; buf[6]=p->sym20; buf[7]=p->p0;
    buf[8]=p->p1; buf[9]=p->ecc0; buf[10]=p->ecc1;
    buf[11]=(u8)p->noise; buf[12]=(u8)(p->noise>>8);
    buf[13]=(u8)(p->aux); buf[14]=(u8)(p->aux>>8); buf[15]=(u8)(p->aux>>16); buf[16]=(u8)(p->aux>>24);
    /* payload64 entra no CRC só como “sombra” */
    buf[17]=(u8)(payload64); buf[18]=(u8)(payload64>>8); buf[19]=(u8)(payload64>>16); buf[20]=(u8)(payload64>>24);
    buf[21]=(u8)(payload64>>32); buf[22]=(u8)(payload64>>40); buf[23]=(u8)(payload64>>48); buf[24]=(u8)(payload64>>56);
    buf[25]=(u8)(noise_hint);

    p->crc16 = raf_crc16(buf, 26);
  }

  if(raf_is_attractor42(p)) st->attract++;
  raf_area_update(st, p, raf_area_index(p));
  st->telemetry.points = st->count + 1u;
  st->telemetry.last_crc16 = p->crc16;
  st->telemetry.last_attract = st->attract;

  /* avança ring */
  st->head = (st->head + 1u) % RAF_POINTS_MAX;
  st->count++;
}

/* ==== Motor BITRAF: 4 ciclos (input/process/output/semântica) ==== */
typedef struct {
  raf_state_t s;
  u32 t;      /* “tempo”/tick lógico (não clock físico) */
  u32 phase;  /* 0..3 */
  u32 slot_seed;
  u32 sym_seed;
  u32 aux;
} raf_machine_t;

static void raf_machine_init(raf_machine_t *m){
  if(!m) rmr_panic("raf_machine NULL");
  m->s = RAF_D;
  m->t = 0;
  m->phase = 0;
  m->slot_seed = 0;
  m->sym_seed  = 0;
  m->aux = 0;
}

/* transição determinística (simetria assimétrica): alterna sentido por paridade */
static raf_state_t raf_next_state(raf_state_t cur, u8 p0, u8 p1){
  /* se p0^p1=0: avança; senão: retorna (reverso) */
  u8 dir = (u8)(p0 ^ p1);
  if(dir==0){
    /* D->I->P->R->D */
    return (raf_state_t)((cur + 1) & 3);
  } else {
    /* D<-I<-P<-R<-D */
    return (raf_state_t)((cur + 3) & 3);
  }
}

static void raf_step(raf_store_t *st, raf_machine_t *m, u64 payload64, u16 noise_hint){
  if(!st || !m) rmr_panic("raf_step NULL");

  /* fase = 4 ciclos absorvente (input/process/output/semântica) */
  switch(m->phase & 3u){
    case 0: /* INPUT: coleta/endereça */
      m->slot_seed = m->t;
      m->sym_seed  = (m->t * 7u) + 14u; /* 7+14 como “painel reverso” */
      m->aux = (m->t << 16) ^ (u32)payload64;
      break;

    case 1: /* PROCESSO: mistura determinística */
      m->slot_seed ^= (u32)(payload64) ^ (u32)noise_hint;
      m->sym_seed  ^= (u32)(payload64>>32) + (u32)(m->s * 42u);
      break;

    case 2: /* SAÍDA: grava ponto (append-only) */
      raf_append_point(st, m->s, m->slot_seed, m->sym_seed, payload64, noise_hint, m->aux);
      break;

    case 3: /* SEMÂNTICA: muda estado pelo “barramento” (paridades) */
    {
      u8 p0,p1;
      raf_dual_parity_u64(payload64, &p0, &p1);
      m->s = raf_next_state(m->s, p0, p1);
      break;
    }
  }

  m->phase++;
  if((m->phase & 3u)==0u) m->t++; /* só avança “tempo” ao fechar 4 ciclos */
  st->telemetry.steps++;
  st->telemetry.tick = m->t;
}

/* ==== Exemplo hosted (opcional) ==== */
/* Se quiser testar em ambiente com libc, plugue write/panic e rode. */
#ifdef RAF_HOSTED_TEST
#include <stdio.h>
static u32 host_write(void*ctx, const u8*buf, u32 len){ (void)ctx; return (u32)fwrite(buf,1,len,stdout); }
static void host_panic(void*ctx, const char*msg){ (void)ctx; fputs(msg, stderr); fputc('\n', stderr); }
int main(void){
  struct RMR_API api = { host_write, host_panic, NULL };
  rmr_bind_api(&api);

  raf_store_t st; raf_store_init(&st);
  raf_machine_t m; raf_machine_init(&m);

  for(u32 i=0;i<1000;i++){
    u64 payload = (0x9E3779B97F4A7C15ULL * (u64)(i+1)) ^ ((u64)i<<33);
    raf_step(&st,&m,payload,(u16)(i*3u));
  }

  char out[128];
  int n = snprintf(out,sizeof(out),"pts=%u attract42=%u noise=%u head=%u\n", st.count, st.attract, st.noise_acc, st.head);
  rmr_write_bytes((const u8*)out,(u32)n);
  return 0;
}
#endif
