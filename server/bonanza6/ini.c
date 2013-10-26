﻿// $Id: ini.c,v 1.6 2012-04-11 06:35:48 eikii Exp $

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#if ! defined(_WIN32)
#  include <unistd.h>
#endif

#include "shogi.h"

#ifdef CLUSTER_PARALLEL
#include "../if_bonanza.h"
#endif

#ifdef FVBIN_MMAP
// for mmap, open
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#if   defined(_MSC_VER)
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#else
static int first_one00( int pcs );
static int last_one00( int pcs );
#endif


static void ini_check_table( void );
static bitboard_t bb_set_mask( int isquare );
static void set_attacks( int irank, int ifilea, bitboard_t *pbb );
static void ini_is_same( void );
static void ini_tables( void );
static void ini_attack_tables( void );
static void ini_random_table( void );

int CONV load_fv( void )
{
#if 0
  FILE *pf;
  size_t size;
  int iret;

  pf = file_open( str_fv, "rb" );
  if ( pf == NULL ) { return -2; }

  size = nsquare * pos_n;
  if ( fread( pc_on_sq, sizeof(short), size, pf ) != size )
    {
      str_error = str_io_error;
      return -2;
    }

  size = nsquare * nsquare * kkp_end;
  if ( fread( kkp, sizeof(short), size, pf ) != size )
    {
      str_error = str_io_error;
      return -2;
    }

  iret = file_close( pf );
  if ( iret < 0 ) { return iret; }
#elif ! defined(FVBIN_MMAP)
  FILE *pf;
  size_t size;
  int iret;

  pf = file_open( "fv3.bin", "rb" );
  if ( pf == NULL ) { return -2; }

  size = nsquare * fe_end * fe_end;
  pc_on_sq = (pconsqAry *)memory_alloc( sizeof(short) * size );
  if ( fread( pc_on_sq, sizeof(short), size, pf ) != size )
    {
      memory_free(pc_on_sq);
      str_error = str_io_error;
      return -2;
    }

  size = nsquare * nsquare * kkp_end;
  kkp = (kkpAry *)memory_alloc( sizeof(short) * size );
  if ( fread( kkp, sizeof(short), size, pf ) != size )
    {
      memory_free(kkp);
      memory_free(pc_on_sq);
      str_error = str_io_error;
      return -2;
    }

  iret = file_close( pf );
  if ( iret < 0 ) { return iret; }
#else
  int fd;
  size_t sz1, sz2, j;
  void* mapbase;

#ifndef USE_FV3
  fd = open(str_fv, O_RDONLY);
  sz1 = nsquare * pos_n;
#else
  fd = open("fv3.bin", O_RDONLY);
  sz1 = nsquare * fe_end * fe_end;
#endif
  sz2 = nsquare * nsquare * kkp_end;

   //          hint_adr                                   offset
  mapbase = mmap( NULL, (sz1+sz2) * sizeof(short),
                  PROT_READ, MAP_SHARED, fd, 0 );
  if ( mapbase == MAP_FAILED ) { return -2; }
  pc_on_sq = (pconsqAry*)mapbase;
  kkp = (kkpAry*)(mapbase + sz1 * sizeof(short));
  close(fd);

  { // read in all the area into memory
    int x = 0;
    char* p = (char*)mapbase;
    for ( j = 0; j < (sz1+sz2)*sizeof(short); j += 4096 )
      x += p[j];
    if ( x == 43256 ) printf(".");
  }
#endif

#if 0
#  define X0 -10000
#  define X1 +10000
  {
    unsigned int a[X1-X0+1];
    int i, n, iv;

    for ( i = 0; i < X1-X0+1; i++ ) { a[i] = 0; }
    n = nsquare * pos_n;
    for ( i = 0; i < n; i++ )
      {
        iv = pc_on_sq[0][i];
        if      ( iv < X0 ) { iv = X0; }
        else if ( iv > X1 ) { iv = X1; }
        a[ iv - X0 ] += 1;
      }

    pf = file_open( "dist.dat", "w" );
    if ( pf == NULL ) { return -2; }

    for ( i = X0; i <= X1; i++ ) { fprintf( pf, "%d %d\n", i, a[i-X0] ); }

    iret = file_close( pf );
    if ( iret < 0 ) { return iret; }
  }
#  undef x0
#  undef x1
#endif

  return 1;
}


