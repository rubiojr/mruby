/*
** load.c - mruby binary loader
**
** See Copyright Notice in mruby.h
*/

#ifndef SIZE_MAX
 /* Some versions of VC++
  * has SIZE_MAX in stdint.h
  */
# include <limits.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "mruby/dump.h"
#include "mruby/irep.h"
#include "mruby/proc.h"
#include "mruby/string.h"

<<<<<<< HEAD
typedef struct _RiteFILE
{
  FILE* fp;
  unsigned char buf[256];
  int cnt;
  int readlen;
} RiteFILE;

const char hex2bin[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //00-0f
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //10-1f
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //20-2f
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,  //30-3f
  0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //40-4f
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //50-5f
  0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0   //60-6f
  //70-ff
};

static uint16_t hex_to_bin8(unsigned char*,unsigned char*);
static uint16_t hex_to_bin16(unsigned char*,unsigned char*);
static uint16_t hex_to_bin32(unsigned char*,unsigned char*);
static uint8_t hex_to_uint8(unsigned char*);
static uint16_t hex_to_uint16(unsigned char*);
static uint32_t hex_to_uint32(unsigned char*);
static char* hex_to_str(char*,char*,uint16_t*);
uint16_t calc_crc_16_ccitt(unsigned char*,int);
static unsigned char rite_fgetcSub(RiteFILE*);
static unsigned char rite_fgetc(RiteFILE*,int);
static unsigned char* rite_fgets(RiteFILE*,unsigned char*,int,int);
static int load_rite_header(FILE*,rite_binary_header*,unsigned char*);
static int load_rite_irep_record(mrb_state*, RiteFILE*,unsigned char*,uint32_t*);
static int read_rite_header(mrb_state*,unsigned char*,rite_binary_header*);
static int read_rite_irep_record(mrb_state*,unsigned char*,uint32_t*);


static unsigned char
rite_fgetcSub(RiteFILE* rfp)
{
  //only first call
  if (rfp->buf[0] == '\0') {
    rfp->readlen = fread(rfp->buf, 1, sizeof(rfp->buf), rfp->fp);
    rfp->cnt = 0;
  }

  if (rfp->readlen == rfp->cnt) {
    rfp->readlen = fread(rfp->buf, 1, sizeof(rfp->buf), rfp->fp);
    rfp->cnt = 0;
    if (rfp->readlen == 0) {
      return '\0';
    }
  }
  return rfp->buf[(rfp->cnt)++];
}

static unsigned char
rite_fgetc(RiteFILE* rfp, int ignorecomment)
{
  unsigned char  tmp;

  for (;;) {
    tmp = rite_fgetcSub(rfp);
    if (tmp == '\n' || tmp == '\r') {
      continue;
    }
    else if (ignorecomment && tmp == '#') {
      while (tmp != '\n' && tmp != '\r' && tmp != '\0')
        tmp = rite_fgetcSub(rfp);
      if (tmp == '\0')
        return '\0';
    }
    else {
      return tmp;
    }
  }
}

static unsigned char*
rite_fgets(RiteFILE* rfp, unsigned char* dst, int len, int ignorecomment)
{
  int i;

  for (i=0; i<len; i++) {
    if ('\0' == (dst[i] = rite_fgetc(rfp, ignorecomment))) {
      return NULL;
    }
  }
  return dst;
}

static int
load_rite_header(FILE* fp, rite_binary_header* bin_header, unsigned char* hcrc)
{
  rite_file_header    file_header;

  if (fread(&file_header, 1, sizeof(file_header), fp) < sizeof(file_header)) {
    return MRB_DUMP_READ_FAULT;
  }
  memcpy(bin_header->rbfi, file_header.rbfi, sizeof(file_header.rbfi));
  if (memcmp(bin_header->rbfi, RITE_FILE_IDENFIFIER, sizeof(bin_header->rbfi)) != 0) {
    return MRB_DUMP_INVALID_FILE_HEADER;    //File identifier error
  }
  memcpy(bin_header->rbfv, file_header.rbfv, sizeof(file_header.rbfv));
  if (memcmp(bin_header->rbfv, RITE_FILE_FORMAT_VER, sizeof(bin_header->rbfv)) != 0) {
    return MRB_DUMP_INVALID_FILE_HEADER;    //File format version error
  }
  memcpy(bin_header->risv, file_header.risv, sizeof(file_header.risv));
  memcpy(bin_header->rct, file_header.rct, sizeof(file_header.rct));
  memcpy(bin_header->rcv, file_header.rcv, sizeof(file_header.rcv));
  hex_to_bin32(bin_header->rbds, file_header.rbds);
  hex_to_bin16(bin_header->nirep, file_header.nirep);
  hex_to_bin16(bin_header->sirep, file_header.sirep);
  memcpy(bin_header->rsv, file_header.rsv, sizeof(file_header.rsv));
  memcpy(hcrc, file_header.hcrc, sizeof(file_header.hcrc));

  return MRB_DUMP_OK;
}

