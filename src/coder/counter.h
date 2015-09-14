#ifndef COUNTER_H
#define COUNTER_H

class Prob16Counter
{
  public:
    uint16_t p1;
    Prob16Counter():p1(PSCALEh){};
  protected:
    int idiv(int val,int s) {return (val+(1<<(s-1)))>>s;};
    int idiv_signed(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
};

// Linear Counter, p=16 bit
class LinearCounter16 : public Prob16Counter
{
  public:
    using Prob16Counter::Prob16Counter;
    //p'=(1-w0)*p+w0*((1-w1)*bit+w1*0.5)
    #define wh(w) ((w*PSCALEh+PSCALEh)>>PBITS)
    void update(int bit,const int w0,const int w1)
    {
      int h=(w0*wh(w1))>>PBITS;
      int p=idiv((PSCALE-w0)*p1,PBITS);
      p+=bit?w0-h:h;
      p1=clamp(p,1,PSCALEm);
    };
    //p'+=L*(bit-p)
    void update(int bit,int L)
    {
      int err=(bit<<PBITS)-p1;
      // p1 should be converted to "int" implicit anyway?
      int px = int(p1) + idiv_signed(L*err,PBITS);
      p1=clamp(px,1,PSCALEm);
    }
};

#endif // COUNTER_H