int
ini( tree_t * restrict ptree )
{
  int i;

  if ( read_handjoseki() < 0 ) { return -1; }
  if ( load_fv() < 0 ) { return -1; }

  for ( i = 0; i < 31; i++ ) { p_value[i]       = 0; }
  for ( i = 0; i < 31; i++ ) { p_value_ex[i]    = 0; }
  for ( i = 0; i < 15; i++ ) { p_value_pm[i] = 0; }
  p_value[15+pawn]       = DPawn;
  p_value[15+lance]      = DLance;
  p_value[15+knight]     = DKnight;
  p_value[15+silver]     = DSilver;
  p_value[15+gold]       = DGold;
  p_value[15+bishop]     = DBishop;
  p_value[15+rook]       = DRook;
  p_value[15+king]       = DKing;
  p_value[15+pro_pawn]   = DProPawn;
  p_value[15+pro_lance]  = DProLance;
  p_value[15+pro_knight] = DProKnight;
  p_value[15+pro_silver] = DProSilver;
  p_value[15+horse]      = DHorse;
  p_value[15+dragon]     = DDragon;

  game_status           = 0;
  str_buffer_cmdline[0] = '\0';
  ptrans_table_orig     = NULL;
  record_game.pf        = NULL;
  node_per_second       = TIME_CHECK_MIN_NODE;
  resign_threshold      = RESIGN_THRESHOLD;
  node_limit            = UINT64_MAX;
  time_response         = TIME_RESPONSE;
  sec_limit             = 0;
  sec_limit_up          = 10U;
  sec_limit_depth       = UINT_MAX;
  depth_limit           = PLY_MAX;
#ifndef CLUSTER_PARALLEL
  log2_ntrans_table     = 20;
#endif
  record_num            = -1;

  pf_book               = NULL;

  for ( i = 0; i < NUM_UNMAKE; i++ )
    {
      amove_save[i]            = MOVE_NA;
      amaterial_save[i]        = 0;
      ansuc_check_save[i]      = 0;
      alast_root_value_save[i] = 0;
      alast_pv_save[i].a[0]    = 0;
      alast_pv_save[i].a[1]    = 0;
      alast_pv_save[i].depth   = 0;
      alast_pv_save[i].length  = 0;
      alast_pv_save[i].type    = no_rep;
    }

#if defined(TLP)

#ifndef CLUSTER_PARALLEL
  use_cpu_affinity      = -1;
#endif
  tlp_max        = 1;
  tlp_abort      = 0;
  tlp_num        = 0;
  tlp_idle       = 0;
  tlp_atree_work[0].tlp_id           = 0;
  tlp_atree_work[0].tlp_abort        = 0;
  tlp_atree_work[0].tlp_used         = 1;
  tlp_atree_work[0].tlp_slot         = 0;
  tlp_atree_work[0].tlp_nsibling     = 0;
  if ( lock_init( &tlp_atree_work[0].tlp_lock ) < 0 ) { return -1; }
  if ( lock_init( &tlp_lock )                   < 0 ) { return -1; }
  for ( i = 0; i < tlp_max; i++ )
    {
      tlp_atree_work->tlp_ptrees_sibling[i] = NULL;
    }
  for ( i = 1; i < TLP_NUM_WORK; i++ )
    {
      tlp_atree_work[i].tlp_slot = (unsigned short)i;
      tlp_atree_work[i].tlp_used = 0;
      if ( lock_init( &tlp_atree_work[i].tlp_lock ) < 0 ) { return -1; }
    }
#endif

#if ! defined(_WIN32) && ( defined(DFPN_CLIENT) || defined(TLP) )
  if ( pthread_attr_init( &pthread_attr )
       || pthread_attr_setdetachstate( &pthread_attr,
                                       PTHREAD_CREATE_DETACHED ) )
    {
      str_error = "pthread_attr_init() failed.";
      return -1;
    }
#endif


#if defined(CSA_LAN)
  sckt_csa       = SCKT_NULL;
  time_last_send = 0U;
#endif

#if defined(MNJ_LAN) || defined(USI)
  moves_ignore[0] = MOVE_NA;
#endif

#if defined(MNJ_LAN)
  sckt_mnj              = SCKT_NULL;
  mnj_posi_id           = 0;
  time_last_send        = 0U;
#endif

#if defined(TLP) || defined(DFPN_CLIENT)
  if ( lock_init( &io_lock ) < 0 ) { return -1; }
#endif

#if defined(DFPN_CLIENT)
  if ( lock_init( &dfpn_client_lock ) < 0 ) { return -1; }
  dfpn_client_str_move[0]  = '\0';
  dfpn_client_str_addr[0]  = '\0';
  dfpn_client_signature[0] = '\0';
  dfpn_client_rresult      = dfpn_client_na;
  dfpn_client_num_cresult  = 0;
  dfpn_client_flag_read    = 0;
  dfpn_client_sckt         = SCKT_NULL;
#endif

#if defined(USI)
  usi_byoyomi         = 0;
  if ( usi_mode != usi_off )
    {
      game_status      |= flag_noprompt;
      game_status      |= flag_nostdout;
      game_status      |= flag_noponder;
      game_status      |= flag_nobeep;
      resign_threshold  = 65535;
    }
#endif

#if defined(_WIN32)
#else
  clk_tck = (clock_t)sysconf(_SC_CLK_TCK);
#endif

#if ! defined(NO_LOGGING)
  pf_log = NULL;
#endif

#if   defined(_MSC_VER)
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#else
  for ( i = 0; i < 0x200; i++ )
    {
      aifirst_one[i] = (unsigned char)first_one00(i);
      ailast_one[i]  = (unsigned char)last_one00(i);
    }
#endif
  
  ini_rand( 5489U );
  ini_is_same();
  ini_tables();
  ini_attack_tables();
  ini_random_table();
  ini_check_table();
  ini_mate3();
  set_derivative_param();

#ifdef BITBRD64
  ini_bitboards();
#endif

  if ( ini_game( ptree, &min_posi_no_handicap, flag_history, NULL, NULL ) < 0 )
    {
      return -1;
    }

  OutCsaShogi( "%s\n", str_myname );
  Out( "%s\n", str_myname );

  if ( ini_trans_table() < 0 ) { return -1; }

#if defined(DFPN)
  dfpn_sckt      = SCKT_NULL;
  dfpn_hash_log2 = 20;
  if ( dfpn_ini_hash() < 0 ) { return -1; }
#endif

#ifdef CLUSTER_PARALLEL
  if ( !Mproc )
#endif
  {
    if ( book_on() < 0 ) { out_warning( "%s", str_error );}
    else                 { Out( "%s found\n", str_book );}
  }

  if ( get_elapsed( &time_turn_start ) < 0 ) { return -1; }

  ini_rand( time_turn_start );
  Out( "rand seed = %x\n", time_turn_start );

#if defined(MPV)
  mpv_num   = 1;
  mpv_width = 2 * MT_CAP_PAWN;
#endif

#if defined(INANIWA_SHIFT)
  inaniwa_flag = 0;
#endif

  return 1;
}


