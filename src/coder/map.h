#ifndef MAP_H
#define MAP_H

#include "counter.h"

/*
 SSE: functions of context history
 maps a probability via (linear-)quantization to a new probability
*/
template <int NB>
class SSE {
  uint16_t p_quant,px;
  public:
    enum {mapsize=1<<NB};
    SSE ()
    {
      for (int i=0;i<=mapsize;i++) // init prob-map that SSE.p1(p)~p
      {
        int v=((i*PSCALE)>>NB);
        v = clamp(v,1,PSCALEm);
        Map[i].p1=v;
      }
    }
    int p1(int p1) // linear interpolate beetween bins
    {
      p_quant=p1>>(PBITS-NB);
      int p_mod=p1&(mapsize-1); //int p_mod=p1%map_size;
      int pl=Map[p_quant].p1;
      int ph=Map[p_quant+1].p1;
      px=(pl+((p_mod*(ph-pl))>>NB));
      return px;
    }
    void update2(int bit,int rate) // update both bins
    {
      Map[p_quant].update(bit,rate);
      Map[p_quant+1].update(bit,rate);
    }
    void update4(int bit,int rate) // update four nearest bins
    {
      if (p_quant>0) Map[p_quant-1].update(bit,rate>>1);
      Map[p_quant].update(bit,rate);
      Map[p_quant+1].update(bit,rate);
      if (p_quant<mapsize-1) Map[p_quant+2].update(bit,rate>>1);
    }
    void update1(int bit,int rate) // update artifical bin
    {
      LinearCounter16 tmp;
      tmp.p1=px;
      tmp.update(bit,rate);
      int pm=tmp.p1-px;
      int pt1=Map[p_quant].p1+pm;
      int pt2=Map[p_quant+1].p1+pm;
      Map[p_quant].p1=clamp(pt1,1,PSCALEm);
      Map[p_quant+1].p1=clamp(pt2,1,PSCALEm);
    }
  protected:
    LinearCounter16 Map[(1<<NB)+1];
};

// Maps a state to a probability
class HistProbMapping
{
  enum {NUMSTATES=256};
  public:
    HistProbMapping()
    {
      for (int i=0;i<NUMSTATES;i++) Map[i].p1=StateProb::GetP1(i);
    };
    inline int p1(uint8_t state)
    {
       return Map[state].p1;
    }
    void Update(int state,int bit,int rate)
    {
       Map[state].update(bit,rate);
    }
  protected:
    LinearCounter16 Map[NUMSTATES];
};

#endif // MAP_H
