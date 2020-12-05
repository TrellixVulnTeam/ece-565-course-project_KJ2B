#include "cpu/vpred/fcmvp.hh"

#include "base/intmath.hh"

FCMVP::FCMVP(const FCMVPParams *params)
    : VPredUnit(params),
      historyLength(params->historyLength),
      historyTableSize(params->historyTableSize),
      valuePredictorTableSize(params->valuePredictorTableSize),
      ctrBits(params->ctrBits),
      classificationTable(historyTableSize, SatCounter(ctrBits)),
      valueHistoryTable(historyTableSize, std::vector<RegVal> (historyLength)),
      valuePredictionTable(valuePredictorTableSize)
{
    // valuePredictionTable.resize(lastPredictorSize);
}

bool
FCMVP::lookup(Addr inst_addr, RegVal &value)
{
    unsigned indexVHT = inst_addr%historyTableSize;

    uint8_t counter_val = classificationTable[indexVHT];

    /*Gets the MSB of the count.*/
    bool prediction = counter_val >> (ctrBits-1);

    if (prediction)
    {
        RegVal hashedHistory = 0;

        for(int i = 0; i < historyLength; ++i)
        {
            hashedHistory = hashedHistory ^ valueHistoryTable[indexVHT][i];
        }
        unsigned indexVPT = hashedHistory%valuePredictorTableSize;
        value = valuePredictionTable[indexVPT];
    }

    return prediction;
    
}

float
FCMVP::getconf(Addr inst_addr, RegVal &value)
{
    unsigned index = inst_addr%historyTableSize;

    uint8_t counter_val = classificationTable[index];

    return float(counter_val/2^ctrBits); // Returning prediction confidence
}

void
FCMVP::updateTable(Addr inst_addr, bool isValuePredicted, bool isValueTaken, RegVal &trueValue)
{
    unsigned indexVHT = inst_addr%historyTableSize;;

    // Update Counters
    if (isValuePredicted)
    {
        if (isValueTaken)
        {
            // The Value predicted and True values are the same.
            classificationTable[indexVHT]++; 
        }
        else
        {
            // Decrease the counter and update the value to prediction table.
            classificationTable[indexVHT]--;
        }
    }
    else
    {
        /*Increasing the Counter when the Predictor doesn't predict, so that it predicts in next instance.*/
        classificationTable[indexVHT]++;
    }

    // Update History and Tables
    // Add the new data history at historyLength-1. O is the Least used.

    RegVal hashedHistory = valueHistoryTable[indexVHT][0];

    for(int i = historyLength-1; i > 0; --i)
    {
        hashedHistory = hashedHistory ^ valueHistoryTable[indexVHT][i];
        valueHistoryTable[indexVHT][i-1] = valueHistoryTable[indexVHT][i];
    }

    valueHistoryTable[indexVHT][historyLength-1] = trueValue;

    unsigned indexVPT = hashedHistory%valuePredictorTableSize;
    valuePredictionTable[indexVPT] = trueValue;
}

 FCMVP*
 FCMVPParams::create()
 {
     return new FCMVP(this);
 }