int
fin( void )
{
#if defined(TLP)
  int i;
#endif

  memory_free( (void *)ptrans_table_orig );

#if ! defined(FVBIN_MMAP)
  if ( pc_on_sq != NULL )
     {
       memory_free( pc_on_sq );
       pc_on_sq = NULL;
     }
  if ( kkp != NULL )
     {
       memory_free( kkp );
       kkp = NULL;
     }
#endif

#if defined(TLP) || defined(DFPN_CLIENT)
  if ( lock_free( &io_lock ) < 0 ) { return -1; }
#endif

#if defined(DFPN_CLIENT)
  if ( lock_free( & dfpn_client_lock ) < 0 ) { return -1; }
#endif

#if defined(TLP)
  tlp_abort = 1;
  while ( tlp_num ) { tlp_yield(); }
  if ( lock_free( &tlp_atree_work[0].tlp_lock ) < 0 ) { return -1; }
  if ( lock_free( &tlp_lock )                   < 0 ) { return -1; }
  for ( i = 1; i < TLP_NUM_WORK; i++ )
    {
      if ( lock_free( &tlp_atree_work[i].tlp_lock ) < 0 ) { return -1; }
    }
#endif

#if ! defined(_WIN32) && ( defined(DFPN_CLIENT) || defined(TLP) )
  if ( pthread_attr_destroy( &pthread_attr ) )
    {
      str_error = "pthread_attr_destroy() failed.";
      return -1;
    }
#endif

  if ( book_off() < 0 ) { return -1; }
  if ( record_close( &record_game ) < 0 ) { return -1; }
#if ! defined(NO_LOGGING)
  if ( file_close( pf_log ) < 0 ) { return -1; }
#endif

  ShutdownAll();
  return 1;
}


void
set_derivative_param( void )
{
  p_value_ex[15+pawn]       = p_value[15+pawn]       + p_value[15+pawn];
  p_value_ex[15+lance]      = p_value[15+lance]      + p_value[15+lance];
  p_value_ex[15+knight]     = p_value[15+knight]     + p_value[15+knight];
  p_value_ex[15+silver]     = p_value[15+silver]     + p_value[15+silver];
  p_value_ex[15+gold]       = p_value[15+gold]       + p_value[15+gold];
  p_value_ex[15+bishop]     = p_value[15+bishop]     + p_value[15+bishop];
  p_value_ex[15+rook]       = p_value[15+rook]       + p_value[15+rook];
  p_value_ex[15+king]       = p_value[15+king]       + p_value[15+king];
  p_value_ex[15+pro_pawn]   = p_value[15+pro_pawn]   + p_value[15+pawn];
  p_value_ex[15+pro_lance]  = p_value[15+pro_lance]  + p_value[15+lance];
  p_value_ex[15+pro_knight] = p_value[15+pro_knight] + p_value[15+knight];
  p_value_ex[15+pro_silver] = p_value[15+pro_silver] + p_value[15+silver];
  p_value_ex[15+horse]      = p_value[15+horse]      + p_value[15+bishop];
  p_value_ex[15+dragon]     = p_value[15+dragon]     + p_value[15+rook];

  p_value_pm[7+pawn]     = p_value[15+pro_pawn]   - p_value[15+pawn];
  p_value_pm[7+lance]    = p_value[15+pro_lance]  - p_value[15+lance];
  p_value_pm[7+knight]   = p_value[15+pro_knight] - p_value[15+knight];
  p_value_pm[7+silver]   = p_value[15+pro_silver] - p_value[15+silver];
  p_value_pm[7+bishop]   = p_value[15+horse]      - p_value[15+bishop];
  p_value_pm[7+rook]     = p_value[15+dragon]     - p_value[15+rook];

  p_value[15-pawn]          = p_value[15+pawn];
  p_value[15-lance]         = p_value[15+lance];
  p_value[15-knight]        = p_value[15+knight];
  p_value[15-silver]        = p_value[15+silver];
  p_value[15-gold]          = p_value[15+gold];
  p_value[15-bishop]        = p_value[15+bishop];
  p_value[15-rook]          = p_value[15+rook];
  p_value[15-king]          = p_value[15+king];
  p_value[15-pro_pawn]      = p_value[15+pro_pawn];
  p_value[15-pro_lance]     = p_value[15+pro_lance];
  p_value[15-pro_knight]    = p_value[15+pro_knight];
  p_value[15-pro_silver]    = p_value[15+pro_silver];
  p_value[15-horse]         = p_value[15+horse];
  p_value[15-dragon]        = p_value[15+dragon];

  p_value_ex[15-pawn]       = p_value_ex[15+pawn];
  p_value_ex[15-lance]      = p_value_ex[15+lance];
  p_value_ex[15-knight]     = p_value_ex[15+knight];
  p_value_ex[15-silver]     = p_value_ex[15+silver];
  p_value_ex[15-gold]       = p_value_ex[15+gold];
  p_value_ex[15-bishop]     = p_value_ex[15+bishop];
  p_value_ex[15-rook]       = p_value_ex[15+rook];
  p_value_ex[15-king]       = p_value_ex[15+king];
  p_value_ex[15-pro_pawn]   = p_value_ex[15+pro_pawn];
  p_value_ex[15-pro_lance]  = p_value_ex[15+pro_lance];
  p_value_ex[15-pro_knight] = p_value_ex[15+pro_knight];
  p_value_ex[15-pro_silver] = p_value_ex[15+pro_silver];
  p_value_ex[15-horse]      = p_value_ex[15+horse];
  p_value_ex[15-dragon]     = p_value_ex[15+dragon];

  p_value_pm[7-pawn]     = p_value_pm[7+pawn];
  p_value_pm[7-lance]    = p_value_pm[7+lance];
  p_value_pm[7-knight]   = p_value_pm[7+knight];
  p_value_pm[7-silver]   = p_value_pm[7+silver];
  p_value_pm[7-bishop]   = p_value_pm[7+bishop];
  p_value_pm[7-rook]     = p_value_pm[7+rook];
}


static void
ini_is_same( void )
{
  int p[16], i, j;

  for ( i = 0; i < 16; i++ ) { p[i] = 0; }

  p[pawn]       =  1;
  p[lance]      =  3;
  p[pro_pawn]   =  3;
  p[knight]     =  3;
  p[pro_lance]  =  3;
  p[pro_knight] =  3;
  p[silver]     =  4;
  p[pro_silver] =  4;
  p[gold]       =  5;
  p[bishop]     =  6;
  p[horse]      =  7;
  p[rook]       =  7;
  p[dragon]     =  8;
  p[king]       = 99;

  for ( i = 0; i < 16; i++ )
    for ( j = 0; j < 16; j++ )
      {
        if      ( p[i] < p[j]-1 ) { is_same[i][j] = 2U; }
        else if ( p[i] > p[j]+1 ) { is_same[i][j] = 1U; }
        else                      { is_same[i][j] = 0U; }
      }
}