static int
load_rite_irep_record(mrb_state *mrb, RiteFILE* rfp, unsigned char* dst, uint32_t* len)
{
  int i;
  uint32_t blocklen;
  uint16_t offset, pdl, snl, clen;
  unsigned char hex2[2] = {0}, hex4[4] = {0}, hex8[8] = {0}, hcrc[4] = {0};
  unsigned char *pStart;
  char *char_buf;
  uint16_t buf_size =0;
  int result;

  buf_size = MRB_DUMP_DEFAULT_STR_LEN;
  char_buf = (char *)mrb_malloc(mrb, buf_size);
  if (char_buf == NULL) {
    result = MRB_DUMP_GENERAL_FAILURE;
    goto error_exit;
  }

  pStart = dst;

  //IREP HEADER BLOCK
  *dst = rite_fgetc(rfp, TRUE);                         //record identifier
  if (*dst != RITE_IREP_IDENFIFIER)
    return MRB_DUMP_INVALID_IREP;
  dst += sizeof(unsigned char);
  *dst = rite_fgetc(rfp, TRUE);                         //class or module
  dst += sizeof(unsigned char);
  rite_fgets(rfp, hex4, sizeof(hex4), TRUE);            //number of local variable
  dst += hex_to_bin16(dst, hex4);
  rite_fgets(rfp, hex4, sizeof(hex4), TRUE);            //number of register variable
  dst += hex_to_bin16(dst, hex4);
  rite_fgets(rfp, hex4, sizeof(hex4), TRUE);            //offset of isec block
  offset = hex_to_uint16(hex4);
  rite_fgets(rfp, hcrc, sizeof(hcrc), TRUE);            //header CRC
  memset( char_buf, '\0', buf_size);
  rite_fgets(rfp, (unsigned char*)char_buf, (offset - (MRB_DUMP_SIZE_OF_SHORT * RITE_FILE_HEX_SIZE)), TRUE); //class or module name
  hex_to_str(char_buf, (char*)(dst + MRB_DUMP_SIZE_OF_SHORT + MRB_DUMP_SIZE_OF_SHORT), &clen); //class or module name
  dst += uint16_to_bin((MRB_DUMP_SIZE_OF_SHORT/*crc*/ + clen), (char*)dst); //offset of isec block
  dst += hex_to_bin16(dst, hcrc);                 //header CRC
  dst += clen;

  //ISEQ BLOCK
  rite_fgets(rfp, hex8, sizeof(hex8), TRUE);            //iseq length
  dst += hex_to_bin32(dst, hex8);
  blocklen = hex_to_uint32(hex8);
  for (i=0; i<blocklen; i++) {
    rite_fgets(rfp, hex8, sizeof(hex8), TRUE);          //iseq
    dst += hex_to_bin32(dst, hex8);
  }
  rite_fgets(rfp, hcrc, sizeof(hcrc), TRUE);            //iseq CRC
  dst += hex_to_bin16(dst, hcrc);

  //POOL BLOCK
  rite_fgets(rfp, hex8, sizeof(hex8), TRUE);            //pool length
  dst += hex_to_bin32(dst, hex8);
  blocklen = hex_to_uint32(hex8);
  for (i=0; i<blocklen; i++) {
    rite_fgets(rfp, hex2, sizeof(hex2), TRUE);          //TT
    dst += hex_to_bin8(dst, hex2);
    rite_fgets(rfp, hex4, sizeof(hex4), TRUE);          //pool data length
    pdl = hex_to_uint16(hex4);

    if ( pdl > buf_size - 1) {
      buf_size = pdl + 1;
      char_buf = (char *)mrb_realloc(mrb, char_buf, buf_size);
      if (char_buf == NULL) {
        result = MRB_DUMP_GENERAL_FAILURE;
        goto error_exit;
      }
    }
    memset(char_buf, '\0', buf_size);
    rite_fgets(rfp, (unsigned char*)char_buf, pdl, FALSE); //pool
    hex_to_str(char_buf, (char*)(dst + MRB_DUMP_SIZE_OF_SHORT), &clen);
    dst += uint16_to_bin(clen, (char*)dst);
    dst += clen;
  }
  rite_fgets(rfp, hcrc, sizeof(hcrc), TRUE);            //pool CRC
  dst += hex_to_bin16(dst, hcrc);

  //SYMS BLOCK
  rite_fgets(rfp, hex8, sizeof(hex8), TRUE);            //syms length
  dst += hex_to_bin32(dst, hex8);
  blocklen = hex_to_uint32(hex8);
  for (i=0; i<blocklen; i++) {
    rite_fgets(rfp, hex4, sizeof(hex4), TRUE);          //symbol name length
    snl = hex_to_uint16(hex4);

    if (snl == MRB_DUMP_NULL_SYM_LEN) {
      dst += uint16_to_bin(snl, (char*)dst);
      continue;
    }

    if ( snl > buf_size - 1) {
      buf_size = snl + 1;
      char_buf = (char *)mrb_realloc(mrb, char_buf, buf_size);
      if (char_buf == NULL) {
        result = MRB_DUMP_GENERAL_FAILURE;
        goto error_exit;
      }
    }
    memset(char_buf, '\0', buf_size);
    rite_fgets(rfp, (unsigned char*)char_buf, snl, FALSE); //symbol name
    hex_to_str(char_buf, (char*)(dst + MRB_DUMP_SIZE_OF_SHORT), &clen);
    dst += uint16_to_bin(clen, (char*)dst);
    dst += clen;
  }
  rite_fgets(rfp, hcrc, sizeof(hcrc), TRUE);            //syms CRC
  dst += hex_to_bin16(dst, hcrc);

  *len = dst - pStart;

  result = MRB_DUMP_OK;
error_exit:
  mrb_free(mrb, char_buf);

  return result;
}

