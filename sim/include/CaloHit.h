#ifndef CaloHit_h
#define CaloHit_h 1

#include <vector>

#include "CaloID.h"

class CaloHit
{
public:
   CaloHit();
   ~CaloHit();

   void print();

   CaloID caloid;
   int pid;
   int trackid;
   int calotype;
   double x;
   double y;
   double z;
   double steplength;
   double globaltime;
   double localtime;
   double edep;
   double edepNonIon;
   double edepInv;
   double edepbirk;
   double ncer;    // number of cerenkov photons
   double ncercap; // number of cerenkov photons
};

#endif