static void
ini_tables( void )
{
  const unsigned char aini_rl90[] = { A1, A2, A3, A4, A5, A6, A7, A8, A9,
                                      B1, B2, B3, B4, B5, B6, B7, B8, B9,
                                      C1, C2, C3, C4, C5, C6, C7, C8, C9,
                                      D1, D2, D3, D4, D5, D6, D7, D8, D9,
                                      E1, E2, E3, E4, E5, E6, E7, E8, E9,
                                      F1, F2, F3, F4, F5, F6, F7, F8, F9,
                                      G1, G2, G3, G4, G5, G6, G7, G8, G9,
                                      H1, H2, H3, H4, H5, H6, H7, H8, H9,
                                      I1, I2, I3, I4, I5, I6, I7, I8, I9 };
  
  const unsigned char aini_rl45[] = { A9, B1, C2, D3, E4, F5, G6, H7, I8,
                                      A8, B9, C1, D2, E3, F4, G5, H6, I7,
                                      A7, B8, C9, D1, E2, F3, G4, H5, I6,
                                      A6, B7, C8, D9, E1, F2, G3, H4, I5,
                                      A5, B6, C7, D8, E9, F1, G2, H3, I4,
                                      A4, B5, C6, D7, E8, F9, G1, H2, I3,
                                      A3, B4, C5, D6, E7, F8, G9, H1, I2,
                                      A2, B3, C4, D5, E6, F7, G8, H9, I1,
                                      A1, B2, C3, D4, E5, F6, G7, H8, I9 };
  
  const unsigned char aini_rr45[] = { I8, I7, I6, I5, I4, I3, I2, I1, I9,
                                      H7, H6, H5, H4, H3, H2, H1, H9, H8,
                                      G6, G5, G4, G3, G2, G1, G9, G8, G7,
                                      F5, F4, F3, F2, F1, F9, F8, F7, F6,
                                      E4, E3, E2, E1, E9, E8, E7, E6, E5,
                                      D3, D2, D1, D9, D8, D7, D6, D5, D4,
                                      C2, C1, C9, C8, C7, C6, C5, C4, C3,
                                      B1, B9, B8, B7, B6, B5, B4, B3, B2,
                                      A9, A8, A7, A6, A5, A4, A3, A2, A1 };
  bitboard_t abb_plus1dir[ nsquare ];   // 右
  bitboard_t abb_plus8dir[ nsquare ];   // 左下
  bitboard_t abb_plus9dir[ nsquare ];   // 下
  bitboard_t abb_plus10dir[ nsquare ];  // 右下
  bitboard_t abb_minus1dir[ nsquare ];  // 左
  bitboard_t abb_minus8dir[ nsquare ];  // 右上
  bitboard_t abb_minus9dir[ nsquare ];  // 上
  bitboard_t abb_minus10dir[ nsquare ]; // 左上
  bitboard_t bb;
  int isquare, i, ito, ifrom, irank, ifile;
  int isquare_rl90, isquare_rl45, isquare_rr45;

  for ( isquare = 0; isquare < nsquare; isquare++ )
    {
      isquare_rl90 = aini_rl90[isquare];
      isquare_rl45 = aini_rl45[isquare];
      isquare_rr45 = aini_rr45[isquare];
      abb_mask[isquare]      = bb_set_mask( isquare );
      abb_mask_rl90[isquare] = bb_set_mask( isquare_rl90 );
      abb_mask_rl45[isquare] = bb_set_mask( isquare_rl45 );
      abb_mask_rr45[isquare] = bb_set_mask( isquare_rr45 );
    }

  for ( irank = 0; irank < nrank; irank++ )
    for ( ifile = 0; ifile < nfile; ifile++ )
      {
        isquare = irank*nfile + ifile;
        BBIni( abb_plus1dir[isquare] );
        BBIni( abb_plus8dir[isquare] );
        BBIni( abb_plus9dir[isquare] );
        BBIni( abb_plus10dir[isquare] );
        BBIni( abb_minus1dir[isquare] );
        BBIni( abb_minus8dir[isquare] );
        BBIni( abb_minus9dir[isquare] );
        BBIni( abb_minus10dir[isquare] );
        for ( i = 1; i < nfile; i++ )
          {
            set_attacks( irank,   ifile+i, abb_plus1dir   + isquare );
            set_attacks( irank+i, ifile-i, abb_plus8dir   + isquare );
            set_attacks( irank+i, ifile,   abb_plus9dir   + isquare );
            set_attacks( irank+i, ifile+i, abb_plus10dir  + isquare );
            set_attacks( irank,   ifile-i, abb_minus1dir  + isquare );
            set_attacks( irank-i, ifile+i, abb_minus8dir  + isquare );
            set_attacks( irank-i, ifile,   abb_minus9dir  + isquare );
            set_attacks( irank-i, ifile-i, abb_minus10dir + isquare );
          }
      }


  for ( isquare = 0; isquare < nsquare; isquare++ )
    {
      BBOr( abb_plus_rays[isquare],
            abb_plus1dir[isquare],  abb_plus8dir[isquare] );
      BBOr( abb_plus_rays[isquare],
            abb_plus_rays[isquare], abb_plus9dir[isquare] );
      BBOr( abb_plus_rays[isquare],
            abb_plus_rays[isquare], abb_plus10dir[isquare] );
      BBOr( abb_minus_rays[isquare],
            abb_minus1dir[isquare],  abb_minus8dir[isquare] );
      BBOr( abb_minus_rays[isquare],
            abb_minus_rays[isquare], abb_minus9dir[isquare] );
      BBOr( abb_minus_rays[isquare],
            abb_minus_rays[isquare], abb_minus10dir[isquare] );
    }


  // adirecはifromとitoの位置関係を設定する
  // 横移動であればdirec_rank、
  // 縦移動であればdirect_fileが設定される
  for ( ifrom = 0; ifrom < nsquare; ifrom++ )
    {
      for ( ito = 0; ito < 128; ito++ )
        {
          adirec[ifrom][ito] = direc_misc;
        }

      BBOr( bb, abb_plus1dir[ifrom], abb_minus1dir[ifrom] );
      while ( BBTest(bb) )
        {
          ito = FirstOne( bb );
          adirec[ifrom][ito]  = direc_rank;
          Xor( ito, bb );
        }
      BBOr( bb, abb_plus8dir[ifrom], abb_minus8dir[ifrom] );
      while ( BBTest(bb) )
        {
          ito = FirstOne( bb );
          adirec[ifrom][ito]  = direc_diag1;
          Xor( ito, bb );
        }
      BBOr( bb, abb_plus9dir[ifrom], abb_minus9dir[ifrom] );
      while ( BBTest(bb) )
        {
          ito = FirstOne( bb );
          adirec[ifrom][ito]  = direc_file;
          Xor( ito, bb );
        }
      BBOr( bb, abb_plus10dir[ifrom], abb_minus10dir[ifrom] );
      while ( BBTest(bb) )
        {
          ito = FirstOne( bb );
          adirec[ifrom][ito]  = direc_diag2;
          Xor( ito, bb );
        }
    }

  // abb_obstacleは、ifromからitoへの障害物を設定する
  // ifromとitoが縦、横、斜めの関係になければゼロ
  // さもなくば、その間のマス目が1であるようなbitboard
  // (ただしそのbitboardでifrom,itoに相当する箇所は0)
  for ( ifrom = 0; ifrom < nsquare; ifrom++ )
    for ( ito = 0; ito < nsquare; ito++ )
      {
        BBIni( abb_obstacle[ifrom][ito] );

        // ifromから見て
        if ( ifrom > ito ) switch ( adirec[ifrom][ito] )
          {
          case direc_misc:
            break;
          case direc_rank: // 左
            BBXor( abb_obstacle[ifrom][ito],
                   abb_minus1dir[ito+1], abb_minus1dir[ifrom] );
            break;
          case direc_file: // 上
            BBXor( abb_obstacle[ifrom][ito],
                   abb_minus9dir[ito+9], abb_minus9dir[ifrom] );
            break;
          case direc_diag1: // 右上
            BBXor( abb_obstacle[ifrom][ito],
                   abb_minus8dir[ito+8], abb_minus8dir[ifrom] );
            break;
          case direc_diag2: // 左上
            BBXor( abb_obstacle[ifrom][ito],
                   abb_minus10dir[ito+10], abb_minus10dir[ifrom] );
            break;
          default:
            unreachable();
            break;
          }
        else switch ( adirec[ifrom][ito] )
          {
          case direc_misc:
            break;
          case direc_rank: // 右
            BBXor( abb_obstacle[ifrom][ito],
                   abb_plus1dir[ito-1], abb_plus1dir[ifrom] );
            break;
          case direc_file: // 下
            BBXor( abb_obstacle[ifrom][ito],
                   abb_plus9dir[ito-9], abb_plus9dir[ifrom] );
            break;
          case direc_diag1: // 左下
            BBXor( abb_obstacle[ifrom][ito],
                   abb_plus8dir[ito-8], abb_plus8dir[ifrom] );
            break;
          case direc_diag2: // 右下
            BBXor( abb_obstacle[ifrom][ito],
                   abb_plus10dir[ito-10], abb_plus10dir[ifrom] );
            break;
          default:
            unreachable();
            break;
          }
      }
}