static int
load_debug_info_irep(mrb_state *mrb, mrb_irep *irep)
{
  mrb_value *pool;
  char *c;
  int i, len, plen, n = -1, base = -1, ret = 0;
  const char *mrb_debug_dump_mark = "**MRB_DEBUG_DUMP**";
  len = (int) sizeof(mrb_debug_dump_mark);

  if (irep->plen < 3)
    return ret;

  for (i=0; i<irep->plen; i++) {
    if (!mrb_string_p(irep->pool[i]))
        continue;

    c = mrb_string_value_ptr(mrb, irep->pool[i]);
    if (sizeof(c) == len && strncmp(c, mrb_debug_dump_mark, len) == 0) {
      plen = i;
      i++;
      /* fetch filename */
      if (mrb_nil_p(irep->pool[i])) {
        irep->filename = NULL;
      } else {
        irep->filename = mrb_string_value_ptr(mrb, irep->pool[i]);
      }
      i++;
      /* fetch lines size */
      n = mrb_fixnum(irep->pool[i]);
      i++;
      /* set lines start pos */
      base = i;
      break;
    }
  }

  if (base > 0 && n > 0 && irep->plen >= base+n) {
    irep->lines = mrb_malloc(mrb, sizeof(mrb_value) * n);
    if (irep->lines == NULL) {
      return MRB_DUMP_INVALID_IREP;
    }

    /* copy irep -> lines */
    for (i=0; i<n; i++) {
      irep->lines[i] = mrb_fixnum(irep->pool[base+i]);
    }

    /* remove debug info from irep->pool */
    pool = mrb_malloc(mrb, sizeof(mrb_value) * plen);
    if (pool == NULL) {
      return MRB_DUMP_INVALID_IREP;
    }
    for (i=0; i<plen; i++) {
      pool[i] = irep->pool[i];
    }
    mrb_free(mrb, irep->pool);
    irep->pool = pool;
    irep->plen = plen;
  }

  return ret;
}

static int
load_debug_info(mrb_state *mrb)
{
  int irep_no, ret = 0;
  for (irep_no = 0; irep_no < mrb->irep_len; irep_no++) {
    mrb_irep *irep = mrb->irep[irep_no];
    if (irep != NULL && irep->plen > 0) {
      ret = load_debug_info_irep(mrb, irep);
      if (ret != 0)
        return ret;
    }
  }

  return ret;
}


