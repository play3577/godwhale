
// compile w/ : g++ bbatk.cpp -c -msse2

//#define DBG_BBATK

 // WARNING! must match w/ shogi.h
#define USE_BBSUFMASK 0

#include <stdio.h>
 // for memset
#include <string.h>

#ifdef HAVE_AVX
#include <immintrin.h>
#else
#include <emmintrin.h>
#endif

typedef union {
  unsigned int p[4];
  __m128i m;
} bitboard_t;

#define areaclr(x) memset(&x, 0, sizeof(x))
//typedef long long int int64_t;
//typedef unsigned long long int uint64_t;
#include <inttypes.h>

extern bitboard_t abb_mask[81];

#ifdef DBG_BBATK
bitboard_t abb_mask[81];
#endif

 //******** external def/data/func 

#ifdef HAVE_AVX
typedef union {
 __m256 m;
#else
#ifdef HAVE_SSE2
typedef union {
 __m128i m[2];
#else
typedef struct {
#endif
#endif
 uint64_t x[4];  // rank, file, diag1(rl45), diag2(rr45)
} occupiedC;

bitboard_t abb_attacks[4][128/*81*/][128];
void initNewBB();
extern int ai_shift[4][81];  // value defined below
extern int ai_sufmask[4][81];  // ditto
occupiedC ao_bitmask[81];

#define AttackRankE(occ,sq) abb_attacks[0][sq][ \
          (occ.x[0] >>ai_shift[0][sq]) & 0x7f]
#define AttackFileE(occ,sq) abb_attacks[1][sq][ \
          (occ.x[1] >>ai_shift[1][sq]) & 0x7f]
#define AttackDiag1E(occ,sq) abb_attacks[2][sq][ \
   (occ.x[2] >> ai_shift[2][sq]) & (!USE_BBSUFMASK ? 0x7f : ai_sufmask[2][sq])]
#define AttackDiag2E(occ,sq) abb_attacks[3][sq][ \
   (occ.x[3] >> ai_shift[3][sq]) & (!USE_BBSUFMASK ? 0x7f : ai_sufmask[3][sq])]

 //******** external def/data/func end

#define Xor(sq,bb) \
  do { int i; for(i=0;i<=2;i++) bb.p[i] ^= abb_mask[sq].p[i]; } while(0)

 enum { A9 = 0, B9, C9, D9, E9, F9, G9, H9, I9,
           A8, B8, C8, D8, E8, F8, G8, H8, I8,
           A7, B7, C7, D7, E7, F7, G7, H7, I7,
           A6, B6, C6, D6, E6, F6, G6, H6, I6,
           A5, B5, C5, D5, E5, F5, G5, H5, I5,
           A4, B4, C4, D4, E4, F4, G4, H4, I4,
           A3, B3, C3, D3, E3, F3, G3, H3, I3,
           A2, B2, C2, D2, E2, F2, G2, H2, I2,
           A1, B1, C1, D1, E1, F1, G1, H1, I1 };

static bitboard_t
bb_set_mask( int sq )
{
  bitboard_t bb;
  
  //BBIni(bb);
  bb.p[2] = bb.p[1] = bb.p[0] = 0;
  if      ( sq > 53 ) { bb.p[2] = 1U << ( 80 - sq ); }
  else if ( sq > 26 ) { bb.p[1] = 1U << ( 53 - sq ); }
  else                { bb.p[0] = 1U << ( 26 - sq ); }
  
  return bb;
}


#define foro(i,m,n) for(i=(m); i<=(n); i++)

#define MAXLEN 17

enum { OD_HORIZ=0, OD_VERT=1, OD_DIAG1=2, OD_DIAG2=3 };