static void
ini_random_table( void )
{
  int i;

  for ( i = 0; i < nsquare; i++ )
    {
      b_pawn_rand[ i ]       = rand64();
      b_lance_rand[ i ]      = rand64();
      b_knight_rand[ i ]     = rand64();
      b_silver_rand[ i ]     = rand64();
      b_gold_rand[ i ]       = rand64();
      b_bishop_rand[ i ]     = rand64();
      b_rook_rand[ i ]       = rand64();
      b_king_rand[ i ]       = rand64();
      b_pro_pawn_rand[ i ]   = rand64();
      b_pro_lance_rand[ i ]  = rand64();
      b_pro_knight_rand[ i ] = rand64();
      b_pro_silver_rand[ i ] = rand64();
      b_horse_rand[ i ]      = rand64();
      b_dragon_rand[ i ]     = rand64();
      w_pawn_rand[ i ]       = rand64();
      w_lance_rand[ i ]      = rand64();
      w_knight_rand[ i ]     = rand64();
      w_silver_rand[ i ]     = rand64();
      w_gold_rand[ i ]       = rand64();
      w_bishop_rand[ i ]     = rand64();
      w_rook_rand[ i ]       = rand64();
      w_king_rand[ i ]       = rand64();
      w_pro_pawn_rand[ i ]   = rand64();
      w_pro_lance_rand[ i ]  = rand64();
      w_pro_knight_rand[ i ] = rand64();
      w_pro_silver_rand[ i ] = rand64();
      w_horse_rand[ i ]      = rand64();
      w_dragon_rand[ i ]     = rand64();
    }

  for ( i = 0; i < npawn_max; i++ )
    {
      b_hand_pawn_rand[ i ]   = rand64();
      w_hand_pawn_rand[ i ]   = rand64();
    }

  for ( i = 0; i < nlance_max; i++ )
    {
      b_hand_lance_rand[ i ]  = rand64();
      b_hand_knight_rand[ i ] = rand64();
      b_hand_silver_rand[ i ] = rand64();
      b_hand_gold_rand[ i ]   = rand64();
      w_hand_lance_rand[ i ]  = rand64();
      w_hand_knight_rand[ i ] = rand64();
      w_hand_silver_rand[ i ] = rand64();
      w_hand_gold_rand[ i ]   = rand64();
    }

  for ( i = 0; i < nbishop_max; i++ )
    {
      b_hand_bishop_rand[ i ] = rand64();
      b_hand_rook_rand[ i ]   = rand64();
      w_hand_bishop_rand[ i ] = rand64();
      w_hand_rook_rand[ i ]   = rand64();
    }
}


