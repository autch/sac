#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "../global.h"

// general purpose predictor class
class MonoPredictor {
  public:
      virtual int32_t Predict()=0;
      virtual void Update(int32_t val)=0;
};

class O1Pred : public MonoPredictor {
  public:
      O1Pred():last(0){};
      int32_t Predict() {
        return last;
      }
      void Update(int32_t val) {
        last=val;
      }
  private:
    int32_t last;
};

#endif // PREDICTOR_H