static int ai_bitpos[4][81] = {
  // HORIZ
 {-1,  0,  1,  2,  3,  4,  5,  6, -1,
  -1,  7,  8,  9, 10, 11, 12, 13, -1,
  -1, 14, 15, 16, 17, 18, 19, 20, -1,
  -1, 21, 22, 23, 24, 25, 26, 27, -1,
  -1, 28, 29, 30, 31, 32, 33, 34, -1,
  -1, 35, 36, 37, 38, 39, 40, 41, -1,
  -1, 42, 43, 44, 45, 46, 47, 48, -1,
  -1, 49, 50, 51, 52, 53, 54, 55, -1,
  -1, 56, 57, 58, 59, 60, 61, 62, -1 },

  // VERT
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   1,  8, 15, 22, 29, 36, 43, 50, 57,
   2,  9, 16, 23, 30, 37, 44, 51, 58,
   3, 10, 17, 24, 31, 38, 45, 52, 59,
   4, 11, 18, 25, 32, 39, 46, 53, 60,
   5, 12, 19, 26, 33, 40, 47, 54, 61,
   6, 13, 20, 27, 34, 41, 48, 55, 62,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 },

  // DIAG1
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1,  0,  2,  5,  9, 14, 20, 27, -1,
  -1,  1,  4,  8, 13, 19, 26, 33, -1,
  -1,  3,  7, 12, 18, 25, 32, 38, -1,
  -1,  6, 11, 17, 24, 31, 37, 42, -1,
  -1, 10, 16, 23, 30, 36, 41, 45, -1,
  -1, 15, 22, 29, 35, 40, 44, 47, -1,
  -1, 21, 28, 34, 39, 43, 46, 48, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 },

  // DIAG2
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 21, 15, 10,  6,  3,  1,  0, -1,
  -1, 28, 22, 16, 11,  7,  4,  2, -1,
  -1, 34, 29, 23, 17, 12,  8,  5, -1,
  -1, 39, 35, 30, 24, 18, 13,  9, -1,
  -1, 43, 40, 36, 31, 25, 19, 14, -1,
  -1, 46, 44, 41, 37, 32, 26, 20, -1,
  -1, 48, 47, 45, 42, 38, 33, 27, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};

static void iniAtkData(); // defined below

void initNewBB() {
 int d, sq;
 areaclr(ao_bitmask);

 foro(d,0,3)
 foro(sq,0,80)
   if (ai_bitpos[d][sq] >= 0)
     ao_bitmask[sq].x[d] = 1UL << ai_bitpos[d][sq];

 iniAtkData();

}

int ai_shift[4][81] = {
  // HORIZ
 { 0,  0,  0,  0,  0,  0,  0,  0,  0,
   7,  7,  7,  7,  7,  7,  7,  7,  7,
  14, 14, 14, 14, 14, 14, 14, 14, 14,
  21, 21, 21, 21, 21, 21, 21, 21, 21,
  28, 28, 28, 28, 28, 28, 28, 28, 28,
  35, 35, 35, 35, 35, 35, 35, 35, 35,
  42, 42, 42, 42, 42, 42, 42, 42, 42,
  49, 49, 49, 49, 49, 49, 49, 49, 49,
  56, 56, 56, 56, 56, 56, 56, 56, 56 },

  // VERT 
 { 0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56 },

  // DIAG1
 { 0,  0,  0,  1,  3,  6, 10, 15, 21,
   0,  0,  1,  3,  6, 10, 15, 21, 28,
   0,  1,  3,  6, 10, 15, 21, 28, 34,
   1,  3,  6, 10, 15, 21, 28, 34, 39,
   3,  6, 10, 15, 21, 28, 34, 39, 43,
   6, 10, 15, 21, 28, 34, 39, 43, 46,
  10, 15, 21, 28, 34, 39, 43, 46, 48,
  15, 21, 28, 34, 39, 43, 46, 48,  0,
  21, 28, 34, 39, 43, 46, 48,  0,  0 },

  // DIAG2
 {21, 15, 10,  6,  3,  1,  0,  0,  0,
  28, 21, 15, 10,  6,  3,  1,  0,  0,
  34, 28, 21, 15, 10,  6,  3,  1,  0,
  39, 34, 28, 21, 15, 10,  6,  3,  1,
  43, 39, 34, 28, 21, 15, 10,  6,  3,
  46, 43, 39, 34, 28, 21, 15, 10,  6,
  48, 46, 43, 39, 34, 28, 21, 15, 10,
   0, 48, 46, 43, 39, 34, 28, 21, 15,
   0,  0, 48, 46, 43, 39, 34, 28, 21 },
};

