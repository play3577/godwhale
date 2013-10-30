﻿/* $Id: splane.h,v 1.72 2012-04-19 22:25:07 eiki Exp $ */

//******** FIXME duplicated? ********

 // copied from shogi.h
#define score_bound 32600
#define score_max_eval 32500

extern cmdPacketC cmd2send;

//******** jobDtorC ********
enum
{
    RUNNING_NONE   = 0,
    RUNNING_SINGLE = 99
};

/**
 * @brief ジョブの仕分けを行います。
 */
struct jobDtorC
{
    int itd, exd, mvsuf;

    jobDtorC() {}
    void clear() { exd = mvsuf = 0; itd = RUNNING_NONE; }
    bool idle() { return (itd == RUNNING_NONE); }

    bool onSingle() { return (itd == RUNNING_SINGLE); }
    bool onList()  { return (RUNNING_NONE < itd && itd < RUNNING_SINGLE); }

    void setIdle() { itd = RUNNING_NONE; }
    void setSingle() { itd = RUNNING_SINGLE; }
    void setList(int i, int e, int s) { itd = i; exd = e; mvsuf = s; }
};

//******** smvEntryC ********

#define MAXTRYCNT 32

class smvEntryC
{
public:
    mvC mv;
    mvC bestmv;
    int depth;
    uint64_t numnode;
    short upper, lower;
    int perfTrycnt;

    /*smvEntryC() {}*/
    void clear()
    {
        memset(this, 0, sizeof(smvEntryC));
    }

    void perfIncTrycnt()
    {
        if (perfTrycnt < MAXTRYCNT-1) perfTrycnt++;
    }

    bool done(int dep, int A, int B)
    {
        return (depth >= dep && (upper==lower || -lower <= A || B <= -upper));
        // 11/29/2011 %16 was   upper              lower
    }

    void pushNewmv(mvC m, mvC bm)
    {
        clear();
        mv = m;
        bestmv = bm;
    }

    void update(int dep, int val, int A, int B, uint64_t numnod, mvC bestmove);
};

void smvEntryC::update(int dep, int val, int A, int B,
                       uint64_t numnod, mvC bestmove)
{
    if (A < val && val < B) {
        upper = lower = val;
    }
    else if (val <= A) {
        if (depth < dep) {
            upper = A;
            lower = - score_bound;
        }
        else {
            upper = A;
            //lower = min(lower, upper-1);
        }
    }
    else { // val >= B
        if (depth < dep) {
            lower = val;
            upper = score_bound;
        }
        else {
            lower = val;
            //upper = max(upper, lower+1); FIXME? need something like min(bound,..)?
        }
    }

    if (val > A) bestmv = bestmove;
    numnode = numnod;
    depth   = dep;
}

//******** sprocmvsC ********

class sprocmvsC
{
public:
    int mvcnt;
    smvEntryC mvs[GMX_MAX_LEGAL_MVS];

    int findsuf(mvC mv)
    {
        forr (i, 0, mvcnt-1) {
            if (mvs[i].mv == mv) {
                return i;
            }
        }

        return -1;
    }
    
    int nextsuf(int dep, int A, int B)
    {
        forr (i, 0, mvcnt-1) {
            if (!mvs[i].done(dep, A, B)) {
                return i;
            }
        }

        return -1;
    }
    
    int done(int dep, int A, int B)
    {
        return (nextsuf(dep, A, B) == -1);
    }
};

//******** srowC ********

class srowC
{
public:
    char updated;
    mvC registKillerMv;
    int alpha, beta, gamma;
    mvC firstmv;
    int bestval, bestseqLeng, bestule;
    mvC bestseq[GMX_MAX_BESTSEQ];
    sprocmvsC procmvs;

    void clear()
    {
        memset(this, 0, sizeof(srowC));
        invalidate();
    }

    void invalidate()
    {
        procmvs.mvcnt = -1;
    }

    void shrink(int mvcnt)
    {
        procmvs.mvcnt = mvcnt;
    }

    void extend(int mvcnt, mvtupleC* tuples)
    {
        int curcnt = procmvs.mvcnt;

        dumpExtend(mvcnt, curcnt, tuples);
        forr (i, 0, mvcnt-1) {
            procmvs.mvs[curcnt+i].mv     = tuples[i].mv;
            procmvs.mvs[curcnt+i].bestmv = tuples[i].bestmv;
            procmvs.mvs[curcnt+i].depth  = tuples[i].depth;
            procmvs.mvs[curcnt+i].upper  = tuples[i].upper;
            procmvs.mvs[curcnt+i].lower  = tuples[i].lower;

            // FIXME? are these ok?
            procmvs.mvs[curcnt+i].numnode = 0;
        }
        procmvs.mvcnt += mvcnt;
    }
    