static void
ini_attack_tables( void )
{
  int irank, ifile, pcs, i;
  bitboard_t bb;

  // 以下、aslideのテーブル初期化コード
  for ( i = 0; i < nsquare; i++ )
    {
      // そのnsquareが何番目のbitboardに属するのか
      // 000000000
      // 000000000
      // 000000000
      // 111111111
      // 111111111
      // 111111111
      // 222222222
      // 222222222
      // 222222222
      aslide[i].ir0 = (unsigned char)(i/(nfile*3));

      // bitboardのp[N]から、その段の2～8筋(7bit)を取り出すのに必要なシフト回数
      // 何故7bitで良いかというと、駒を捕獲しない指し手の生成だから、
      // 例えば飛車が5筋にいて、2,3,4筋に駒がなければ1筋まで行けるのは確定で、
      // 1筋に仮に駒があれば、それは捕獲する移動だから自動的に除外されるから、
      // 飛車の利きを最初に調べるときに1筋まで考慮する必要はないのである。
      // よって、2～8筋の駒の状態だけ調べれば十分。
      //
      // 19 19 19 ..
      // 10 10 10 ..
      //  1  1  1 ..
      // 19 19 19 ..
      // 10 10 10 ..
      //  1  1  1 ..
      // 19 19 19 ..
      // 10 10 10 ..
      //  1  1  1 ..
      aslide[i].sr0 = (unsigned char)((2-(i/nfile)%3)*9+1);

      // ir0を90度回転させたもの。
      // あるBoardPosが occupied_rl90の何番目の要素にあるかを
      // 確定させるのに使う。
      // 
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      // 222111000
      aslide[i].irl90 = (unsigned char)(2-(i%nfile)/3);

      // ↑で求めたoccupied_rl90から、その筋(2～8筋の7bit)を
      // 取り出すときに必要なshiftの回数。
      // srl90はShift number to Rotate turn to the Left 90゜の意味か。
      // 
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      //  1 10 19 ..
      aslide[i].srl90 = (unsigned char)(((i%nfile)%3)*9+1);
    }
  
  for ( irank = 0; irank < nrank; irank++ )
    for ( ifile = 0; ifile < nfile; ifile++ )
      {
        BBIni(bb);
        set_attacks( irank-1, ifile-1, &bb );
        set_attacks( irank-1, ifile+1, &bb );
        set_attacks( irank+1, ifile-1, &bb );
        set_attacks( irank+1, ifile+1, &bb );
        set_attacks( irank-1, ifile, &bb );
        abb_b_silver_attacks[ irank*nfile + ifile ] = bb;
      
        BBIni(bb);
        set_attacks( irank-1, ifile-1, &bb );
        set_attacks( irank-1, ifile+1, &bb );
        set_attacks( irank+1, ifile-1, &bb );
        set_attacks( irank+1, ifile+1, &bb );
        set_attacks( irank+1, ifile,   &bb );
        abb_w_silver_attacks[ irank*nfile + ifile ] = bb;

        BBIni(bb);
        set_attacks( irank-1, ifile-1, &bb );
        set_attacks( irank-1, ifile+1, &bb );
        set_attacks( irank-1, ifile,   &bb );
        set_attacks( irank+1, ifile,   &bb );
        set_attacks( irank,   ifile-1, &bb );
        set_attacks( irank,   ifile+1, &bb );
        abb_b_gold_attacks[ irank*nfile + ifile ] = bb;
      
        BBIni(bb);
        set_attacks( irank+1, ifile-1, &bb );
        set_attacks( irank+1, ifile+1, &bb );
        set_attacks( irank+1, ifile,   &bb );
        set_attacks( irank-1, ifile,   &bb );
        set_attacks( irank,   ifile-1, &bb );
        set_attacks( irank,   ifile+1, &bb );
        abb_w_gold_attacks[ irank*nfile + ifile ] = bb;
      
        BBIni(bb);
        set_attacks( irank+1, ifile-1, &bb );
        set_attacks( irank+1, ifile+1, &bb );
        set_attacks( irank+1, ifile,   &bb );
        set_attacks( irank-1, ifile-1, &bb );
        set_attacks( irank-1, ifile+1, &bb );
        set_attacks( irank-1, ifile,   &bb );
        set_attacks( irank,   ifile-1, &bb );
        set_attacks( irank,   ifile+1, &bb );
        abb_king_attacks[ irank*nfile + ifile ] = bb;
      
        BBIni(bb);
        set_attacks( irank-2, ifile-1, &bb );
        set_attacks( irank-2, ifile+1, &bb );
        abb_b_knight_attacks[ irank*nfile + ifile ] = bb;
      
        BBIni(bb);
        set_attacks( irank+2, ifile-1, &bb );
        set_attacks( irank+2, ifile+1, &bb );
        abb_w_knight_attacks[ irank*nfile + ifile ] = bb;
      

      // abb_file_attacksとは、
      // file = 筋 , file_attacks = 縦方向の利き
      // の意味。
      // abb_file_attacks[boardPos][pcs]
      // を与えると、boardPosから上下に対する利きが一発で求まる。
      // 
      // また、
      // pcs = そのboardPosの属する列の、駒がある位置が1であるbit pattern(7bit)
      //   2の段 = 1 bit目
      //    …
      //   8の段 = 7 bit目
      // に相当するので1,9の段にある障害駒は無視されるが、
      // それらの駒は利きには関係ないので考慮する必要がない。
      // ※なぜなら、1段目に駒があろうとなかろうと下から2段目までに
      // 利きが伸びていて2段目に駒がなければ利きは当然1段目にも
      // 到達しているからで、結局、1段目まで利きが到達するかどうかを
      // 判定するために1段目に駒があるかないかという情報は必要ないのである。
      //
      // あと、特に pcs == 0 のときは、boardPosの位置の上下に発生している
      // 利きを表現する。これは詰み判定のときに役立つ。
      for ( pcs = 0; pcs < 128; pcs++ )
        {
          BBIni(bb);

          // 駒のある位置を除いた上下の利きを求める。
          for ( i = -1; irank+i >= 0; i-- )
            {
              set_attacks( irank+i, ifile, &bb );
              if ( (pcs<<1) & (1 << (8-irank-i)) ) { break; }
            }
          for ( i = 1; irank+i <= 8; i++ )
            {
              set_attacks( irank+i, ifile, &bb );
              if ( (pcs<<1) & (1 << (8-irank-i)) ) { break; }
            }
          abb_file_attacks[irank*nfile+ifile][pcs] = bb; 

          // 駒のある位置を除いた左右の利きを求める。
          BBIni(bb);
          for ( i = -1; ifile+i >= 0; i-- )
            {
              set_attacks( irank, ifile+i, &bb );
              if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
            }
          for ( i = 1; ifile+i <= 8; i++ )
            {
              set_attacks( irank, ifile+i, &bb );
              if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
            }
          abb_rank_attacks[irank*nfile+ifile][pcs] = bb;

          // 左上から右下への利き。
          BBIni(bb);
          if ( ifile <= irank )
            {
              for ( i = -1; ifile+i >= 0; i-- )
                {
                  set_attacks( irank+i, ifile+i, &bb );
                  if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
                }
              for ( i = 1; irank+i <= 8; i++ )
                {
                  set_attacks( irank+i, ifile+i, &bb );
                  if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
                }
            }
          else {
            for ( i = -1; irank+i >= 0; i-- )
              {
                set_attacks( irank+i, ifile+i, &bb );
                if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
              }
            for ( i = 1; ifile+i <= 8; i++ )
              {
                set_attacks( irank+i, ifile+i, &bb );
                if ( (pcs<<1) & (1 << (8-ifile-i)) ) { break; }
              }
          }

          abb_bishop_attacks_rl45[irank*nfile+ifile][pcs] = bb; 

          // 右上から左下への利き。
          BBIni(bb);
          if ( ifile+irank >= 8 )
            {
              for ( i = -1; irank-i <= 8; i-- )
                {
                  set_attacks( irank-i, ifile+i, &bb );
                  if ( (pcs<<1) & (1 << (irank-i)) ) { break; }
                }
              for ( i = 1; ifile+i <= 8; i++ )
                {
                  set_attacks( irank-i, ifile+i, &bb );
                  if ( (pcs<<1) & (1 << (irank-i)) ) { break; }
                }
            }
          else {
            for ( i = -1; ifile+i >= 0; i-- )
              {
                set_attacks( irank-i, ifile+i, &bb );
                if ( (pcs<<1) & (1 << (irank-i)) ) { break; }
              }
            for ( i = 1; irank-i >= 0; i++ )
              {
                set_attacks( irank-i, ifile+i, &bb );
                if ( (pcs<<1) & (1 << (irank-i)) ) { break; }
              }
          }
          abb_bishop_attacks_rr45[irank*nfile+ifile][pcs] = bb; 
        }

      // 将棋盤に斜めのラインは9+8=17本あって、45度回転させた盤面が
      // 9*9に入りきらないので、それを詰め込むために共有しているラインがある。
      // そこで盤面の右下半分なのか左上半分なのかで意味を変えることにする。
      if ( irank >= ifile )
        {
          // 何番目のbitboardのunsigned intか(p[N]のN)。
          aslide[irank*nfile+ifile].irl45 = (unsigned char)((irank-ifile)/3);
          // そこを取り出すのに必要な右shift回数。
          aslide[irank*nfile+ifile].srl45
            = (unsigned char)((2-((irank-ifile)%3))*9+1);
        }
      else {
        aslide[irank*nfile+ifile].irl45 = (unsigned char)((9+irank-ifile)/3);
        aslide[irank*nfile+ifile].srl45
          = (unsigned char)((2-((9+irank-ifile)%3))*9+1);
      }
      
      if ( ifile+irank >= 8 )
        {
          aslide[irank*nfile+ifile].irr45 = (unsigned char)((irank+ifile-8)/3);
          aslide[irank*nfile+ifile].srr45
            = (unsigned char)((2-((irank+ifile-8)%3))*9+1);
        }
      else {
        aslide[irank*nfile+ifile].irr45 = (unsigned char)((irank+ifile+1)/3);
        aslide[irank*nfile+ifile].srr45
          = (unsigned char)((2-((irank+ifile+1)%3))*9+1);
      }
    }
}