int ai_sufmask[4][81] = {
  // HORIZ
 { },
  // VERT 
 { },

  // DIAG1
 { 0,  0,  1,  3,  7, 15, 31, 63,127,
   0,  1,  3,  7, 15, 31, 63,127, 63,
   1,  3,  7, 15, 31, 63,127, 63, 31,
   3,  7, 15, 31, 63,127, 63, 31, 15,
   7, 15, 31, 63,127, 63, 31, 15,  7,
  15, 31, 63,127, 63, 31, 15,  7,  3,
  31, 63,127, 63, 31, 15,  7,  3,  1,
  63,127, 63, 31, 15,  7,  3,  1,  0,
 127, 63, 31, 15,  7,  3,  1,  0,  0 },

  // DIAG2
 {127, 63, 31, 15,  7,  3,  1,  0,  0,
   63,127, 63, 31, 15,  7,  3,  1,  0,
   31, 63,127, 63, 31, 15,  7,  3,  1,
   15, 31, 63,127, 63, 31, 15,  7,  3,
    7, 15, 31, 63,127, 63, 31, 15,  7,
    3,  7, 15, 31, 63,127, 63, 31, 15,
    1,  3,  7, 15, 31, 63,127, 63, 31,
    0,  1,  3,  7, 15, 31, 63,127, 63,
    0,  0,  1,  3,  7, 15, 31, 63,127 }
};

typedef struct {
 int listlen;    // 0/90 : 9   45 : 17
 int width[MAXLEN];  // 0/90 : all 9   45 : 1,2,..,8,9,8,..,1
 int locs[MAXLEN][9];  // squares on the line
} iniAtkDataC ;

iniAtkDataC adata[4];

static int locsR[MAXLEN][9] = {
  { A9, B9, C9, D9, E9, F9, G9, H9, I9 },
  { A8, B8, C8, D8, E8, F8, G8, H8, I8 },
  { A7, B7, C7, D7, E7, F7, G7, H7, I7 },
  { A6, B6, C6, D6, E6, F6, G6, H6, I6 },
  { A5, B5, C5, D5, E5, F5, G5, H5, I5 },
  { A4, B4, C4, D4, E4, F4, G4, H4, I4 },
  { A3, B3, C3, D3, E3, F3, G3, H3, I3 },
  { A2, B2, C2, D2, E2, F2, G2, H2, I2 },
  { A1, B1, C1, D1, E1, F1, G1, H1, I1 } };

static int locsF[MAXLEN][9] = {
  { A9, A8, A7, A6, A5, A4, A3, A2, A1 },
  { B9, B8, B7, B6, B5, B4, B3, B2, B1 },
  { C9, C8, C7, C6, C5, C4, C3, C2, C1 },
  { D9, D8, D7, D6, D5, D4, D3, D2, D1 },
  { E9, E8, E7, E6, E5, E4, E3, E2, E1 },
  { F9, F8, F7, F6, F5, F4, F3, F2, F1 },
  { G9, G8, G7, G6, G5, G4, G3, G2, G1 },
  { H9, H8, H7, H6, H5, H4, H3, H2, H1 },
  { I9, I8, I7, I6, I5, I4, I3, I2, I1 } };

static int locsD1[MAXLEN][9] = {
  { A9 },
  { A8, B9 },
  { A7, B8, C9 },
  { A6, B7, C8, D9 },
  { A5, B6, C7, D8, E9 },
  { A4, B5, C6, D7, E8, F9 },
  { A3, B4, C5, D6, E7, F8, G9 },
  { A2, B3, C4, D5, E6, F7, G8, H9 },
  { A1, B2, C3, D4, E5, F6, G7, H8, I9 },
  { B1, C2, D3, E4, F5, G6, H7, I8 },
  { C1, D2, E3, F4, G5, H6, I7 },
  { D1, E2, F3, G4, H5, I6 },
  { E1, F2, G3, H4, I5 },
  { F1, G2, H3, I4 },
  { G1, H2, I3 },
  { H1, I2 },
  { I1 } };

static int locsD2[MAXLEN][9] = {
  { A1 },
  { A2, B1 },
  { A3, B2, C1 },
  { A4, B3, C2, D1 },
  { A5, B4, C3, D2, E1 },
  { A6, B5, C4, D3, E2, F1 },
  { A7, B6, C5, D4, E3, F2, G1 },
  { A8, B7, C6, D5, E4, F3, G2, H1 },
  { A9, B8, C7, D6, E5, F4, G3, H2, I1 },
  { B9, C8, D7, E6, F5, G4, H3, I2 },
  { C9, D8, E7, F6, G5, H4, I3 },
  { D9, E8, F7, G6, H5, I4 },
  { E9, F8, G7, H6, I5 },
  { F9, G8, H7, I6 },
  { G9, H8, I7 },
  { H9, I8 },
  { I9 } };


static iniAtkDataC f, r, d1,d2;