    void dumpExtend(int mvcnt, int curcnt, mvtupleC* tuples)
    {
        if (!DBG_DUMP_COMM) return;
        SLTOut("^^^^ extend called  itd %d exd %d mvcnt %d(cur %d)  mvs:\n",
               itd(), exd(),  mvcnt, curcnt);

        forr (i, 0, mvcnt-1) {
            SLTOut(" %07x-%07x %d %d %d",
                   readable(tuples[i].mv),
                   readable(tuples[i].bestmv),
                   tuples[i].depth,
                   tuples[i].upper,
                   tuples[i].lower);
            if (i % 2 == 1) SLTOut("\n");
        }

        SLTOut("\n");
    }

    void setlist(int A, int B, mvC firstmv, int mvcnt, mvtupleC* tuples);
    void updateValue(int val, ValueType typ);

    int itd();
    int exd();
};

void srowC::updateValue(int val, ValueType type) {
    if (type == VALTYPE_BETA) {
        assert(val < beta);
        beta = val;
    }
    else if (type == VALTYPE_GAMMA) {
        gamma = val;
    }
    else {
        assert(type == VALTYPE_ALPHA);
        if (val > alpha) {
            alpha = val;
        }
    }
}

void srowC::setlist(int A, int B, mvC first_mv, int mvcnt, mvtupleC* tuples)
{
    (void)B;

    alpha   = -score_bound;
    gamma   = A;    // 11/30/2011 %18 was setting A, not gamma
    beta    = score_bound;    // FIXME B not used?
    firstmv = registKillerMv = first_mv;
    updated = 0;

    forr (i, 0, mvcnt-1) {
        procmvs.mvs[i].mv      = tuples[i].mv;
        procmvs.mvs[i].bestmv  = tuples[i].bestmv;
        procmvs.mvs[i].depth   = tuples[i].depth;
        procmvs.mvs[i].upper   = tuples[i].upper;
        procmvs.mvs[i].lower   = tuples[i].lower;
        procmvs.mvs[i].numnode = 0;   // FIXME? right?
        procmvs.mvs[i].perfTrycnt = 0;
    }
    procmvs.mvcnt = mvcnt;
}

//******** sstreamC ********

class sstreamC
{
public:
    char tailExd;
    srowC row[MAX_EXPDEP];
    int seqrootLeng;
    mvC seqFromRoot[MAX_SEQPREV];

    int itd();  // defined after decl of plane
    void clear()
    {
        memset(this, 0, sizeof(sstreamC));
        invalidate();
    }

    void invalidate()
    {
        tailExd = -1;
        forr (d, 0, MAX_EXPDEP-1) {
            row[d].invalidate();
        }
    }
    int propagateUp(int exd, int val, int runexd);
    int propagateDown(int exd, int val, ValueType valtyp, int runexd);

    // notify must have been done, no change in alpha/gamma
    void commit(int exd)
    { 
        tailExd = exd - 1;
        if (exd > 0) {
            // in betacut case, this must be F
            row[exd-1].alpha = max(row[exd-1].alpha, -row[exd].alpha);
            row[exd-1].gamma = -score_bound;
        }
        // NOTE for slave, no need to copy bestseq
    }

    // runexd: exd of current running job, which may need interrupt
    int notify(int exd, int val, mvC notifymv, int runexd)
    {
        if (row[exd].firstmv == notifymv) {    // Fcomp case
            return fixgamma(exd, val, runexd);
        }

        if (row[exd].registKillerMv == NULLMV &&  // simply overwriting may work also?
            !(UToCap(notifymv.v) ||
              (I2IsPromote(notifymv.v) &&
               I2PieceMove(notifymv.v) != silver))) {
            row[exd].registKillerMv = notifymv;
        }
        
        SLTOut("---- notify: i %d e %d v %d mv %07x rune %d A %d\n",
               itd(), exd, val, readable(notifymv), runexd, row[exd].alpha);
        
        // Pvs case
        if (row[exd].alpha > val) {
            return 0; // another mv has already proved better, ignore this mv
        }

        // 3/29/2012 %xx was '>=' : on pvs update by this proc,
        //  we expect notify from master. A=val case must proceed
        int abt = 0;
        row[exd].updateValue(val, VALTYPE_ALPHA);
        if (exd > 0) {
            abt = propagateUp(exd-1, -val, runexd);
        }
        if (exd < tailExd) {
            abt += propagateDown(exd+1, -val, VALTYPE_BETA, runexd);
        }
        return abt; // return >0 if interrupting (need runexd)
    }
    
    int fixgamma(int exd, int val, int runexd)
    {
        int oldA = row[exd].alpha;
        int oldG = row[exd].gamma;
        SLTOut("---- fixgam: i %d e %d v %d mv %07x rune %d olA %d olG %d\n",
               itd(), exd, val, readable(row[exd].firstmv), runexd, oldA, oldG);
        
        row[exd].gamma = -score_bound; // 12/3/2011 %34 was missing
        if (oldA < val) {
            row[exd].alpha = val;
        }
        if (oldG == val) {
            return 0;
        }
        
        int abt = 0;
        if (exd > 0 && oldG != val && max(oldG,val) > oldA) {
            abt = propagateUp(exd-1, -val, runexd);
        }

        return abt; // return >0 if interrupting (need runexd)
    }