int
mrb_read_irep_file(mrb_state *mrb, FILE* fp)
{
  int ret, i;
  uint32_t  len, rlen = 0;
  unsigned char hex8[8], hcrc[4];
  unsigned char *dst, *rite_dst = NULL;
  rite_binary_header  bin_header;
  RiteFILE ritefp = { 0 };
  RiteFILE *rfp;

  if ((mrb == NULL) || (fp == NULL)) {
    return MRB_DUMP_INVALID_ARGUMENT;
  }
  ritefp.fp = fp;
  rfp = &ritefp;

  //Read File Header Section
  ret = load_rite_header(fp, &bin_header, hcrc);
  if (ret != MRB_DUMP_OK)
    return ret;

  len = sizeof(rite_binary_header) + bin_to_uint32(bin_header.rbds);
  rite_dst = (unsigned char *)mrb_malloc(mrb, len);
  if (rite_dst == NULL)
    return MRB_DUMP_GENERAL_FAILURE;

  dst = rite_dst;
  memset(dst, 0x00, len);
  *(rite_binary_header *)dst = bin_header;
  dst += sizeof(rite_binary_header);
  dst += hex_to_bin16(dst, hcrc);

  //Read Binary Data Section
  len = bin_to_uint16(bin_header.nirep);
  for (i=0; i<len; i++) {
    rite_fgets(rfp, hex8, sizeof(hex8), TRUE);                      //record len
    dst += hex_to_bin32(dst, hex8);
    ret = load_rite_irep_record(mrb, rfp, dst, &rlen);
    if (ret != MRB_DUMP_OK) //irep info
      goto error_exit;
    dst += rlen;
  }
  rite_fgets(rfp, hex8, sizeof(hex8), TRUE);                        //dummy record len
  hex_to_bin32(dst, hex8);  /* dst += hex_to_bin32(dst, hex8); */
  if (0 != hex_to_uint32(hex8)) {
    ret = MRB_DUMP_INVALID_IREP;
    goto error_exit;
  }

  if (ret == MRB_DUMP_OK)
    ret = mrb_read_irep(mrb, (char*)rite_dst);

  load_debug_info(mrb);

error_exit:
  mrb_free(mrb, rite_dst);
=======
#ifndef _WIN32
# if SIZE_MAX < UINT32_MAX
#  error "It can't be run this code on this environment (SIZE_MAX < UINT32_MAX)"
# endif
#endif
>>>>>>> master

#if CHAR_BIT != 8
# error This code assumes CHAR_BIT == 8
#endif

static size_t
offset_crc_body()
{
  struct rite_binary_header header;
  return ((uint8_t *)header.binary_crc - (uint8_t *)&header) + sizeof(header.binary_crc);
}

static int
read_rite_irep_record(mrb_state *mrb, const uint8_t *bin, uint32_t *len)
{
  int ret;
  size_t i;
  const uint8_t *src = bin;
  uint16_t tt, pool_data_len, snl;
  size_t plen;
  int ai = mrb_gc_arena_save(mrb);
  mrb_irep *irep = mrb_add_irep(mrb);
#ifdef ENABLE_REGEXP
  mrb_value str;
#endif

  // skip record size
  src += sizeof(uint32_t);

  // number of local variable
  irep->nlocals = bin_to_uint16(src);
  src += sizeof(uint16_t);

  // number of register variable
  irep->nregs = bin_to_uint16(src);         
  src += sizeof(uint16_t);

  // Binary Data Section
  // ISEQ BLOCK
  irep->ilen = bin_to_uint32(src);
  src += sizeof(uint32_t);
  if (irep->ilen > 0) {
    irep->iseq = (mrb_code *)mrb_malloc(mrb, sizeof(mrb_code) * irep->ilen);
    if (irep->iseq == NULL) {
      ret = MRB_DUMP_GENERAL_FAILURE;
      goto error_exit;
    }
    for (i = 0; i < irep->ilen; i++) {
      irep->iseq[i] = bin_to_uint32(src);     //iseq
      src += sizeof(uint32_t);
    }
  }

  //POOL BLOCK
  plen = bin_to_uint32(src); /* number of pool */
  src += sizeof(uint32_t);
  if (plen > 0) {
    irep->pool = (mrb_value *)mrb_malloc(mrb, sizeof(mrb_value) * plen);
    if (irep->pool == NULL) {
      ret = MRB_DUMP_GENERAL_FAILURE;
      goto error_exit;
    }

    for (i = 0; i < plen; i++) {
      mrb_value s;
      tt = *src++; //pool TT
      pool_data_len = bin_to_uint16(src); //pool data length
      src += sizeof(uint16_t);
      s = mrb_str_new(mrb, (char *)src, pool_data_len);
      src += pool_data_len;
      switch (tt) { //pool data
      case MRB_TT_FIXNUM:
        irep->pool[i] = mrb_str_to_inum(mrb, s, 10, FALSE);
        break;

      case MRB_TT_FLOAT:
        irep->pool[i] = mrb_float_value(mrb_str_to_dbl(mrb, s, FALSE));
        break;

      case MRB_TT_STRING:
<<<<<<< HEAD
        irep->pool[i] = mrb_str_new(mrb, buf, pdl);
        break;

#ifdef ENABLE_REGEXP
      case MRB_TT_REGEX:
        str = mrb_str_new(mrb, buf, pdl);
        irep->pool[i] = mrb_reg_str_to_reg(mrb, str);
=======
        irep->pool[i] = s;
>>>>>>> master
        break;

      default:
        irep->pool[i] = mrb_nil_value();
        break;
      }
      irep->plen++;
      mrb_gc_arena_restore(mrb, ai);
    }
  }

  //SYMS BLOCK
  irep->slen = bin_to_uint32(src);  //syms length
  src += sizeof(uint32_t);
  if (irep->slen > 0) {
    irep->syms = (mrb_sym *)mrb_malloc(mrb, sizeof(mrb_sym) * irep->slen);
    if (irep->syms == NULL) {
      ret = MRB_DUMP_GENERAL_FAILURE;
      goto error_exit;
    }

    for (i = 0; i < irep->slen; i++) {
      snl = bin_to_uint16(src);               //symbol name length
      src += sizeof(uint16_t);

      if (snl == MRB_DUMP_NULL_SYM_LEN) {
        irep->syms[i] = 0;
        continue;
      }

      irep->syms[i] = mrb_intern2(mrb, (char *)src, snl);
      src += snl + 1;

      mrb_gc_arena_restore(mrb, ai);
    }
  }
  *len = src - bin;

  ret = MRB_DUMP_OK;
error_exit:
  return ret;
}

static int
read_rite_section_irep(mrb_state *mrb, const uint8_t *bin)
{
  int result;
  size_t sirep;
  size_t i;
  uint32_t len;
  uint16_t nirep;
  uint16_t n;
  const struct rite_section_irep_header *header;

  header = (const struct rite_section_irep_header*)bin;
  bin += sizeof(struct rite_section_irep_header);

  sirep = mrb->irep_len;
  nirep = bin_to_uint16(header->nirep);

  //Read Binary Data Section
  for (n = 0, i = sirep; n < nirep; n++, i++) {
    result = read_rite_irep_record(mrb, bin, &len);
    if (result != MRB_DUMP_OK)
      goto error_exit;
    bin += len;
  }

  result = MRB_DUMP_OK;
error_exit:
  if (result != MRB_DUMP_OK) {
    for (i = sirep; i < mrb->irep_len; i++) {
      if (mrb->irep[i]) {
        if (mrb->irep[i]->iseq)
          mrb_free(mrb, mrb->irep[i]->iseq);

        if (mrb->irep[i]->pool)
          mrb_free(mrb, mrb->irep[i]->pool);

        if (mrb->irep[i]->syms)
          mrb_free(mrb, mrb->irep[i]->syms);

        mrb_free(mrb, mrb->irep[i]);
      }
    }
    return result;
  }
  return sirep + bin_to_uint16(header->sirep);
}

static int
read_rite_lineno_record(mrb_state *mrb, const uint8_t *bin, size_t irepno, uint32_t *len)
{
  int ret;
  size_t i, fname_len, niseq;
  char *fname;
  uint16_t *lines;

  ret = MRB_DUMP_OK;
  *len = 0;
  bin += sizeof(uint32_t); // record size
  *len += sizeof(uint32_t);
  fname_len = bin_to_uint16(bin);
  bin += sizeof(uint16_t);
  *len += sizeof(uint16_t);
  fname = (char *)mrb_malloc(mrb, fname_len + 1);
  if (fname == NULL) {
    ret = MRB_DUMP_GENERAL_FAILURE;
    goto error_exit;
  }
  memcpy(fname, bin, fname_len);
  fname[fname_len] = '\0';
  bin += fname_len;
  *len += fname_len;

  niseq = bin_to_uint32(bin);
  bin += sizeof(uint32_t); // niseq
  *len += sizeof(uint32_t);

  lines = (uint16_t *)mrb_malloc(mrb, niseq * sizeof(uint16_t));
  for (i = 0; i < niseq; i++) {
    lines[i] = bin_to_uint16(bin);
    bin += sizeof(uint16_t); // niseq
    *len += sizeof(uint16_t);
 }

  mrb->irep[irepno]->filename = fname;
  mrb->irep[irepno]->lines = lines;

error_exit:
  return ret;
}

static int
read_rite_section_lineno(mrb_state *mrb, const uint8_t *bin, size_t sirep)
{
  int result;
  size_t i;
  uint32_t len;
  uint16_t nirep;
  uint16_t n;
  const struct rite_section_lineno_header *header;

  len = 0;
  header = (const struct rite_section_lineno_header*)bin;
  bin += sizeof(struct rite_section_lineno_header);

  nirep = bin_to_uint16(header->nirep);

  //Read Binary Data Section
  for (n = 0, i = sirep; n < nirep; n++, i++) {
    result = read_rite_lineno_record(mrb, bin, i, &len);
    if (result != MRB_DUMP_OK)
      goto error_exit;
    bin += len;
  }

  result = MRB_DUMP_OK;
error_exit:
  if (result != MRB_DUMP_OK) {
    return result;
  }
  return sirep + bin_to_uint16(header->sirep);
}


static int
read_rite_binary_header(const uint8_t *bin, size_t *bin_size, uint16_t *crc)
{
  const struct rite_binary_header *header = (const struct rite_binary_header *)bin;

  if(memcmp(header->binary_identify, RITE_BINARY_IDENFIFIER, sizeof(header->binary_identify)) != 0) {
    return MRB_DUMP_INVALID_FILE_HEADER;
  }
  
  if(memcmp(header->binary_version, RITE_BINARY_FORMAT_VER, sizeof(header->binary_version)) != 0) {
    return MRB_DUMP_INVALID_FILE_HEADER;
  }

  *crc = bin_to_uint16(header->binary_crc);
  if (bin_size) {
    *bin_size = bin_to_uint32(header->binary_size);
  }

  return MRB_DUMP_OK;
}

int32_t
mrb_read_irep(mrb_state *mrb, const uint8_t *bin)
{
<<<<<<< HEAD
  char *src, *dst, buf[4];
  int escape = 0, base = 0;
  char *err_ptr;

  *str_len = 0;
  for (src = hex, dst = str; *src != '\0'; src++) {
    if (escape) {
      switch(*src) {
      case 'a':  *dst++ = '\a'/* BEL */; break;
      case 'b':  *dst++ = '\b'/* BS  */; break;
      case 't':  *dst++ = '\t'/* HT  */; break;
      case 'n':  *dst++ = '\n'/* LF  */; break;
      case 'v':  *dst++ = '\v'/* VT  */; break;
      case 'f':  *dst++ = '\f'/* FF  */; break;
      case 'r':  *dst++ = '\r'/* CR  */; break;
      case '\"': /* fall through */
      case '\'': /* fall through */
      case '\?': /* fall through */
      case '\\': *dst++ = *src; break;
      default:
        memset(buf, 0x00, sizeof(buf));

        if (*src >= '0' && *src <= '7') {
          base = 8;
          strncpy(buf, src, 3);
        } else if (*src == 'x' || *src == 'X') {
          base = 16;
          src++;
          strncpy(buf, src, 2);
        }

        *dst++ = (unsigned char) strtol(buf, &err_ptr, base) & 0xff;
        src += (err_ptr - buf - 1);
        break;
      }
      escape = 0;
    } else {
      if (*src == '\\') {
        escape = 1;
      } else {
        escape = 0;
        *dst++ = *src;
=======
  int result;
  int32_t total_nirep = 0;
  const struct rite_section_header *section_header;
  uint16_t crc;
  size_t bin_size = 0;
  size_t n;
  size_t sirep;

  if ((mrb == NULL) || (bin == NULL)) {
    return MRB_DUMP_INVALID_ARGUMENT;
  }

  result = read_rite_binary_header(bin, &bin_size, &crc);
  if(result != MRB_DUMP_OK) {
    return result;
  }

  n = offset_crc_body();
  if(crc != calc_crc_16_ccitt(bin + n, bin_size - n, 0)) {
    return MRB_DUMP_INVALID_FILE_HEADER;
  }

  bin += sizeof(struct rite_binary_header);
  sirep = mrb->irep_len;

  do {
    section_header = (const struct rite_section_header *)bin;
    if(memcmp(section_header->section_identify, RITE_SECTION_IREP_IDENTIFIER, sizeof(section_header->section_identify)) == 0) {
      result = read_rite_section_irep(mrb, bin);
      if(result < MRB_DUMP_OK) {
        return result;
>>>>>>> master
      }
      total_nirep += result;
    }
    else if(memcmp(section_header->section_identify, RITE_SECTION_LIENO_IDENTIFIER, sizeof(section_header->section_identify)) == 0) {
      result = read_rite_section_lineno(mrb, bin, sirep);
      if(result < MRB_DUMP_OK) {
        return result;
      }
    }
    bin += bin_to_uint32(section_header->section_size);
  } while(memcmp(section_header->section_identify, RITE_BINARY_EOF, sizeof(section_header->section_identify)) != 0);

  return total_nirep;
}

static void
irep_error(mrb_state *mrb, int n)
{
  static const char msg[] = "irep load error";
  mrb->exc = mrb_obj_ptr(mrb_exc_new(mrb, E_SCRIPT_ERROR, msg, sizeof(msg) - 1));
}

mrb_value
mrb_load_irep(mrb_state *mrb, const uint8_t *bin)
{
  int32_t n;

  n = mrb_read_irep(mrb, bin);
  if (n < 0) {
    irep_error(mrb, n);
    return mrb_nil_value();
  }
  return mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
}

#ifdef ENABLE_STDIO

static int32_t
read_rite_section_lineno_file(mrb_state *mrb, FILE *fp, size_t sirep)
{
  int32_t result;
  size_t i;
  uint16_t nirep;
  uint16_t n;
  uint32_t len, buf_size;
  uint8_t *buf = NULL;
  const size_t record_header_size = 4;

  struct rite_section_lineno_header header;
  if (fread(&header, sizeof(struct rite_section_lineno_header), 1, fp) == 0) {
    return MRB_DUMP_READ_FAULT;
  }

  nirep = bin_to_uint16(header.nirep);

  buf_size = record_header_size;
  buf = (uint8_t *)mrb_malloc(mrb, buf_size);
  
  //Read Binary Data Section
  for (n = 0, i = sirep; n < nirep; n++, i++) {
    if (fread(buf, record_header_size, 1, fp) == 0) {
      result = MRB_DUMP_READ_FAULT;
      goto error_exit;
    }
    buf_size = bin_to_uint32(&buf[0]);
    buf = (uint8_t *)mrb_realloc(mrb, buf, buf_size);

    if (fread(&buf[record_header_size], buf_size - record_header_size, 1, fp) == 0) {
      result = MRB_DUMP_READ_FAULT;
      goto error_exit;
    }
    result = read_rite_lineno_record(mrb, buf, i, &len);
    if (result != MRB_DUMP_OK)
      goto error_exit;
  }

  result = MRB_DUMP_OK;
error_exit:
  mrb_free(mrb, buf);
  if (result != MRB_DUMP_OK) {
    for (i = sirep; i < mrb->irep_len; i++) {
      if (mrb->irep[i]) {
        if (mrb->irep[i]->iseq)
          mrb_free(mrb, mrb->irep[i]->iseq);

        if (mrb->irep[i]->pool)
          mrb_free(mrb, mrb->irep[i]->pool);

        if (mrb->irep[i]->syms)
          mrb_free(mrb, mrb->irep[i]->syms);

        mrb_free(mrb, mrb->irep[i]);
      }
    }
    return result;
  }
  return sirep + bin_to_uint16(header.sirep);
}

static int32_t
read_rite_section_irep_file(mrb_state *mrb, FILE *fp)
{
  int32_t result;
  size_t sirep;
  size_t i;
  uint16_t nirep;
  uint16_t n;
  uint32_t len, buf_size;
  uint8_t *buf = NULL;
  const size_t record_header_size = 1 + 4;
  struct rite_section_irep_header header;

  if (fread(&header, sizeof(struct rite_section_irep_header), 1, fp) == 0) {
    return MRB_DUMP_READ_FAULT;
  }

  sirep = mrb->irep_len;
  nirep = bin_to_uint16(header.nirep);

  buf_size = record_header_size;
  buf = (uint8_t *)mrb_malloc(mrb, buf_size);
  
  //Read Binary Data Section
  for (n = 0, i = sirep; n < nirep; n++, i++) {
    if (fread(buf, record_header_size, 1, fp) == 0) {
      result = MRB_DUMP_READ_FAULT;
      goto error_exit;
    }
    buf_size = bin_to_uint32(&buf[0]);
    buf = (uint8_t *)mrb_realloc(mrb, buf, buf_size);
    if (fread(&buf[record_header_size], buf_size - record_header_size, 1, fp) == 0) {
      result = MRB_DUMP_READ_FAULT;
      goto error_exit;
    }
    result = read_rite_irep_record(mrb, buf, &len);
    if (result != MRB_DUMP_OK)
      goto error_exit;
  }

  result = MRB_DUMP_OK;
error_exit:
  mrb_free(mrb, buf);
  if (result != MRB_DUMP_OK) {
    for (i = sirep; i < mrb->irep_len; i++) {
      if (mrb->irep[i]) {
        if (mrb->irep[i]->iseq)
          mrb_free(mrb, mrb->irep[i]->iseq);

        if (mrb->irep[i]->pool)
          mrb_free(mrb, mrb->irep[i]->pool);

        if (mrb->irep[i]->syms)
          mrb_free(mrb, mrb->irep[i]->syms);

        mrb_free(mrb, mrb->irep[i]);
      }
    }
    return result;
  }
  return sirep + bin_to_uint16(header.sirep);
}

int32_t
mrb_read_irep_file(mrb_state *mrb, FILE* fp)
{
  int result;
  int32_t total_nirep = 0;
  uint8_t *buf;
  uint16_t crc, crcwk = 0;
  uint32_t section_size = 0;
  size_t nbytes;
  size_t sirep;
  struct rite_section_header section_header;
  long fpos;
  const size_t block_size = 1 << 14;
  const size_t buf_size = sizeof(struct rite_binary_header);

  if ((mrb == NULL) || (fp == NULL)) {
    return MRB_DUMP_INVALID_ARGUMENT;
  }

  buf = mrb_malloc(mrb, buf_size);
  if (fread(buf, buf_size, 1, fp) == 0) {
    mrb_free(mrb, buf);
    return MRB_DUMP_READ_FAULT;
  }
  result = read_rite_binary_header(buf, NULL, &crc);
  mrb_free(mrb, buf);
  if(result != MRB_DUMP_OK) {
    return result;
  }

  /* verify CRC */
  fpos = ftell(fp);
  buf = mrb_malloc(mrb, block_size);
  fseek(fp, offset_crc_body(), SEEK_SET);
  while((nbytes = fread(buf, 1, block_size, fp)) > 0) {
    crcwk = calc_crc_16_ccitt(buf, nbytes, crcwk);
  }
  mrb_free(mrb, buf);
  if (nbytes == 0 && ferror(fp)) {
    return MRB_DUMP_READ_FAULT;
  }
  if(crcwk != crc) {
    return MRB_DUMP_INVALID_FILE_HEADER;
  }
  fseek(fp, fpos + section_size, SEEK_SET);
  sirep = mrb->irep_len;

  // read sections
  do {
    fpos = ftell(fp);
    if (fread(&section_header, sizeof(struct rite_section_header), 1, fp) == 0) {
      return MRB_DUMP_READ_FAULT;
    }
    section_size = bin_to_uint32(section_header.section_size);

    if(memcmp(section_header.section_identify, RITE_SECTION_IREP_IDENTIFIER, sizeof(section_header.section_identify)) == 0) {
      fseek(fp, fpos, SEEK_SET);
      result = read_rite_section_irep_file(mrb, fp);
      if(result < MRB_DUMP_OK) {
        return result;
      }
      total_nirep += result;
    }
    else if(memcmp(section_header.section_identify, RITE_SECTION_LIENO_IDENTIFIER, sizeof(section_header.section_identify)) == 0) {
      fseek(fp, fpos, SEEK_SET);
      result = read_rite_section_lineno_file(mrb, fp, sirep);
      if(result < MRB_DUMP_OK) {
        return result;
      }
    }

    fseek(fp, fpos + section_size, SEEK_SET);
  } while(memcmp(section_header.section_identify, RITE_BINARY_EOF, sizeof(section_header.section_identify)) != 0);

  return total_nirep;
}

mrb_value
mrb_load_irep_file(mrb_state *mrb, FILE* fp)
{
  int n = mrb_read_irep_file(mrb, fp);

  if (n < 0) {
    irep_error(mrb, n);
    return mrb_nil_value();
  }
  return mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
}
#endif /* ENABLE_STDIO */