static void iniAtkData() {
 int i,j,k,direc, line,sq,sqsuf,vec,pcs;
 memset(&r, 0, sizeof(f));
 memset(&f, 0, sizeof(f));
 memset(&d1, 0, sizeof(f));
 memset(&d2, 0, sizeof(f));
 r.listlen =
 f.listlen = 9;
 d1.listlen =
 d2.listlen = 17;

 foro(k, 0, 8) {
   r.width[k] =
   f.width[k] = 9;
   d1.width[k] =
   d2.width[k] = k+1;
 }
 foro(k, 9, 16) {
   d1.width[k] =
   d2.width[k] = 17-k;
 }

 foro(i, 0, 16) {
  foro(j, 0, 8) {
    r.locs[i][j] = locsR[i][j];
    f.locs[i][j] = locsF[i][j];
    d1.locs[i][j] = locsD1[i][j];
    d2.locs[i][j] = locsD2[i][j];
  }
 }

 adata[0] = r;
 adata[1] = f;
 adata[2] = d1;
 adata[3] = d2;

 areaclr(abb_attacks);

 foro(direc, OD_HORIZ, OD_DIAG2) {
   foro(line, 0, adata[direc].listlen-1) {
     int wid = adata[direc].width[line];
     foro(sqsuf, 0, wid-1) {
       int sq = adata[direc].locs[line][sqsuf];

        //**** for each PCS bit patterns
       foro(pcs, 0, 127) {
         bitboard_t bb;

          //**** need only up to sufmask, in case of diag
         if (USE_BBSUFMASK && (direc==OD_DIAG1 || direc==OD_DIAG2) &&
             pcs > ai_sufmask[direc][sq])
           break;
         bb.p[3] = bb.p[2] = bb.p[1] = bb.p[0] = 0;

          //**** go both +/- dir's from sq
         for(vec=-1; vec<=1; vec+=2) { //vec=1/-1
           int tgtsuf = sqsuf + vec;

            //**** go until either edge or blocked 
           while(0<=tgtsuf && tgtsuf<wid) {
             int tgtsq = adata[direc].locs[line][tgtsuf];
             Xor(tgtsq,bb);
             if(tgtsuf > 0 && ((1 << (tgtsuf-1)) & pcs) != 0)
               break;
             tgtsuf += vec;
           } // while tgtsuf

         } // for vec

         abb_attacks[direc][sq][pcs] = bb;
       } // for pcs
     } // for sqsuf
   } // for line
 } // for direc

}

#ifdef DBG_BBATK

occupiedC occ;

void setOcc(int* a) {
 int i;
 areaclr(occ);
 foro(i, 0, 80)
   if (a[i]) {
     occ.x[0] |= ao_bitmask[i].x[0];
     occ.x[1] |= ao_bitmask[i].x[1];
     occ.x[2] |= ao_bitmask[i].x[2];
     occ.x[3] |= ao_bitmask[i].x[3];
   }
}

int bitisset(bitboard_t bb, int sq) {
 int i;
 bitboard_t bb2 = bb_set_mask(sq);
 foro(i,0,2)
   if (bb.p[i] & bb2.p[i])
     return 1;
 return 0;
}

void dispbb(bitboard_t bb) {
 int i;
 foro(i, 0, 80) {
   printf("%d", bitisset(bb,i));
   if (i%9 == 8) putchar('\n');
 }
}

int blocked[81] = {
 1,0,0,1,0,0,1,0,0,
 1,0,0,0,0,0,1,0,0,
 0,0,0,1,0,0,0,0,0,
 0,1,0,1,0,0,1,0,0,
 1,1,0,0,0,0,0,0,0,
 0,0,1,0,0,0,1,0,0,
 0,0,0,0,0,0,1,0,0,
 1,0,0,1,0,0,0,0,0,
 0,0,0,0,0,0,1,0,1
};

int main() {
 bitboard_t bb;
 int i, q;
 int sqa[6] = {C8, A4, E2, H9, F3, G6};
 foro(q,0,80)
   abb_mask[q] = bb_set_mask(q);
 initNewBB();    // initilize tables
 setOcc(blocked); // set occupied data
 foro(i,0,5) {
   int sq = sqa[i];
   printf("---- (%d,%d)\n", 9-sq%9, sq/9+1);
   bb = AttackRankE(occ,sq);
   printf("rank:\n");
   dispbb(bb);
   bb = AttackFileE(occ,sq);
   printf("file:\n");
   dispbb(bb);
   bb = AttackDiag1E(occ,sq);
   printf("diag1:\n");
   dispbb(bb);
   bb = AttackDiag2E(occ,sq);
   printf("diag2:\n");
   dispbb(bb);
 }
}

#endif