    void setseq(int pvleng, mvC* pv)
    {
        forr (i, 0, pvleng-1) {
            seqFromRoot[i] = pv[i];
        }

        seqrootLeng = pvleng;
    }
};

  //****
int sstreamC::propagateUp(int exd, int val, int runexd)
{
    int abt = 0;

    (void)runexd;

    forv (d, exd, 0) {
        srowC& r = row[d];
        if (val == r.gamma) {
            break; // FIXME need this break?  or go upto d=0?
        }

        int oldG = r.gamma;
        r.updateValue(val, VALTYPE_GAMMA);

        SLTOut("<<<< propUp i %d e %d oldG %d newG %d abt %d\n",
               itd(), d, oldG, val, abt);
        val = -max(r.alpha, val);
    }

    return abt;
}

  //****
int sstreamC::propagateDown(int exd, int val, ValueType valtyp, int runexd)
{
    int abt = 0;
    ValueType typ = valtyp;
    assert(typ == VALTYPE_ALPHA || typ == VALTYPE_BETA);

    forr (d, exd, tailExd) {
        bool isA = (typ == VALTYPE_ALPHA);
        srowC& r = row[d];
        int oldA = r.alpha;
        int oldB = r.beta;

        if (isA && val <= r.alpha) {
            break;   // FIXME need this break?  or go downto d=tail?
        }

        if (d == runexd &&
            (( isA && r.beta  <= val) ||   // A goes up, exceeds B
             (!isA && r.alpha >= val) )) { // B goes down, expect bcut
            abt = 1;
        }
        r.updateValue(val, typ);

        SLTOut("<<<< propDn i %d e %d isB %d old %d new %d abt %d\n",
               itd(), d, typ, (isA ? oldA : oldB), val, abt);

        typ = (!isA ? VALTYPE_ALPHA : VALTYPE_BETA);
        val = -val;
    }

    return abt;
}

//******** splaneC ********

class splaneC
{
public:
    sstreamC stream[MAX_ITD];

    void clear()
    {
        memset(this, 0, sizeof(splaneC));
        invalidateAll();
    }

    void invalidateAll()
    {
        forr (i, 0, MAX_ITD-1) {
            stream[i].invalidate();   // will not issue anything
        }
    }

    jobDtorC nextjob()
    {
        jobDtorC jd;
        int s;

        forr (i, 1, MAX_ITD-1) {
            if (stream[i].tailExd < 0) continue;

            forv (e, stream[i].tailExd, 0) {
                srowC& sr = stream[i].row[e];
                int srd = itdexd2srd(i, e);
                int mxA = max(sr.alpha, sr.gamma);
                int A = effalpha(sr.alpha, sr.gamma); // 11/30/2011 %19 was just A
                int B = sr.beta;

                if (mxA < B && (s=sr.procmvs.nextsuf(srd, A, B)) != -1) {
                    jd.itd = i;
                    jd.exd = e;
                    jd.mvsuf = s;
                    return jd;
                }
            }
        }

        jd.itd = RUNNING_NONE;
        return jd;
    }

    int cmdNotify(int itd, int exd, int val, mvC notifymv, int runexd)
    {
        return stream[itd].notify(exd, val, notifymv, runexd);
    }

    void cmdCommit(int itd, int exd)
    {
        stream[itd].commit(exd);
    }

    void cmdShrink(int itd, int exd, int mvcnt)
    {
        stream[itd].row[exd].shrink(mvcnt);
    }

    void cmdExtend(int itd, int exd, int mvcnt, mvtupleC* tuples)
    {
        stream[itd].row[exd].extend(mvcnt, tuples);
    }

    void cmdList(int itd, int exd, int A, int B, mvC firstmv, int pvleng,
                 mvC* pv, int mvcnt, mvtupleC* tuples)
    {
        stream[itd].row[exd].setlist(A, B, firstmv, mvcnt, tuples);
        stream[itd].setseq(pvleng, pv);

        if (stream[itd].tailExd < exd) {
            stream[itd].tailExd = exd;
        }
    }
};

 //********

extern splaneC splane;

int sstreamC::itd()
{
    return (int)(this - &splane.stream[0]);
}

int srowC::itd() {
    char *here = (char *)this;
    int it = -1;

    assert(here >= (char *)&splane.stream[0]);

    // 12/9/2011 %42 was using MAX_EXPDEP
    forr (i, 1, MAX_ITD) {
        if (here < (char *)&splane.stream[i]) {
            it = i - 1;
            break;
        }
    }

    assert(it != -1);
    return it;
}

int srowC::exd()
{
    return (int)(this - &splane.stream[itd()].row[0]);
}