static void
set_attacks( int irank, int ifile, bitboard_t *pbb )
{
  if ( irank >= rank1 && irank <= rank9 && ifile >= file1 && ifile <= file9 )
    {
      Xor( irank*nfile + ifile, *pbb );
    }
}


static bitboard_t
bb_set_mask( int sq )
{
  bitboard_t bb;
  
  BBIni(bb);
  if      ( sq > 53 ) { bb.p[2] = 1U << ( 80 - sq ); }
  else if ( sq > 26 ) { bb.p[1] = 1U << ( 53 - sq ); }
  else                { bb.p[0] = 1U << ( 26 - sq ); }
  
  return bb;
}


static void
ini_check_table( void )
{
  bitboard_t bb_check, bb;
  int iking, sq_chk;
  
  for ( iking = 0; iking < nsquare; iking++ ) {
    
    /* black gold */
    BBIni( b_chk_tbl[iking].gold );
    bb_check = abb_w_gold_attacks[iking];
    while ( BBTest(bb_check) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( b_chk_tbl[iking].gold, b_chk_tbl[iking].gold,
              abb_w_gold_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    BBOr( bb, abb_mask[iking], abb_w_gold_attacks[iking] );
    BBNotAnd( b_chk_tbl[iking].gold, b_chk_tbl[iking].gold, bb );
    
    /* black silver */
    BBIni( b_chk_tbl[iking].silver );
    bb_check = abb_w_silver_attacks[iking];
    while ( BBTest(bb_check) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( b_chk_tbl[iking].silver, b_chk_tbl[iking].silver,
              abb_w_silver_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    bb_check.p[0] = abb_w_gold_attacks[iking].p[0];
    while ( bb_check.p[0] )
      {
        sq_chk = last_one0( bb_check.p[0] );
        BBOr( b_chk_tbl[iking].silver, b_chk_tbl[iking].silver,
              abb_w_silver_attacks[sq_chk] );
        bb_check.p[0] ^= abb_mask[sq_chk].p[0];
      }
    bb_check.p[1] = abb_w_gold_attacks[iking].p[1];
    while ( bb_check.p[1] )
      {
        sq_chk = last_one1( bb_check.p[1] );
        b_chk_tbl[iking].silver.p[0] |= abb_w_silver_attacks[sq_chk].p[0];
        bb_check.p[1] ^= abb_mask[sq_chk].p[1];
      }
    BBOr( bb, abb_mask[iking], abb_w_silver_attacks[iking] );
    BBNotAnd( b_chk_tbl[iking].silver, b_chk_tbl[iking].silver, bb );
    
    /* black knight */
    BBIni( b_chk_tbl[iking].knight );
    bb_check = abb_w_knight_attacks[iking];
    while ( BBTest(bb_check) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( b_chk_tbl[iking].knight, b_chk_tbl[iking].knight,
              abb_w_knight_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    bb_check.p[0] = abb_w_gold_attacks[iking].p[0];
    while ( bb_check.p[0] )
      {
        sq_chk = last_one0( bb_check.p[0] );
        BBOr( b_chk_tbl[iking].knight, b_chk_tbl[iking].knight,
              abb_w_knight_attacks[sq_chk] );
        bb_check.p[0] ^= abb_mask[sq_chk].p[0];
      }
    
    /* black lance */
    if ( iking <= I3 ) {
      BBAnd( b_chk_tbl[iking].lance, abb_plus_rays[iking+nfile],
             abb_file_attacks[iking][0] );
      if ( iking <= I7 && iking != A9 && iking != A8 && iking != A7 ) {
        BBAnd( bb, abb_plus_rays[iking-1], abb_file_attacks[iking-1][0] );
        BBOr( b_chk_tbl[iking].lance,        b_chk_tbl[iking].lance, bb );
      }
      if ( iking <= I7 && iking != I9 && iking != I8 && iking != I7 ) {
        BBAnd( bb, abb_plus_rays[iking+1], abb_file_attacks[iking+1][0] );
        BBOr( b_chk_tbl[iking].lance, b_chk_tbl[iking].lance, bb );
      }
    } else { BBIni( b_chk_tbl[iking].lance ); }
    
    /* white gold */
    BBIni( w_chk_tbl[iking].gold );
    bb_check = abb_b_gold_attacks[iking];
    while ( BBTest(bb_check) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( w_chk_tbl[iking].gold, w_chk_tbl[iking].gold,
              abb_b_gold_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    BBOr( bb, abb_mask[iking], abb_b_gold_attacks[iking] );
    BBNotAnd( w_chk_tbl[iking].gold, w_chk_tbl[iking].gold, bb );
    
    /* white silver */
    BBIni( w_chk_tbl[iking].silver );
    bb_check = abb_b_silver_attacks[iking];
    while ( BBTest(bb_check) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( w_chk_tbl[iking].silver, w_chk_tbl[iking].silver,
              abb_b_silver_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    bb_check.p[2] = abb_b_gold_attacks[iking].p[2];
    while ( bb_check.p[2] )
      {
        sq_chk = first_one2( bb_check.p[2] );
        BBOr( w_chk_tbl[iking].silver, w_chk_tbl[iking].silver,
              abb_b_silver_attacks[sq_chk] );
        bb_check.p[2] ^= abb_mask[sq_chk].p[2];
      }
    bb_check.p[1] = abb_b_gold_attacks[iking].p[1];
    while ( bb_check.p[1] )
      {
        sq_chk = first_one1( bb_check.p[1] );
        w_chk_tbl[iking].silver.p[2]
          |= abb_b_silver_attacks[sq_chk].p[2];
        bb_check.p[1] ^= abb_mask[sq_chk].p[1];
      }
    BBOr( bb, abb_mask[iking], abb_b_silver_attacks[iking] );
    BBNotAnd( w_chk_tbl[iking].silver, w_chk_tbl[iking].silver, bb );
    
    /* white knight */
    BBIni( w_chk_tbl[iking].knight );
    bb_check = abb_b_knight_attacks[iking];
    while ( BBTest( bb_check ) )
      {
        sq_chk = LastOne( bb_check );
        BBOr( w_chk_tbl[iking].knight, w_chk_tbl[iking].knight,
              abb_b_knight_attacks[sq_chk] );
        Xor( sq_chk, bb_check );
      }
    bb_check.p[2] = abb_b_gold_attacks[iking].p[2];
    while ( bb_check.p[2] )
      {
        sq_chk = first_one2( bb_check.p[2] );
        BBOr( w_chk_tbl[iking].knight, w_chk_tbl[iking].knight,
              abb_b_knight_attacks[sq_chk] );
        bb_check.p[2] ^= abb_mask[sq_chk].p[2];
      }
    
    /* white lance */
    if ( iking >= A7 ) {
      BBAnd( w_chk_tbl[iking].lance, abb_minus_rays[iking-nfile],
             abb_file_attacks[iking][0] );
      if ( iking >= A3 && iking != A3 && iking != A2 && iking != A1 ) {
        BBAnd( bb, abb_minus_rays[iking-1], abb_file_attacks[iking-1][0] );
        BBOr( w_chk_tbl[iking].lance, w_chk_tbl[iking].lance, bb );
      }
      if ( iking >= A3 && iking != I3 && iking != I2 && iking != I1 ) {
        BBAnd( bb, abb_minus_rays[iking+1], abb_file_attacks[iking+1][0] );
        BBOr( w_chk_tbl[iking].lance, w_chk_tbl[iking].lance, bb );
      }
    } else { BBIni( w_chk_tbl[iking].lance ); }
  }
}


#if   defined(_MSC_VER)
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#else
static int
first_one00( int pcs )
{
  int i;
  
  for ( i = 0; i < 9; i++ ) { if ( pcs & (1<<(8-i)) ) { break; } }
  return i;
}


static int
last_one00( int pcs )
{
  int i;
  
  for ( i = 8; i >= 0; i-- ) { if ( pcs & (1<<(8-i)) ) { break; } }
  return i;
}
#endif
