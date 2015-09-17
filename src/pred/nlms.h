#ifndef NLMS_H
#define NLMS_H

class NLMS : public MonoPredictor {
  public:
      NLMS(double mu,int order):w(order),hist(order),mu(mu),order(order) {
        pred=0;
        spow=0.0;
        imin=-(1<<15);imax=(1<<15);
        //imin=imax=0;
      }
      int32_t Predict()
      {
        double sum=0.; // caluclate prediction
        for (int i=0;i<order;i++) sum+=w[i]*hist[i];
        pred=floor(sum+0.5);

        //if(pred<imin)pred=imin;
        //else if (pred>imax)pred=imax;

        return pred;
      }
      void Update(int32_t val) {
        if (val<imin) imin=val;
        else if (val>imax) imax=val;

        double err=val-pred; // calculate prediction error

        spow-=(hist[order-1]*hist[order-1]); // update energy in the tapping window
        spow+=(val*val);
        //spow=0.992*spow+(val*val);

        const double wf=mu*err/(spow+0.001);
        for (int i=0;i<order;i++) w[i]+=wf*hist[i]; // update weight vector

        for (int i=order-1;i>0;i--) hist[i]=hist[i-1];
        hist[0]=val;
      }
      ~NLMS(){};
  private:
    Vector w,hist;
    double spow,mu;
    int pred,order;
    int imin,imax;
};

// NLMS: updates energy after updating weighting coefs
//#define POSTUPDATE

class NLMS2 : public StereoPredictor {
  public:
      NLMS2(double mu,int S,int D):w(S+D),hist(S+D),mu(mu),S(S),D(D) {
        pred=0;
        err=spow=0.0;
        imin=-(1<<15);imax=(1<<15);
        //imin=imax=0;
      }
      int32_t Predict(int32_t val1)
      {
        UpdateEnergy( (S+D)-1,val1);
        for (int i=(S+D)-1;i>S;i--) hist[i]=hist[i-1];hist[S]=val1;

        double sum=0.; // caluclate prediction
        for (int i=0;i<S+D;i++) sum+=w[i]*hist[i];
        pred=floor(sum+0.5);
        return pred;
      }
      void Update(int32_t val) {
        /*if (val<imin) imin=val;
        else if (val>imax) imax=val;*/

        #ifndef POSTUPDATE
          UpdateEnergy(S-1,val);
        #endif
        err=val-pred; // calculate prediction error

        const double wf=mu*err/(spow+0.001);
        for (int i=0;i<S+D;i++) w[i]+=wf*hist[i]; // update weight vector

        #ifdef POSTUPDATE
          UpdateEnergy(S-1,val);
        #endif
        for (int i=S-1;i>0;i--) hist[i]=hist[i-1];hist[0]=val;
      }
      ~NLMS2(){};
  private:
    void UpdateEnergy(int pos,int32_t val) {
     spow-=(hist[pos]*hist[pos]); // update energy in the tapping window
     spow+=(val*val);
    }
    Vector w,hist;
    double spow,mu,err;
    int pred,S,D;
    int imin,imax;
};


#endif // NLMS_H
