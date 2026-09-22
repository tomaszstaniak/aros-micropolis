#ifndef MICROPOLIS_BUDGET_MODEL_H
#define MICROPOLIS_BUDGET_MODEL_H
#include "micropolis.h"
#include <algorithm>
struct BudgetDraft {
    int tax;
    float road, fire, police;
    bool automatic;
    static BudgetDraft read(const Micropolis &m) {
        return {m.cityTax,m.roadPercent,m.firePercent,m.policePercent,m.autoBudget};
    }
    void set(int field,int value) {
        if(field==3) tax=std::max(0,std::min(20,value));
        else {
            float v=std::max(0,std::min(100,value))/100.0f;
            if(field==0)road=v;else if(field==1)fire=v;else if(field==2)police=v;
        }
    }
    void apply(Micropolis &m) const {
        m.setCityTax(tax);
        m.roadPercent=road; m.firePercent=fire; m.policePercent=police;
        m.setAutoBudget(automatic);
        // Menu recalculation allocates services but does not settle the year.
        // The frontend suppresses its nested showBudget callback while editing.
        m.doBudgetFromMenu();
    }
};
#endif
